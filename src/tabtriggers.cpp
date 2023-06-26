#include "tabtriggers.h"
#include "ui_tabtriggers.h"

#include "message.h"
#include "cutil.h"
#include "config.h"

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
        item->setText(1, c.summary(true));

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

    for (idx = 0; idx < (long)seeds.size(); idx++)
    {
        if (stop) break;
        int64_t seed = seeds[idx];
        wi.seed = seed;
        QTreeWidgetItem *seeditem = new QTreeWidgetItem();
        seeditem->setText(0, QString::asprintf("%" PRId64, seed));
        seeditem->setData(0, Qt::UserRole, QVariant::fromValue(seed));

        if (!conds.empty())
        {
            ConditionTree condtree;
            SearchThreadEnv env;
            QString err = condtree.set(conds, wi.mc);
            if (err.isEmpty())
                err = env.init(wi.mc, wi.large, &condtree);
            if (!err.isEmpty())
            {
                delete seeditem;
                emit warning(err, QMessageBox::Ok);
                break;
            }
            env.setSeed(wi.seed);

            Pos origin = {0, 0};
            Pos cpos[MAX_INSTANCES] = {};
            if (testTreeAt(origin, &env, PASS_FULL_64, &stop, cpos)
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
    , nextupdate()
    , updt(20)
{
    ui->setupUi(this);
    ui->treeWidget->setColumnWidth(1, 280);
    ui->treeWidget->setColumnWidth(2, 65);
    ui->treeWidget->setColumnWidth(3, 65);
    ui->treeWidget->setSortingEnabled(false); // sortable triggers are not necessary

    connect(&thread, &AnalysisTriggers::warning, this, &TabTriggers::warning, Qt::BlockingQueuedConnection);
    connect(&thread, &AnalysisTriggers::itemDone, this, &TabTriggers::onAnalysisItemDone, Qt::BlockingQueuedConnection);
    connect(&thread, &AnalysisTriggers::finished, this, &TabTriggers::onAnalysisFinished);
}

TabTriggers::~TabTriggers()
{
    thread.stop = true;
    thread.wait(500);
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

int TabTriggers::warning(QString text, QMessageBox::StandardButtons buttons)
{
    return warn(parent, text, buttons);
}

void TabTriggers::onAnalysisItemDone(QTreeWidgetItem *item)
{
    qbuf.push_back(item);
    quint64 ns = elapsed.nsecsElapsed();
    if (ns > nextupdate)
    {
        nextupdate = ns + updt * 1e6;
        QTimer::singleShot(updt, this, &TabTriggers::onBufferTimeout);
    }
}

void TabTriggers::onAnalysisFinished()
{
    onBufferTimeout();
    ui->pushExport->setEnabled(ui->treeWidget->topLevelItemCount() > 0);
    ui->pushStart->setChecked(false);
    ui->pushStart->setText(tr("Analyze"));
}

void TabTriggers::onBufferTimeout()
{
    uint64_t t = -elapsed.elapsed();

    if (!qbuf.empty())
    {
        ui->treeWidget->setUpdatesEnabled(false);
        ui->treeWidget->addTopLevelItems(qbuf);
        ui->treeWidget->setUpdatesEnabled(true);

        QString progress = QString::asprintf(" (%ld/%zu)", thread.idx.load(), thread.seeds.size());
        ui->pushStart->setText(tr("Stop") + progress);

        qbuf.clear();
    }

    QApplication::processEvents(); // force processing of events so we can time correctly

    t += elapsed.elapsed();
    if (8*t > updt)
        updt = 4*t;
    nextupdate = elapsed.nsecsElapsed() + 1e6 * updt;
}

void TabTriggers::on_pushStart_clicked()
{
    if (thread.isRunning())
    {
        thread.stop = true;
        return;
    }
    updt = 20;
    nextupdate = 0;
    elapsed.start();

    parent->getSeed(&thread.wi);
    thread.conds = parent->formCond->getConditions();
    thread.seeds.clear();
    if (ui->comboSeedSource->currentIndex() == 0)
        thread.seeds.push_back(thread.wi.seed);
    else
        thread.seeds = parent->formControl->getResults();

    //ui->treeWidget->setSortingEnabled(false);
    while (ui->treeWidget->topLevelItemCount() > 0)
        delete ui->treeWidget->takeTopLevelItem(0);

    ui->pushExport->setEnabled(false);
    ui->pushStart->setChecked(true);
    QString progress = QString::asprintf(" (0/%zu)", thread.seeds.size());
    ui->pushStart->setText(tr("Stop") + progress);
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

static
void csvline(QTextStream& stream, const QString& qte, const QString& sep, QStringList& cols)
{
    if (qte.isEmpty())
    {
        for (QString& s : cols)
            if (s.contains(sep))
                s = "\"" + s + "\"";
    }
    stream << qte << cols.join(sep) << qte << "\n";
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
        warn(parent, tr("Failed to open file for export:\n\"%1\"").arg(fnam));
        return;
    }

    QString qte = parent->config.quote;
    QString sep = parent->config.separator;

    QTextStream stream(&file);
    stream << "Sep=" + sep + "\n";
    sep = qte + sep + qte;

    QStringList header = { tr("seed"), tr("condition"), tr("x"), tr("z") };
    csvline(stream, qte, sep, header);

    QTreeWidgetItemIterator it(ui->treeWidget);
    for (; *it; ++it)
    {
        QTreeWidgetItem *item = *it;
        QStringList cols;
        for (int i = 0, n = item->columnCount(); i < n; i++)
        {
            QString s = item->text(i);
            if (s == "-") s = "";
            cols.append(s);
        }
        csvline(stream, qte, sep, cols);
    }
}

