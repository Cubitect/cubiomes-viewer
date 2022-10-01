#include "tabstructures.h"
#include "ui_tabstructures.h"

#include "cutil.h"

#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QTextStream>
#include <QFileInfo>

#include <map>
#include <set>


enum { C_SEED, C_STRUCT, C_COUNT, C_X, C_Z, C_DETAIL }; // columns

class TreeIntItem : public QTreeWidgetItem
{
public:
    TreeIntItem(QTreeWidget *parent = nullptr) : QTreeWidgetItem(parent) {}
    bool operator< (const QTreeWidgetItem& x) const
    {
        int col = treeWidget()->sortColumn();
        if (col == C_SEED)
            return data(col, Qt::UserRole).toLongLong() < x.data(col, Qt::UserRole).toLongLong();
        return QTreeWidgetItem::operator< (x);
    }
};

void AnalysisStructures::run()
{
    stop = false;

    Generator g;
    setupGenerator(&g, wi.mc, wi.large);

    for (int64_t seed : qAsConst(seeds))
    {
        if (stop) break;
        wi.seed = seed;
        if (quad)
            runQuads(&g);
        else
            runStructs(&g);
    }
}

void AnalysisStructures::runStructs(Generator *g)
{
    QTreeWidgetItem *seeditem = new TreeIntItem();
    seeditem->setText(0, QString::asprintf("%" PRId64, wi.seed));
    seeditem->setData(0, Qt::UserRole+0, QVariant::fromValue(wi.seed));
    seeditem->setData(0, Qt::UserRole+1, QVariant::fromValue((int)DIM_UNDEF));

    std::vector<VarPos> st;
    for (int sopt = D_DESERT; sopt < D_SPAWN; sopt++)
    {
        if (stop)
            break;
        if (!mapshow[sopt])
            continue;

        int stype = mapopt2stype(sopt);
        st.clear();
        StructureConfig sconf;
        if (!getStructureConfig_override(stype, wi.mc, &sconf))
            continue;

        int sdim = DIM_OVERWORLD;
        if (sconf.properties & STRUCT_NETHER)
            sdim = DIM_NETHER;
        if (sconf.properties & STRUCT_END)
            sdim = DIM_END;
        getStructs(&st, sconf, wi, sdim, x1, z1, x2, z2);
        if (st.empty())
            continue;

        QTreeWidgetItem* stitem = new QTreeWidgetItem(seeditem);
        stitem->setText(C_SEED, "-");
        stitem->setText(C_STRUCT, struct2str(stype));
        stitem->setData(C_COUNT, Qt::DisplayRole, QVariant::fromValue(st.size()));
        if (!collect)
            continue;

        for (size_t i = 0; i < st.size(); i++)
        {
            VarPos vp = st[i];
            QTreeWidgetItem* item = new QTreeWidgetItem(stitem);
            item->setText(C_SEED, "-");
            item->setData(C_X, Qt::DisplayRole, QVariant::fromValue(vp.p.x));
            item->setData(C_Z, Qt::DisplayRole, QVariant::fromValue(vp.p.z));
            item->setData(0, Qt::UserRole+0, QVariant::fromValue(wi.seed));
            item->setData(0, Qt::UserRole+1, QVariant::fromValue(sdim));
            item->setData(0, Qt::UserRole+2, QVariant::fromValue(vp.p));
            QStringList sinfo = vp.detail();
            if (!sinfo.empty())
                item->setText(C_DETAIL, sinfo.join(":"));
        }
    }

    if (!stop && mapshow[D_SPAWN])
    {
        applySeed(g, 0, wi.seed);
        Pos pos = getSpawn(g);
        if (pos.x >= x1 && pos.x <= x2 && pos.z >= z1 && pos.z <= z2)
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(seeditem);
            item->setText(C_SEED, "-");
            item->setText(C_STRUCT, "spawn");
            item->setData(C_COUNT, Qt::DisplayRole, QVariant::fromValue(1));
            item->setData(C_X, Qt::DisplayRole, QVariant::fromValue(pos.x));
            item->setData(C_Z, Qt::DisplayRole, QVariant::fromValue(pos.z));
            item->setData(0, Qt::UserRole+0, QVariant::fromValue(wi.seed));
            item->setData(0, Qt::UserRole+1, QVariant::fromValue((int)DIM_OVERWORLD));
            item->setData(0, Qt::UserRole+2, QVariant::fromValue(pos));
        }
    }

    if (!stop && mapshow[D_STRONGHOLD])
    {
        StrongholdIter sh;
        initFirstStronghold(&sh, wi.mc, wi.seed);
        std::vector<Pos> shp;
        applySeed(g, DIM_OVERWORLD, wi.seed);

        // get the maximum relevant ring number
        int rx1 = abs(x1), rx2 = abs(x2);
        int rz1 = abs(z1), rz2 = abs(z2);
        int xt = (rx1 > rx2 ? rx1 : rx2) + 112+8;
        int zt = (rz1 > rz2 ? rz1 : rz2) + 112+8;
        int rmax = xt*xt + zt*zt;
        rmax = (int)((sqrt(rmax) - 1408) / 3072);

        while (nextStronghold(&sh, g) > 0)
        {
            if (stop || sh.ringnum > rmax)
                break;
            Pos pos = sh.pos;
            if (pos.x >= x1 && pos.x <= x2 && pos.z >= z1 && pos.z <= z2)
                shp.push_back(pos);
        }

        if (!shp.empty())
        {
            QTreeWidgetItem* stitem = new QTreeWidgetItem(seeditem);
            stitem->setText(C_SEED, "-");
            stitem->setText(C_STRUCT, "stronghold");
            stitem->setData(C_COUNT, Qt::DisplayRole, QVariant::fromValue(shp.size()));

            if (collect)
            {
                for (Pos pos : shp)
                {
                    QTreeWidgetItem* item = new QTreeWidgetItem(stitem);
                    item->setText(C_SEED, "-");
                    item->setData(C_X, Qt::DisplayRole, QVariant::fromValue(pos.x));
                    item->setData(C_Z, Qt::DisplayRole, QVariant::fromValue(pos.z));
                    item->setData(0, Qt::UserRole+0, QVariant::fromValue(wi.seed));
                    item->setData(0, Qt::UserRole+1, QVariant::fromValue((int)DIM_OVERWORLD));
                    item->setData(0, Qt::UserRole+2, QVariant::fromValue(pos));
                }
            }
        }
    }

    if (seeditem->childCount() == 0)
    {
        delete seeditem;
        return;
    }
    if (stop)
        seeditem->setText(0, QString::asprintf("%" PRId64, wi.seed) + " " + tr("(incomplete)"));
    emit itemDone(seeditem);
}

void AnalysisStructures::runQuads(Generator *g)
{
    applySeed(g, 0, wi.seed);

    QVector<QuadInfo> qsinfo;
    findQuadStructs(Swamp_Hut, g, &qsinfo);
    findQuadStructs(Monument, g, &qsinfo);
    if (qsinfo.empty())
        return;

    QTreeWidgetItem *seeditem = new TreeIntItem();
    seeditem->setText(0, QString::asprintf("%" PRId64, wi.seed));
    seeditem->setData(0, Qt::UserRole+0, QVariant::fromValue(wi.seed));
    seeditem->setData(0, Qt::UserRole+1, QVariant::fromValue((int)DIM_OVERWORLD));

    for (QuadInfo& qi : qsinfo)
    {
        QString label;
        if (qi.typ == Swamp_Hut)
            label = tr("quad-hut");
        else
            label = tr("quad-monument");

        QTreeWidgetItem *item = new QTreeWidgetItem(seeditem);

        qreal dist = qi.afk.x*(qreal)qi.afk.x + qi.afk.z*(qreal)qi.afk.z;
        dist = sqrt(dist);

        item->setText(0, "-");
        item->setData(1, Qt::DisplayRole, QVariant::fromValue(label));
        item->setData(2, Qt::DisplayRole, QVariant::fromValue((qlonglong)dist));
        item->setData(3, Qt::DisplayRole, QVariant::fromValue(qi.afk.x));
        item->setData(4, Qt::DisplayRole, QVariant::fromValue(qi.afk.z));
        item->setData(5, Qt::DisplayRole, QVariant::fromValue(qi.rad));
        item->setData(6, Qt::DisplayRole, QVariant::fromValue(qi.spcnt));
        item->setData(0, Qt::UserRole+0, QVariant::fromValue(wi.seed));
        item->setData(0, Qt::UserRole+1, QVariant::fromValue((int)DIM_OVERWORLD));
        item->setData(0, Qt::UserRole+2, QVariant::fromValue(qi.afk));
    }

    emit quadDone(seeditem);
}


TabStructures::TabStructures(MainWindow *parent)
    : QWidget(parent)
    , ui(new Ui::TabStructures)
    , parent(parent)
    , thread(this)
{
    ui->setupUi(this);

    ui->treeStructs->setColumnWidth(C_STRUCT, 160);
    ui->treeStructs->setColumnWidth(C_COUNT, 50);
    ui->treeStructs->setColumnWidth(C_X, 65);
    ui->treeStructs->setColumnWidth(C_Z, 65);
    ui->treeStructs->sortByColumn(0, Qt::AscendingOrder);

    ui->treeQuads->setColumnWidth(0, 160);
    ui->treeQuads->sortByColumn(0, Qt::AscendingOrder);

    connect(&thread, &AnalysisStructures::itemDone, this, &TabStructures::onAnalysisItemDone);
    connect(&thread, &AnalysisStructures::quadDone, this, &TabStructures::onAnalysisQuadDone);
    connect(&thread, &AnalysisStructures::finished, this, &TabStructures::onAnalysisFinished);

    connect(ui->treeStructs, &QTreeWidget::itemClicked, this, &TabStructures::onTreeItemClicked);
    connect(ui->treeQuads, &QTreeWidget::itemClicked, this, &TabStructures::onTreeItemClicked);
}

TabStructures::~TabStructures()
{
    delete ui;
}

void TabStructures::save(QSettings& settings)
{
    settings.setValue("analysis/x1", ui->lineX1->text().toInt());
    settings.setValue("analysis/z1", ui->lineZ1->text().toInt());
    settings.setValue("analysis/x2", ui->lineX2->text().toInt());
    settings.setValue("analysis/z2", ui->lineZ2->text().toInt());
    settings.setValue("analysis/seedsrc", ui->comboSeedSource->currentIndex());
    settings.setValue("analysis/maponly", ui->radioMap->isChecked());
    settings.setValue("analysis/collect", ui->checkCollect->isChecked());
}

static void loadCheck(QSettings *s, QCheckBox *cb, const char *key)
{
    cb->setChecked( s->value(key, cb->isChecked()).toBool() );
}
static void loadCombo(QSettings *s, QComboBox *combo, const char *key)
{
    combo->setCurrentIndex( s->value(key, combo->currentIndex()).toInt() );
}
static void loadLine(QSettings *s, QLineEdit *line, const char *key)
{
    qlonglong x = line->text().toLongLong();
    line->setText( QString::number(s->value(key, x).toLongLong()) );
}
void TabStructures::load(QSettings& settings)
{
    loadLine(&settings, ui->lineX1, "analysis/x1");
    loadLine(&settings, ui->lineZ1, "analysis/z1");
    loadLine(&settings, ui->lineX2, "analysis/x2");
    loadLine(&settings, ui->lineZ2, "analysis/z2");
    loadCombo(&settings, ui->comboSeedSource, "analysis/seedsrc");
    loadCheck(&settings, ui->checkCollect, "analysis/collect");
    if (settings.value("analysis/maponly", true).toBool())
        ui->radioMap->setChecked(true);
    else
        ui->radioAll->setChecked(true);
}

void TabStructures::onAnalysisItemDone(QTreeWidgetItem *item)
{
    ui->treeStructs->addTopLevelItem(item);
    ui->treeStructs->resizeColumnToContents(C_DETAIL);

    QString progress = QString::asprintf(" (%d/%d)", ui->treeStructs->topLevelItemCount()+1, thread.seeds.size());
    ui->pushStart->setText(tr("Stop") + progress);
}

void TabStructures::onAnalysisQuadDone(QTreeWidgetItem *item)
{
    ui->treeQuads->addTopLevelItem(item);
    item->setExpanded(true);

    QString progress = QString::asprintf(" (%d/%d)", ui->treeQuads->topLevelItemCount()+1, thread.seeds.size());
    ui->pushStart->setText(tr("Stop") + progress);
}

void TabStructures::onAnalysisFinished()
{
    ui->pushExport->setEnabled(ui->treeStructs->topLevelItemCount() > 0);
    ui->pushStart->setChecked(false);
    ui->pushStart->setText(tr("Analyze"));
}

void TabStructures::onTreeItemClicked(QTreeWidgetItem *item, int column)
{
    (void) column;
    QVariant dat;
    dat = item->data(0, Qt::UserRole);
    if (dat.isValid())
    {
        uint64_t seed = qvariant_cast<uint64_t>(dat);
        int dim = item->data(0, Qt::UserRole+1).toInt();
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

void TabStructures::on_pushStart_clicked()
{
    if (thread.isRunning())
    {
        thread.stop = true;
        return;
    }
    parent->getSeed(&thread.wi);
    thread.seeds.clear();
    if (ui->comboSeedSource->currentIndex() == 0)
        thread.seeds.append(thread.wi.seed);
    else
        thread.seeds = parent->formControl->getResults();

    int x1 = ui->lineX1->text().toInt();
    int z1 = ui->lineZ1->text().toInt();
    int x2 = ui->lineX2->text().toInt();
    int z2 = ui->lineZ2->text().toInt();
    if (x2 < x1) std::swap(x1, x2);
    if (z2 < z1) std::swap(z1, z2);
    thread.x1 = x1;
    thread.z1 = z1;
    thread.x2 = x2;
    thread.z2 = z2;

    thread.collect = ui->checkCollect->isChecked();

    for (int sopt = 0; sopt < STRUCT_NUM; sopt++)
        thread.mapshow[sopt] = ui->radioAll->isChecked() || parent->getMapView()->getShow(sopt);

    if (ui->tabWidget->currentWidget() == ui->tabStructures)
    {
        thread.quad = false;
        while (ui->treeStructs->topLevelItemCount() > 0)
            delete ui->treeStructs->takeTopLevelItem(0);
    }
    else
    {
        thread.quad = true;
        while (ui->treeStructs->topLevelItemCount() > 0)
            delete ui->treeStructs->takeTopLevelItem(0);
    }

    ui->pushExport->setEnabled(false);
    ui->pushStart->setChecked(true);
    ui->pushStart->setText(tr("Stop"));
    thread.start();
}

void TabStructures::on_pushExport_clicked()
{
    QString fnam = QFileDialog::getSaveFileName(
        this, tr("Export structure analysis"), parent->prevdir, tr("Text files (*.txt *csv);;Any files (*)"));
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

    stream << "#X1; " << thread.x1 << "\n";
    stream << "#Z1; " << thread.z1 << "\n";
    stream << "#X2; " << thread.x2 << "\n";
    stream << "#Z2; " << thread.z2 << "\n";

    QTreeWidgetItemIterator it(ui->treeStructs);

    if (ui->checkCollect->isChecked())
    {
        stream << "seed; structure; x; z; details\n";
        QString seed;
        QString structure;
        for (; *it; ++it)
        {
            QTreeWidgetItem *item = *it;
            if (item->text(C_SEED) != "-")
                seed = item->text(C_SEED);
            if (!item->text(C_STRUCT).isEmpty())
                structure = item->text(C_STRUCT);
            if (!item->data(0, Qt::UserRole+2).isValid())
                continue;

            QStringList cols;
            cols.append(seed);
            cols.append(structure);
            cols.append(item->text(C_X));
            cols.append(item->text(C_Z));
            cols.append(item->text(C_DETAIL));
            stream << cols.join(";") << "\n";
        }
    }
    else
    {
        std::set<QString> structures;
        std::map<uint64_t, std::map<QString, QString>> cnt; // [seed][stype]/[row][col]

        uint64_t seed;
        QString structure;
        for (; *it; ++it)
        {
            QTreeWidgetItem *item = *it;
            if (item->data(0, Qt::UserRole).isValid())
                seed = item->data(0, Qt::UserRole).toLongLong();
            if (!item->text(C_STRUCT).isEmpty())
                structures.insert((structure = item->text(C_STRUCT)));
            if (!item->text(C_COUNT).isEmpty())
                cnt[seed][structure] = item->text(C_COUNT);
        }

        QStringList header;
        header.append("seed");
        for (auto& sit : structures)
            header.append(sit);
        stream << header.join(";") << "\n";
        for (auto& m : cnt)
        {
            QStringList cols;
            cols << QString::asprintf("%" PRId64, m.first);
            for (auto& sit : structures)
            {
                QString cntstr = m.second[sit];
                if (cntstr.isEmpty())
                    cntstr = "0";
                cols.append(cntstr);
            }
            stream << cols.join(";") << "\n";
        }
    }
}

void TabStructures::on_buttonFromVisible_clicked()
{
    MapView *mapview = parent->getMapView();
    qreal uiw = mapview->width() * mapview->getScale();
    qreal uih = mapview->height() * mapview->getScale();
    int bx0 = (int) floor(mapview->getX() - uiw/2);
    int bz0 = (int) floor(mapview->getZ() - uih/2);
    int bx1 = (int) ceil(mapview->getX() + uiw/2);
    int bz1 = (int) ceil(mapview->getZ() + uih/2);

    ui->lineX1->setText( QString::number(bx0) );
    ui->lineZ1->setText( QString::number(bz0) );
    ui->lineX2->setText( QString::number(bx1) );
    ui->lineZ2->setText( QString::number(bz1) );
}
