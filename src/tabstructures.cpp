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

    for (idx = 0; idx < seeds.size(); idx++)
    {
        if (stop) break;
        wi.seed = seeds[idx];
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
        getStructs(&st, sconf, wi, sdim, area.x1, area.z1, area.x2, area.z2);
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
        if (pos.x >= area.x1 && pos.x <= area.x2 && pos.z >= area.z1 && pos.z <= area.z2)
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
        int rx1 = abs(area.x1), rx2 = abs(area.x2);
        int rz1 = abs(area.z1), rz2 = abs(area.z2);
        int xt = (rx1 > rx2 ? rx1 : rx2) + 112+8;
        int zt = (rz1 > rz2 ? rz1 : rz2) + 112+8;
        int rmax = xt*xt + zt*zt;
        rmax = (int)((sqrt(rmax) - 1408) / 3072);

        while (nextStronghold(&sh, g) > 0)
        {
            if (stop || sh.ringnum > rmax)
                break;
            Pos pos = sh.pos;
            if (pos.x >= area.x1 && pos.x <= area.x2 && pos.z >= area.z1 && pos.z <= area.z2)
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
    , sortcols(-1)
    , sortcolq(-1)
    , nextupdate()
    , updt(100)
{
    ui->setupUi(this);

    ui->treeStructs->setColumnWidth(C_STRUCT, 160);
    ui->treeStructs->setColumnWidth(C_COUNT, 50);
    ui->treeStructs->setColumnWidth(C_X, 65);
    ui->treeStructs->setColumnWidth(C_Z, 65);
    ui->treeStructs->sortByColumn(-1, Qt::AscendingOrder);
    connect(ui->treeStructs->header(), &QHeaderView::sectionClicked, this, [=](){ onHeaderClick(ui->treeStructs); } );

    ui->treeQuads->setColumnWidth(0, 160);
    ui->treeQuads->sortByColumn(-1, Qt::AscendingOrder);
    connect(ui->treeQuads->header(), &QHeaderView::sectionClicked, this, [=](){ onHeaderClick(ui->treeQuads); } );

    connect(&thread, &AnalysisStructures::itemDone, this, &TabStructures::onAnalysisItemDone, Qt::BlockingQueuedConnection);
    connect(&thread, &AnalysisStructures::quadDone, this, &TabStructures::onAnalysisQuadDone, Qt::BlockingQueuedConnection);
    connect(&thread, &AnalysisStructures::finished, this, &TabStructures::onAnalysisFinished);

    connect(ui->treeStructs, &QTreeWidget::itemClicked, this, &TabStructures::onTreeItemClicked);
    connect(ui->treeQuads, &QTreeWidget::itemClicked, this, &TabStructures::onTreeItemClicked);
}

TabStructures::~TabStructures()
{
    thread.stop = true;
    thread.wait(500);
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

void TabStructures::onHeaderClick(QTreeView *tree)
{
    int& col = (tree == ui->treeStructs) ? sortcols : sortcolq;
    int section =  tree->header()->sortIndicatorSection();
    if (tree->header()->sortIndicatorOrder() == Qt::AscendingOrder && col == section)
    {
        tree->sortByColumn(-1, Qt::DescendingOrder);
        section = -1;
    }
    col = section;
}

void TabStructures::onAnalysisItemDone(QTreeWidgetItem *item)
{
    qbufs.push_back(item);
    quint64 ns = elapsed.nsecsElapsed();
    if (ns > nextupdate)
    {
        nextupdate = ns + updt * 1e6;
        QTimer::singleShot(updt, this, &TabStructures::onBufferTimeout);
    }
}

void TabStructures::onAnalysisQuadDone(QTreeWidgetItem *item)
{
    qbufq.push_back(item);
    quint64 ns = elapsed.nsecsElapsed();
    if (ns > nextupdate)
    {
        nextupdate = ns + updt * 1e6;
        QTimer::singleShot(updt, this, &TabStructures::onBufferTimeout);
    }
}

void TabStructures::onAnalysisFinished()
{
    onBufferTimeout();
    on_tabWidget_currentChanged(-1);
    ui->treeStructs->setSortingEnabled(true);
    ui->treeQuads->setSortingEnabled(true);
    ui->pushStart->setChecked(false);
    ui->pushStart->setText(tr("Analyze"));
}

void TabStructures::onBufferTimeout()
{
    if (qbufs.empty() && qbufq.empty())
        return;
    uint64_t t = -elapsed.elapsed();
    if (!qbufs.empty())
    {
        ui->treeStructs->setSortingEnabled(false);
        ui->treeStructs->setUpdatesEnabled(false);
        ui->treeStructs->addTopLevelItems(qbufs);
        ui->treeStructs->resizeColumnToContents(C_DETAIL);
        ui->treeStructs->setUpdatesEnabled(true);
        ui->treeStructs->setSortingEnabled(true);
        qbufs.clear();
    }
    if (!qbufq.empty())
    {
        ui->treeQuads->setSortingEnabled(false);
        ui->treeQuads->setUpdatesEnabled(false);
        ui->treeQuads->addTopLevelItems(qbufq);
        for (QTreeWidgetItem *item: qAsConst(qbufq))
            item->setExpanded(true);
        ui->treeQuads->setUpdatesEnabled(true);
        ui->treeQuads->setSortingEnabled(true);
        qbufq.clear();
    }
    QString progress = QString::asprintf(" (%d/%d)", thread.idx.load(), thread.seeds.size());
    ui->pushStart->setText(tr("Stop") + progress);

    QApplication::processEvents(); // force processing of events so we can time correctly

    t += elapsed.elapsed();
    if (8*t > updt)
        updt = 4*t;
    nextupdate = elapsed.nsecsElapsed() + 1e6 * updt;
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
    updt = 20;
    nextupdate = 0;
    elapsed.start();

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
    thread.area = AnalysisStructures::Dat{x1, z1, x2, z2};

    thread.collect = ui->checkCollect->isChecked();

    for (int sopt = 0; sopt < STRUCT_NUM; sopt++)
        thread.mapshow[sopt] = ui->radioAll->isChecked() || parent->getMapView()->getShow(sopt);

    if (ui->tabWidget->currentWidget() == ui->tabStructures)
    {
        thread.quad = false;
        dats = thread.area;
        ui->treeStructs->setSortingEnabled(false);
        while (ui->treeStructs->topLevelItemCount() > 0)
            delete ui->treeStructs->takeTopLevelItem(0);
        ui->treeStructs->setSortingEnabled(true);
    }
    else
    {
        thread.quad = true;
        datq = thread.area;
        ui->treeQuads->setSortingEnabled(false);
        while (ui->treeQuads->topLevelItemCount() > 0)
            delete ui->treeQuads->takeTopLevelItem(0);
        ui->treeQuads->setSortingEnabled(true);
    }

    ui->pushExport->setEnabled(false);
    ui->pushStart->setChecked(true);
    QString progress = QString::asprintf(" (0/%d)", thread.seeds.size());
    ui->pushStart->setText(tr("Stop") + progress);
    thread.start();
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

    QString qte = parent->config.quote;
    QString sep = parent->config.separator;

    QTextStream stream(&file);
    stream << "Sep=" + sep + "\n";
    sep = qte + sep + qte;

    if (ui->tabWidget->currentWidget() == ui->tabStructures)
    {
        stream << qte << "#X1" << sep << dats.x1 << qte << "\n";
        stream << qte << "#Z1" << sep << dats.z1 << qte << "\n";
        stream << qte << "#X2" << sep << dats.x2 << qte << "\n";
        stream << qte << "#Z2" << sep << dats.z2 << qte << "\n";

        if (ui->checkCollect->isChecked())
        {
            QStringList header = { tr("seed"), tr("structure"), tr("x"), tr("z"), tr("details") };
            csvline(stream, qte, sep, header);
            QString seed;
            QString structure;
            for (QTreeWidgetItemIterator it(ui->treeStructs); *it; ++it)
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
                csvline(stream, qte, sep, cols);
            }
        }
        else
        {
            std::set<QString> structures;
            std::map<uint64_t, std::map<QString, QString>> cnt; // [seed][stype]

            uint64_t seed;
            QString structure;
            for (QTreeWidgetItemIterator it(ui->treeStructs); *it; ++it)
            {
                QTreeWidgetItem *item = *it;
                if (item->data(0, Qt::UserRole).isValid())
                    seed = item->data(0, Qt::UserRole).toLongLong();
                if (!item->text(C_STRUCT).isEmpty())
                    structures.insert((structure = item->text(C_STRUCT)));
                if (!item->text(C_COUNT).isEmpty())
                    cnt[seed][structure] = item->text(C_COUNT);
            }

            QStringList header = { tr("seed") };
            for (auto& sit : structures)
                header.append(sit);
            csvline(stream, qte, sep, header);
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
                csvline(stream, qte, sep, cols);
            }
        }
    }
    else if(ui->tabWidget->currentWidget() == ui->tabQuads)
    {
        stream << qte << "#X1" << sep << datq.x1 << qte << "\n";
        stream << qte << "#Z1" << sep << datq.z1 << qte << "\n";
        stream << qte << "#X2" << sep << datq.x2 << qte << "\n";
        stream << qte << "#Z2" << sep << datq.z2 << qte << "\n";

        QStringList header = { tr("seed"), tr("type"), tr("distance"), tr("x"), tr("z"), tr("radius"), tr("spawn area") };
        csvline(stream, qte, sep, header);
        QString seed;
        for (QTreeWidgetItemIterator it(ui->treeQuads); *it; ++it)
        {
            QTreeWidgetItem *item = *it;
            if (item->text(0) != "-")
            {
                seed = item->text(0);
                continue;
            }
            QStringList cols = { seed };
            for (int i = 1, n = item->columnCount(); i < n; i++)
                cols.append(item->text(i));
            csvline(stream, qte, sep, cols);
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

void TabStructures::on_tabWidget_currentChanged(int)
{
    bool ok = false;
    if (!thread.isRunning())
    {
        if (ui->tabWidget->currentWidget() == ui->tabStructures)
            ok = ui->treeStructs->topLevelItemCount() > 0;
        if (ui->tabWidget->currentWidget() == ui->tabQuads)
            ok = ui->treeQuads->topLevelItemCount() > 0;
    }
    ui->pushExport->setEnabled(ok);
}
