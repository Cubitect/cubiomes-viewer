#include "tabbiomes.h"
#include "ui_tabbiomes.h"
#include "cutil.h"
#include "world.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpressionValidator>
#include <QScrollBar>

#include <unordered_set>

void AnalysisBiomes::run()
{
    stop = false;

    Generator g;
    setupGenerator(&g, wi.mc, wi.large);

    for (idx = 0; idx < seeds.size(); idx++)
    {
        if (stop) break;
        wi.seed = seeds[idx];
        if (dat.locate >= 0)
            runLocate(&g);
        else
            runStatistics(&g);
    }
}

void AnalysisBiomes::runStatistics(Generator *g)
{
    QVector<uint64_t> idcnt(256);
    int w = dat.x2 - dat.x1 + 1;
    int h = dat.z2 - dat.z1 + 1;
    uint64_t n = w * (uint64_t)h;

    for (int d = 0; d < 3; d++)
    {
        if (dims[d] == DIM_UNDEF)
            continue;
        applySeed(g, dims[d], wi.seed);

        if (dat.samples >= n)
        {   // full area gen => generate 512x512 areas at a time
            const int step = 512;
            for (int x = dat.x1; x <= dat.x2 && !stop; x += step)
            {
                for (int z = dat.z1; z <= dat.z2 && !stop; z += step)
                {
                    int w = dat.x2-x+1 < step ? dat.x2-x+1 : step;
                    int h = dat.z2-z+1 < step ? dat.z2-z+1 : step;
                    Range r = {dat.scale, x, z, w, h, wi.y, 1};
                    int *ids = allocCache(g, r);
                    genBiomes(g, ids, r);
                    for (int i = 0; i < w*h; i++)
                        idcnt[ ids[i] & 0xff ]++;
                    free(ids);
                }
            }
        }
        else
        {
            std::vector<uint64_t> order;

            if (dat.samples * 2 >= n)
            {   // dense regime => shuffle indeces
                order.resize(n);
                for (uint64_t i = 0; i < n; i++)
                    order[i] = i;
                for (uint64_t i = 0; i < n; i++)
                {
                    if (!(i & 0xffff) && stop)
                        break;
                    uint64_t idx = getRnd64() % n;
                    uint64_t t = order[i];
                    order[i] = order[idx];
                    order[idx] = t;
                }
                order.resize(dat.samples);
            }
            else
            {   // sparse regime => fill randomly without reuse
                std::unordered_set<uint64_t> used;
                order.reserve(dat.samples);
                used.reserve(dat.samples);
                for (uint64_t i = 0; order.size() < dat.samples; i++)
                {
                    if (!(i & 0xffff) && stop)
                        break;
                    uint64_t idx = getRnd64() % n;
                    auto it = used.insert(idx);
                    if (it.second)
                        order.push_back(idx);
                }
            }

            for (uint64_t i = 0; i < dat.samples && !stop; i++)
            {
                uint64_t idx = order[i];
                int x = (int) (idx % w);
                int z = (int) (idx / w);
                int id = getBiomeAt(g, dat.scale, dat.x1+x, wi.y, dat.z1+z);
                idcnt[ id & 0xff ]++;
            }
        }
    }

    if (!stop) // discard partially processed seed
        emit seedDone(wi.seed, idcnt);
}

void AnalysisBiomes::runLocate(Generator *g)
{
    applySeed(g, DIM_OVERWORLD, wi.seed);
    enum { MAX_LOCATE = 4096 };
    Pos pos[MAX_LOCATE];
    int siz[MAX_LOCATE];
    Range r = {4, dat.x1, dat.z1, dat.x2-dat.x1+1, dat.z2-dat.z1+1, wi.y, 1};
    int n = getBiomeCenters(
        pos, siz, MAX_LOCATE, g, r, dat.locate, minsize, tolerance,
        (volatile char*)&stop
    );
    if (n && !stop)
    {
        QTreeWidgetItem *seeditem = new QTreeWidgetItem();
        seeditem->setData(0, Qt::DisplayRole, QVariant::fromValue((qlonglong)wi.seed));
        seeditem->setData(0, Qt::UserRole+0, QVariant::fromValue(wi.seed));
        seeditem->setData(0, Qt::UserRole+1, QVariant::fromValue((int)DIM_OVERWORLD));
        for (int i = 0; i < n; i++)
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(seeditem);
            item->setText(0, "-");
            item->setData(1, Qt::DisplayRole, QVariant::fromValue(siz[i]));
            item->setData(2, Qt::DisplayRole, QVariant::fromValue(pos[i].x));
            item->setData(3, Qt::DisplayRole, QVariant::fromValue(pos[i].z));
            item->setData(0, Qt::UserRole+0, QVariant::fromValue(wi.seed));
            item->setData(0, Qt::UserRole+1, QVariant::fromValue((int)DIM_OVERWORLD));
            item->setData(0, Qt::UserRole+2, QVariant::fromValue(pos[i]));
        }
        emit seedItem(seeditem);
    }
}


QVariant BiomeTableModel::data(const QModelIndex& index, int role) const
{
    if (role != Qt::DisplayRole || index.row() < 0 || index.column() < 0)
        return QVariant::Invalid;
    int id = ids[index.column()];
    uint64_t seed = seeds[index.row()];
    return cnt[id][seed];
}

QVariant BiomeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0)
        return QVariant::Invalid;
    if (role == Qt::InitialSortOrderRole)
        return QVariant::fromValue(Qt::AscendingOrder);
    if (role == Qt::DisplayRole && orientation == Qt::Vertical)
    {
        if  (section < seeds.size())
            return QVariant::fromValue((int64_t)seeds[section]);
    }
    if (orientation == Qt::Horizontal)
    {
        if (section < ids.size())
        {
            if (role == Qt::DisplayRole)
                return QVariant::fromValue(QString(biome2str(cmp.mc, ids[section])));
            else
                return QVariant::fromValue(ids[section]);
        }
    }
    return QVariant::Invalid;
}

void BiomeTableModel::insertIds(QSet<int>& nids)
{
    for (int id : qAsConst(nids))
    {
        QList<int>::iterator it = std::lower_bound(ids.begin(), ids.end(), id, cmp);
        if (it != ids.end() && *it == id)
            continue;
        int i = std::distance(ids.begin(), it);
        beginInsertColumns(QModelIndex(), i, i);
        ids.insert(i, id);
        endInsertColumns();
    }
}

void BiomeTableModel::insertSeeds(QList<uint64_t>& nseeds)
{
    int i = seeds.size();
    beginInsertRows(QModelIndex(), i, i+nseeds.size()-1);
    seeds.append(nseeds);
    endInsertRows();
}

void BiomeTableModel::reset(int mc)
{
    beginResetModel();
    seeds.clear();
    ids.clear();
    cnt.clear();
    cmp.mode = IdCmp::SORT_DIM;
    cmp.dim = DIM_UNDEF;
    cmp.mc = mc;
    endResetModel();
}

BiomeHeader::BiomeHeader(QWidget *parent)
    : QHeaderView(Qt::Horizontal, parent)
    , hover(-1)
    , pressed(-1)
{
    setSectionsClickable(true);
    setHighlightSections(true);
    connect(this, &QHeaderView::sectionPressed, this, &BiomeHeader::onSectionPress);
}

void BiomeHeader::onSectionPress(int section)
{
    pressed = section;
}

bool BiomeHeader::event(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::HoverEnter:
    case QEvent::HoverMove:
        hover = logicalIndexAt(((QHoverEvent*)e)->pos());
        break;
    case QEvent::Leave:
    case QEvent::HoverLeave:
        hover = -1;
        break;
    default: break;
    }
    return QHeaderView::event(e);
}

void BiomeHeader::paintSection(QPainter *painter, const QRect& rect, int section) const
{
    if (!rect.isValid() || !model())
        return;

    QStyleOptionHeader opt;
    initStyleOption(&opt);

    QStyle::State state = QStyle::State_None;
    state |= QStyle::State_Enabled;
    state |= QStyle::State_Active;
    if (section == hover)
        state |= QStyle::State_MouseOver;
    if (section == pressed)
        state |= QStyle::State_Sunken;

    QString s = model()->headerData(section, orientation()).toString();
    painter->setFont(font());
    QFontMetrics fm(font());
    int indicator_height = 0;
    int margin = 2 * style()->pixelMetric(QStyle::PM_HeaderMargin, 0, this);
    QStyleOptionHeader::SortIndicator sortindicator = QStyleOptionHeader::None;

    if (isSortIndicatorShown() && sortIndicatorSection() == section)
    {
        if (sortIndicatorOrder() == Qt::AscendingOrder)
            sortindicator = QStyleOptionHeader::SortDown;
        else
            sortindicator = QStyleOptionHeader::SortUp;
        indicator_height = 20;
    }

    int x = -rect.height() + margin + indicator_height;
    int y = rect.left() + (rect.width() + fm.descent()) / 2 + margin;

    opt.rect = rect;
    opt.section = section;
    opt.state = state;

    QPointF oldBO = painter->brushOrigin();
    painter->save();

    painter->setBrushOrigin(opt.rect.topLeft());
    style()->drawControl(QStyle::CE_Header, &opt, painter, this);

    painter->restore();

    painter->rotate(-90);
    painter->drawText(x, y, s);
    painter->rotate(+90);

    if (sortindicator != QStyleOptionHeader::None)
    {
        opt.sortIndicator = sortindicator;
        opt.rect = rect.adjusted(0, rect.bottom()-rect.y()-indicator_height, 0, 0);
        style()->drawControl(QStyle::CE_Header, &opt, painter, this);
        painter->setBrushOrigin(oldBO);
    }
    painter->setBrushOrigin(oldBO);
}

QSize BiomeHeader::sectionSizeFromContents(int section) const
{
    if (!model())
        return QSize();
    int margin = 2 * style()->pixelMetric(QStyle::PM_HeaderMargin, 0, this);
    QFontMetrics fm(font());
    int w = fm.boundingRect(model()->headerData(section, orientation()).toString()).width();
    return QSize(fm.height() + 2*margin, w + 2*margin);
}



TabBiomes::TabBiomes(MainWindow *parent)
    : QWidget(parent)
    , ui(new Ui::TabBiomes)
    , parent(parent)
    , thread()
    , model(new BiomeTableModel(this))
    , proxy(new BiomeSortProxy(this))
    , sortcol(-1)
    , elapsed()
    , updt(20)
    , nextupdate()
{
    ui->setupUi(this);

    proxy->setSourceModel(model);
    ui->table->setModel(proxy);

    BiomeHeader *header = new BiomeHeader(ui->table);
    ui->table->setHorizontalHeader(header);
    //QHeaderView *header = ui->table->horizontalHeader();
    connect(header, &QHeaderView::sortIndicatorChanged, this, &TabBiomes::onTableSort);

    ui->table->setFont(*gp_font_mono);
    ui->table->setSortingEnabled(true);

    ui->treeLocate->setColumnWidth(0, 160);
    ui->treeLocate->setColumnWidth(1, 120);
    ui->treeLocate->sortByColumn(-1, Qt::DescendingOrder);
    ui->treeLocate->setSortingEnabled(true);
    connect(ui->treeLocate->header(), &QHeaderView::sectionClicked, this, &TabBiomes::onLocateHeaderClick);

    QIntValidator *intval = new QIntValidator(-60e6, 60e6, this);
    ui->lineX1->setValidator(intval);
    ui->lineZ1->setValidator(intval);
    ui->lineX2->setValidator(intval);
    ui->lineZ2->setValidator(intval);

    ui->lineBiomeSize->setValidator(new QIntValidator(1, INT_MAX, this));
    ui->lineTolerance->setValidator(new QIntValidator(0, 255, this));
    ui->lineBiomeSize->setText("1");

    connect(&thread, &AnalysisBiomes::seedDone, this, &TabBiomes::onAnalysisSeedDone, Qt::BlockingQueuedConnection);
    connect(&thread, &AnalysisBiomes::seedItem, this, &TabBiomes::onAnalysisSeedItem, Qt::BlockingQueuedConnection);
    connect(&thread, &AnalysisBiomes::finished, this, &TabBiomes::onAnalysisFinished);

    for (int id = 0; id < 256; id++)
    {
        const char *s;
        if ((s = biome2str(MC_1_17, id)))
            str2biome[s] = id;
        if ((s = biome2str(MC_NEWEST, id)))
            str2biome[s] = id;
    }

    const QStringList bnames = str2biome.keys();
    QRegularExpressionValidator *reval = new QRegularExpressionValidator(
        QRegularExpression("(" + bnames.join("|") + ")"), this
    );
    ui->comboBiome->lineEdit()->setValidator(reval);
}

TabBiomes::~TabBiomes()
{
    thread.stop = true;
    thread.wait(500);
    delete ui;
}

void TabBiomes::save(QSettings& settings)
{
    settings.setValue("analysis/x1", ui->lineX1->text().toInt());
    settings.setValue("analysis/z1", ui->lineZ1->text().toInt());
    settings.setValue("analysis/x2", ui->lineX2->text().toInt());
    settings.setValue("analysis/z2", ui->lineZ2->text().toInt());
    settings.setValue("analysis/seedsrc", ui->comboSeedSource->currentIndex());
    settings.setValue("analysis/scaleidx", ui->comboScale->currentIndex());
    settings.setValue("analysis/overworld", ui->checkOverworld->isChecked());
    settings.setValue("analysis/nether", ui->checkNether->isChecked());
    settings.setValue("analysis/end", ui->checkEnd->isChecked());
    settings.setValue("analysis/samples", ui->lineSamples->text().toULongLong());
    settings.setValue("analysis/fullarea", ui->radioFullSample->isChecked());
    settings.setValue("analysis/biomeid", str2biome[ui->comboBiome->currentText()]);
    settings.setValue("analysis/biomesize", ui->lineBiomeSize->text().toInt());
    settings.setValue("analysis/tolerance", ui->lineTolerance->text().toInt());
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
void TabBiomes::load(QSettings& settings)
{
    loadLine(&settings, ui->lineX1, "analysis/x1");
    loadLine(&settings, ui->lineZ1, "analysis/z1");
    loadLine(&settings, ui->lineX2, "analysis/x2");
    loadLine(&settings, ui->lineZ2, "analysis/z2");
    loadCombo(&settings, ui->comboSeedSource, "analysis/seedsrc");
    loadCombo(&settings, ui->comboScale, "analysis/scaleidx");
    loadCheck(&settings, ui->checkOverworld, "analysis/overworld");
    loadCheck(&settings, ui->checkNether, "analysis/nether");
    loadCheck(&settings, ui->checkEnd, "analysis/end");
    loadLine(&settings, ui->lineSamples, "analysis/samples");
    if (settings.value("analysis/fullarea", true).toBool())
        ui->radioFullSample->setChecked(true);
    else
        ui->radioStochastic->setChecked(true);
    refreshBiomes(settings.value("analysis/biomeid", -1).toInt());
    loadLine(&settings, ui->lineBiomeSize, "analysis/biomesize");
    loadLine(&settings, ui->lineTolerance, "analysis/tolerance");
}

void TabBiomes::refreshBiomes(int activeid)
{
    WorldInfo wi;
    parent->getSeed(&wi);
    if (activeid == -1)
    {
        QString s = ui->comboBiome->currentText();
        if (str2biome.count(s))
            activeid = str2biome[s];
    }
    std::vector<int> ids;
    for (int i = 0; i < 256; i++)
        if (isOverworld(wi.mc, i) || i == activeid)
            ids.push_back(i);
    IdCmp cmp(IdCmp::SORT_LEX, wi.mc, DIM_UNDEF);
    std::sort(ids.begin(), ids.end(), cmp);
    ui->comboBiome->clear();
    for (int i : ids)
        ui->comboBiome->addItem(getBiomeIcon(i), biome2str(wi.mc, i), QVariant::fromValue(i));
    if (activeid >= 0)
    {
        int idx = ui->comboBiome->findText(biome2str(wi.mc, activeid));
        ui->comboBiome->setCurrentIndex(idx);
    }
}

void TabBiomes::onLocateHeaderClick()
{
    int section =  ui->treeLocate->header()->sortIndicatorSection();
    if (ui->treeLocate->header()->sortIndicatorOrder() == Qt::AscendingOrder && sortcol == section)
    {
        ui->treeLocate->sortByColumn(-1, Qt::DescendingOrder);
        section = -1;
    }
    sortcol = section;
}

void TabBiomes::onTableSort(int, Qt::SortOrder)
{
    QHeaderView *header = ui->table->horizontalHeader();

    if (proxy->order == Qt::DescendingOrder && proxy->column != -1)
    {
        header->setSortIndicatorShown(false);
        header->setSortIndicator(-1, Qt::AscendingOrder);
        proxy->column = -1;
    }
    else
    {
        header->setSortIndicatorShown(true);
    }
}

void TabBiomes::onAnalysisSeedDone(uint64_t seed, QVector<uint64_t> idcnt)
{
    idcnt.push_back(seed);
    qbufs.push_back(idcnt);
    quint64 ns = elapsed.nsecsElapsed();
    if (ns > nextupdate)
    {
        nextupdate = ns + updt * 1e6;
        QTimer::singleShot(updt, this, &TabBiomes::onBufferTimeout);
    }
}

void TabBiomes::onAnalysisSeedItem(QTreeWidgetItem *item)
{
    qbufl.push_back(item);
    quint64 ns = elapsed.nsecsElapsed();
    if (ns > nextupdate)
    {
        nextupdate = ns + updt * 1e6;
        QTimer::singleShot(updt, this, &TabBiomes::onBufferTimeout);
    }
}

void TabBiomes::onAnalysisFinished()
{
    onBufferTimeout();
    on_tabWidget_currentChanged(-1);
    ui->pushStart->setChecked(false);
    ui->pushStart->setText(tr("Analyze"));
}

void TabBiomes::onBufferTimeout()
{
    if (qbufs.empty() && qbufl.empty())
        return;

    uint64_t t = -elapsed.elapsed();

    if (!qbufs.empty())
    {
        ui->table->setSortingEnabled(false);
        ui->table->setUpdatesEnabled(false);

        QMap<int, int> colwidth;
        for (int c = 0, n = model->ids.size(); c < n; c++)
            colwidth[model->ids[c]] = ui->table->columnWidth(c);

        QList<uint64_t> new_seeds;
        QSet<int> new_ids;
        QFontMetrics fm(font());

        for (int i = 0, n = qbufs.size(); i < n; i++)
        {
            QVector<uint64_t>& scnt = qbufs[i];
            uint64_t seed = scnt.back();
            scnt.resize(scnt.size()-1);

            new_seeds.push_back(seed);
            for (int id = 0, idn = scnt.size(); id < idn; id++)
            {
                uint64_t cnt = scnt[id];
                if (cnt == 0)
                    continue;
                new_ids.insert(id);
                model->cnt[id][seed] = QVariant::fromValue(cnt);
                int w = fm.boundingRect(QString::number(cnt) + "_").width() + 2;
                if (w > colwidth[id])
                    colwidth[id] = w;
            }
        }
        model->insertIds(new_ids);
        model->insertSeeds(new_seeds);

        ui->table->setUpdatesEnabled(true);
        ui->table->setSortingEnabled(true);

        //ui->table->resizeColumnsToContents();
        for (int i = 0, n = proxy->columnCount(); i < n; i++)
        {
            int id = proxy->headerData(i, Qt::Horizontal, Qt::UserRole).toInt();
            ui->table->setColumnWidth(i, colwidth[id]);
        }
        int rowheight = fm.height() + 4;
        for (int i = 0, n = proxy->rowCount(); i < n; i++)
            ui->table->setRowHeight(i, rowheight);
        qbufs.clear();
    }

    if (!qbufl.empty())
    {
        ui->treeLocate->setSortingEnabled(false);
        ui->treeLocate->setUpdatesEnabled(false);
        ui->treeLocate->addTopLevelItems(qbufl);
        ui->treeLocate->setUpdatesEnabled(true);
        ui->treeLocate->setSortingEnabled(true);
        qbufl.clear();
    }

    QString progress = QString::asprintf(" (%d/%d)", thread.idx.load(), thread.seeds.size());
    ui->pushStart->setText(tr("Stop") + progress);

    QApplication::processEvents(); // force processing of events so we can time correctly

    t += elapsed.elapsed();
    if (8*t > updt)
        updt = 4*t;
    nextupdate = elapsed.nsecsElapsed() + 1e6 * updt;
}

void TabBiomes::on_pushStart_clicked()
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

    thread.dims[0] = ui->checkOverworld->isChecked() ? DIM_OVERWORLD : DIM_UNDEF;
    thread.dims[1] = ui->checkNether->isChecked() ? DIM_NETHER : DIM_UNDEF;
    thread.dims[2] = ui->checkEnd->isChecked() ? DIM_END : DIM_UNDEF;

    int x1 = ui->lineX1->text().toInt();
    int z1 = ui->lineZ1->text().toInt();
    int x2 = ui->lineX2->text().toInt();
    int z2 = ui->lineZ2->text().toInt();
    if (x2 < x1) std::swap(x1, x2);
    if (z2 < z1) std::swap(z1, z2);

    int s = ui->comboScale->currentIndex() * 2; // combo index matches a power of 4 scale
    int scale = 1 << s;

    if (ui->radioFullSample->isChecked())
    {
        thread.dat.samples = ~0ULL;
    }
    else
    {
        thread.dat.samples = ui->lineSamples->text().toULongLong();
        scale = 4;
        s = 2;
    }

    if (ui->tabWidget->currentWidget() == ui->tabLocate)
    {
        //ui->treeWidget->clear();
        ui->treeLocate->setSortingEnabled(false);
        while (ui->treeLocate->topLevelItemCount() > 0)
            delete ui->treeLocate->takeTopLevelItem(0);
        ui->treeLocate->setSortingEnabled(true);
        thread.dat.locate = str2biome[ui->comboBiome->currentText()];
        thread.minsize = ui->lineBiomeSize->text().toInt();
        thread.tolerance = ui->lineTolerance->text().toInt();
        if (thread.minsize <= 0)
            thread.minsize = 1;
        scale = 4;
        s = 2;
    }
    else
    {
        model->reset(thread.wi.mc);
        thread.dat.locate = -1;
    }

    thread.dat.scale = scale;
    thread.dat.x1 = x1 >> s;
    thread.dat.z1 = z1 >> s;
    thread.dat.x2 = x2 >> s;
    thread.dat.z2 = z2 >> s;

    if (thread.dat.locate < 0)
        dats = thread.dat;
    else
        datl = thread.dat;

    ui->pushExport->setEnabled(false);
    ui->pushStart->setChecked(true);
    ui->pushStart->setText(tr("Stop"));
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

void TabBiomes::on_pushExport_clicked()
{
    QString fnam = QFileDialog::getSaveFileName(
        this, tr("Export biome analysis"), parent->prevdir, tr("Text files (*.txt *csv);;Any files (*)"));
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

    if (ui->tabWidget->currentWidget() == ui->tabStats)
    {
        stream << qte << "#X1" << sep << dats.x1 << sep << "(" << (dats.x1*dats.scale) << ")" << qte << "\n";
        stream << qte << "#Z1" << sep << dats.z1 << sep << "(" << (dats.z1*dats.scale) << ")" << qte << "\n";
        stream << qte << "#X2" << sep << dats.x2 << sep << "(" << (dats.x2*dats.scale) << ")" << qte << "\n";
        stream << qte << "#Z2" << sep << dats.z2 << sep << "(" << (dats.z2*dats.scale) << ")" << qte << "\n";
        stream << qte << "#scale" << sep << "1:" << dats.scale << qte << "\n";
        if (dats.samples != ~0ULL)
            stream << qte << "#samples" << sep << dats.samples << qte << "\n";

        QStringList header = { tr("seed") };
        for (int col = 0, ncol = proxy->columnCount(); col < ncol; col++)
            header.append(proxy->headerData(col, Qt::Horizontal).toString());
        csvline(stream, qte, sep, header);

        for (int row = 0, nrow = proxy->rowCount(); row < nrow; row++)
        {
            QStringList cols;
            cols.append(proxy->headerData(row, Qt::Vertical).toString());
            for (int col = 0, ncol = proxy->columnCount(); col < ncol; col++)
            {
                QString cntstr = proxy->data(proxy->index(row, col)).toString();
                cols.append(cntstr == "" ? "0" : cntstr);
            }
            csvline(stream, qte, sep, cols);
        }
    }
    else if (ui->tabWidget->currentWidget() == ui->tabLocate)
    {
        stream << qte << "#X1" << sep << datl.x1 << sep << "(" << (datl.x1*datl.scale) << ")" << qte << "\n";
        stream << qte << "#Z1" << sep << datl.z1 << sep << "(" << (datl.z1*datl.scale) << ")" << qte << "\n";
        stream << qte << "#X2" << sep << datl.x2 << sep << "(" << (datl.x2*datl.scale) << ")" << qte << "\n";
        stream << qte << "#Z2" << sep << datl.z2 << sep << "(" << (datl.z2*datl.scale) << ")" << qte << "\n";
        stream << qte << "#scale" << sep << "1:" << datl.scale << qte << "\n";
        stream << qte << "#biome" << sep << biome2str(MC_NEWEST, datl.locate) << qte << "\n";

        QStringList header = { tr("seed"), tr("area"), tr("x"), tr("z") };
        csvline(stream, qte, sep, header);

        QTreeWidgetItemIterator it(ui->treeLocate);
        QString seed;
        for (; *it; ++it)
        {
            QTreeWidgetItem *item = *it;
            if (item->text(0) != "-")
            {
                seed = item->text(0);
                continue;
            }
            QStringList cols;
            cols.append(seed);
            cols.append(item->text(1));
            cols.append(item->text(2));
            cols.append(item->text(3));
            csvline(stream, qte, sep, cols);
        }
    }
}

void TabBiomes::on_table_doubleClicked(const QModelIndex &index)
{
    uint64_t seed = model->seeds[index.column()];
    int id = model->ids[index.row()];
    WorldInfo wi;
    parent->getSeed(&wi);
    wi.seed = seed;
    parent->setSeed(wi, getDimension(id));
}

void TabBiomes::on_buttonFromVisible_clicked()
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

void TabBiomes::on_radioFullSample_toggled(bool checked)
{
    ui->comboScale->setEnabled(checked);
    ui->lineSamples->setEnabled(!checked);
}

void TabBiomes::on_lineBiomeSize_textChanged(const QString &text)
{
    double area = text.toInt();
    ui->labelBiomeSize->setText(QString::asprintf("(%g sq. chunks)", area / 16));
}

void TabBiomes::on_treeLocate_itemClicked(QTreeWidgetItem *item, int column)
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

void TabBiomes::on_tabWidget_currentChanged(int)
{
    bool ok = false;
    if (!thread.isRunning())
    {
        if (ui->tabWidget->currentWidget() == ui->tabStats)
            ok = !model->ids.empty();
        if (ui->tabWidget->currentWidget() == ui->tabLocate)
            ok = ui->treeLocate->topLevelItemCount() > 0;
    }
    ui->pushExport->setEnabled(ok);
}
