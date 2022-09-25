#include "tabtriggers.h"
#include "ui_tabtriggers.h"

#include "cutil.h"
#include "settings.h"

#include <QElapsedTimer>
#include <QFileDialog>
#include <QTextStream>
#include <QFileInfo>

#include <vector>


static
QTreeWidgetItem *setConditionTreeItems(ConditionTree& ctree, int node, int64_t seed, Pos cpos[], QTreeWidgetItem* parent, bool posval)
{
    Condition& c = ctree.condvec[node];
    Pos p = cpos[c.save];
    const std::vector<char>& branches = ctree.references[c.save];
    QTreeWidgetItem* item;

    if (c.type == 0)
    {
        item = parent;
    }
    else
    {
        item = new QTreeWidgetItem(parent);
        item->setText(0, "-");
        item->setText(1, c.summary());

        if ((p.x == -1 && p.z == -1) || c.type == F_LOGIC_NOT)
            posval = false;
        if (posval)
        {
            const FilterInfo& finfo = g_filterinfo.list[c.type];
            item->setText(2, QString::number(p.x));
            item->setText(3, QString::number(p.z));
            item->setData(0, Qt::UserRole+0, QVariant::fromValue(seed));
            item->setData(0, Qt::UserRole+1, QVariant::fromValue(finfo.dim));
            item->setData(0, Qt::UserRole+2, QVariant::fromValue(p));
        }
    }
    if (!branches.empty())
    {
        for (char b : branches)
            setConditionTreeItems(ctree, b, seed, cpos, item, posval);
    }
    return item;
}

void AnalysisTriggers::run()
{
    stop = false;

    for (int64_t seed : qAsConst(seeds))
    {
        if (stop) break;
        wi.seed = seed;
        QTreeWidgetItem *seeditem = new QTreeWidgetItem();
        seeditem->setText(0, QString::asprintf("%" PRId64, seed));
        seeditem->setData(0, Qt::UserRole, QVariant::fromValue(seed));

        if (!conds.empty())
        {
            WorldGen gen;
            gen.init(wi.mc, wi.large);
            gen.setSeed(wi.seed);

            ConditionTree condtree;
            condtree.set(conds, wi);

            Pos origin = {0, 0};
            Pos cpos[MAX_INSTANCES] = {};
            if (testTreeAt(origin, &condtree, PASS_FULL_64, &gen, &stop, cpos)
                == COND_OK)
            {
                setConditionTreeItems(condtree, 0, seed, cpos, seeditem, true);
            }
        }

        if (seeditem->childCount() == 0)
        {
            delete seeditem;
            continue;
        }
        if (stop)
            seeditem->setText(0, QString::asprintf("%" PRId64, seed) + " " + tr("(incomplete)"));
        emit itemDone(seeditem);
    }
}


TabTriggers::TabTriggers(MainWindow *parent)
    : QWidget(parent)
    , ui(new Ui::TabTriggers)
    , parent(parent)
    , thread()
{
    ui->setupUi(this);
    ui->treeWidget->setColumnWidth(1, 280);
    ui->treeWidget->setColumnWidth(2, 65);
    ui->treeWidget->setColumnWidth(3, 65);

    ui->treeWidget->sortByColumn(0, Qt::AscendingOrder);
    connect(&thread, &AnalysisTriggers::itemDone, this, &TabTriggers::onAnalysisItemDone);
    connect(&thread, &AnalysisTriggers::finished, this, &TabTriggers::onAnalysisFinished);
}

TabTriggers::~TabTriggers()
{
    delete ui;
}

void TabTriggers::save(QSettings& settings)
{
    settings.setValue("analysis/seedsrc", ui->comboSeedSource->currentIndex());
}

void TabTriggers::load(QSettings& settings)
{
    int idx = settings.value("analysis/seedsrc", ui->comboSeedSource->currentIndex()).toInt();
    ui->comboSeedSource->setCurrentIndex(idx);
}

void TabTriggers::onAnalysisItemDone(QTreeWidgetItem *item)
{
    ui->treeWidget->addTopLevelItem(item);
    QString progress = QString::asprintf(" (%d/%d)", ui->treeWidget->topLevelItemCount()+1, thread.seeds.size());
    ui->pushStart->setText(tr("Stop") + progress);
}

void TabTriggers::onAnalysisFinished()
{
    ui->pushExport->setEnabled(ui->treeWidget->topLevelItemCount() > 0);
    ui->pushStart->setChecked(false);
    ui->pushStart->setText(tr("Analyze"));
}

void TabTriggers::on_pushStart_clicked()
{
    if (thread.isRunning())
    {
        thread.stop = true;
        return;
    }
    parent->getSeed(&thread.wi);
    thread.conds = parent->formCond->getConditions();
    thread.seeds.clear();
    if (ui->comboSeedSource->currentIndex() == 0)
        thread.seeds.append(thread.wi.seed);
    else
        thread.seeds = parent->formControl->getResults();

    while (ui->treeWidget->topLevelItemCount() > 0)
        delete ui->treeWidget->takeTopLevelItem(0);

    ui->pushExport->setEnabled(false);
    ui->pushStart->setChecked(true);
    ui->pushStart->setText(tr("Stop"));
    thread.start();
}

void TabTriggers::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    (void) column;
    QVariant dat;
    dat = item->data(0, Qt::UserRole+0);
    if (dat.isValid())
    {
        uint64_t seed = qvariant_cast<uint64_t>(dat);
        dat = item->data(0, Qt::UserRole+1);
        int dim = dat.isValid() ? dat.toInt() : DIM_UNDEF;
        WorldInfo wi;
        parent->getSeed(&wi);
        wi.seed = seed;
        parent->setSeed(wi, dim);
    }

    dat = item->data(0, Qt::UserRole+2);
    if (dat.isValid())
    {
        Pos p = qvariant_cast<Pos>(dat);
        parent->getMapView()->setView(p.x+0.5, p.z+0.5);
    }
}

void TabTriggers::on_pushExpand_clicked()
{
    ui->treeWidget->expandAll();
}

void TabTriggers::on_pushExport_clicked()
{
    QString fnam = QFileDialog::getSaveFileName(
        this, tr("Export trigger analysis"), parent->prevdir, tr("Text files (*.txt *csv);;Any files (*)"));
    if (fnam.isEmpty())
        return;

    QFileInfo finfo(fnam);
    QFile file(fnam);
    parent->prevdir = finfo.absolutePath();

    if (!file.open(QIODevice::WriteOnly))
    {
        parent->warning(tr("Failed to open file for export:\n\"%1\"").arg(fnam));
        return;
    }

    QTextStream stream(&file);
    stream << "#seed; condition; x; z\n";

    QTreeWidgetItemIterator it(ui->treeWidget);
    for (; *it; ++it)
    {
        QTreeWidgetItem *item = *it;
        QStringList cols;
        for (int i = 0, n = item->columnCount(); i < n; i++)
        {
            QString txt = item->text(i);
            if (txt == "-") txt = "";
            if (i == 1) txt = "\"" + txt + "\"";
            cols << txt;
        }
        stream << cols.join("; ") << "\n";
    }
}

