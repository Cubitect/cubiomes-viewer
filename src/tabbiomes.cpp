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

    for (int64_t seed : qAsConst(seeds))
    {
        if (stop) break;
        wi.seed = seed;
        if (locate < 0)
            runStatistics(&g);
        else
            runLocate(&g);
    }
}

void AnalysisBiomes::runStatistics(Generator *g)
{
    QVector<uint64_t> idcnt(256);
    int w = x2 - x1 + 1;
    int h = z2 - z1 + 1;
    uint64_t n = w * (uint64_t)h;

    for (int d = 0; d < 3; d++)
    {
        if (dims[d] == DIM_UNDEF)
            continue;
        applySeed(g, dims[d], wi.seed);

        if (samples >= n)
        {   // full area gen => generate 512x512 areas at a time
            const int step = 512;
            for (int x = x1; x <= x2 && !stop; x += step)
            {
                for (int z = z1; z <= z2 && !stop; z += step)
                {
                    int w = x2-x+1 < step ? x2-x+1 : step;
                    int h = z2-z+1 < step ? z2-z+1 : step;
                    Range r = {scale, x, z, w, h, wi.y, 1};
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

            if (samples * 2 >= n)
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
                order.resize(samples);
            }
            else
            {   // sparse regime => fill randomly without reuse
                std::unordered_set<uint64_t> used;
                order.reserve(samples);
                used.reserve(samples);
                for (uint64_t i = 0; order.size() < samples; i++)
                {
                    if (!(i & 0xffff) && stop)
                        break;
                    uint64_t idx = getRnd64() % n;
                    auto it = used.insert(idx);
                    if (it.second)
                        order.push_back(idx);
                }
            }

            for (uint64_t i = 0; i < samples && !stop; i++)
            {
                uint64_t idx = order[i];
                int x = (int) (idx % w);
                int z = (int) (idx / w);
                int id = getBiomeAt(g, scale, x1+x, wi.y, z1+z);
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
    Range r = {4, x1, z1, x2-x1+1, z2-z1+1, wi.y, 1};
    int n = getBiomeCenters(
        pos, siz, MAX_LOCATE, g, r, locate, minsize, tolerance,
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
    int id = ids[index.row()];
    int col = index.column();
    uint64_t seed = seeds[col];
    return cnt[id][seed];
}

QVariant BiomeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || section < 0)
        return QVariant::Invalid;
    if (orientation == Qt::Horizontal)
    {
        if  (section < seeds.size())
            return QVariant::fromValue(seeds[section]);
    }
    else
    {
        if (section < ids.size())
            return QVariant::fromValue(QString(biome2str(cmp.mc, ids[section])));
    }
    return QVariant::Invalid;
}

int BiomeTableModel::insertId(int id)
{
    QList<int>::iterator it = std::lower_bound(ids.begin(), ids.end(), id, cmp);
    if (it != ids.end() && *it == id)
        return -1;
    int row = std::distance(ids.begin(), it);
    //beginInsertRows(QModelIndex(), row, row);
    ids.insert(row, id);
    //endInsertRows();
    return row;
}

int BiomeTableModel::insertSeed(uint64_t seed)
{
    int col = seeds.size();
    //beginInsertColumns(QModelIndex(), col, col);
    seeds.append(seed);
    //endInsertColumns();
    return col;
}

void BiomeTableModel::reset(int mc)
{
    beginRemoveRows(QModelIndex(), 0, ids.size());
    ids.clear();
    endRemoveRows();
    beginRemoveColumns(QModelIndex(), 0, seeds.size());
    seeds.clear();
    endRemoveColumns();
    cnt.clear();
    cmp.mode = IdCmp::SORT_LEX;
    cmp.dim = DIM_UNDEF;
    cmp.mc = mc;
}


TabBiomes::TabBiomes(MainWindow *parent)
    : QWidget(parent)
    , ui(new Ui::TabBiomes)
    , parent(parent)
    , thread()
    , model(new BiomeTableModel(this))
    , proxy(new BiomeSortProxy(this))
{
    ui->setupUi(this);

    proxy->setSourceModel(model);
    ui->table->setModel(proxy);

    QHeaderView *header = ui->table->horizontalHeader();
    connect(header, &QHeaderView::sortIndicatorChanged, this, &TabBiomes::onSort);

    QFont font = QFont("monospace", 9);
    ui->table->setFont(font);
    ui->table->setSortingEnabled(true);
    ui->table->sortByColumn(-1, Qt::AscendingOrder);

    ui->treeWidget->setColumnWidth(0, 160);
    ui->treeWidget->setColumnWidth(1, 120);
    ui->treeWidget->sortByColumn(0, Qt::AscendingOrder);

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

    QStringList bnames;
    for (auto& it : str2biome)
        bnames.append(it.first);
    QRegularExpressionValidator *reval = new QRegularExpressionValidator(
        QRegularExpression("(" + bnames.join("|") + ")"), this
    );
    ui->comboBiome->lineEdit()->setValidator(reval);
}

TabBiomes::~TabBiomes()
{
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

void TabBiomes::onAnalysisSeedDone(uint64_t seed, QVector<uint64_t> idcnt)
{
    // save state of table UI
    int selr = ui->table->selectionModel()->currentIndex().row();
    int selc = ui->table->selectionModel()->currentIndex().column();
    int posr = ui->table->verticalScrollBar()->value();
    int posc = ui->table->horizontalScrollBar()->value();

    int ncol = proxy->columnCount();
    std::vector<int> colwidth(ncol, 60);
    for (int c = 0; c < ncol; c++)
        colwidth[c] = ui->table->columnWidth(c);

    // create new model
    ui->table->setSortingEnabled(false);
    ui->table->setModel(nullptr);
    BiomeTableModel *m_new = new BiomeTableModel(this);

    m_new->ids = model->ids;
    m_new->seeds = model->seeds;
    m_new->cnt = model->cnt;

    if (m_new->insertSeed(seed) < selc)
        selc++;

    for (int id = 0; id < 256; id++)
    {
        if (idcnt[id] == 0)
            continue;
        int r = m_new->insertId(id);
        if (r >= 0 && r < selr)
            selr++;
        m_new->cnt[id][seed] = QVariant::fromValue(idcnt[id]);
    }

    delete model;
    model = m_new;

    // restore state of table UI for new model
    proxy->setSourceModel(model);
    ui->table->setModel(proxy);
    ui->table->setSortingEnabled(true);

    for (int c = 0; c < ncol; c++)
        ui->table->setColumnWidth(c, colwidth[c]);
    ui->table->resizeColumnToContents(ncol);

    int rowheight = QFontMetrics(ui->table->font()).height() + 4;
    for (int r = 0, nrow = proxy->rowCount(); r < nrow; r++)
        ui->table->setRowHeight(r, rowheight);

    ui->table->selectionModel()->setCurrentIndex(proxy->index(selr, selc), QItemSelectionModel::SelectCurrent);
    ui->table->verticalScrollBar()->setValue(posr);
    ui->table->horizontalScrollBar()->setValue(posc);


    QString progress = QString::asprintf(" (%d/%d)", model->seeds.size()+1, thread.seeds.size());
    ui->pushStart->setText(tr("Stop") + progress);
}

void TabBiomes::onAnalysisSeedItem(QTreeWidgetItem *item)
{
    ui->treeWidget->addTopLevelItem(item);

    QString progress = QString::asprintf(" (%d/%d)", ui->treeWidget->topLevelItemCount()+1, thread.seeds.size());
    ui->pushStart->setText(tr("Stop") + progress);
}

void TabBiomes::onAnalysisFinished()
{
    ui->pushExport->setEnabled(!model->ids.empty() || ui->treeWidget->topLevelItemCount());
    ui->pushStart->setChecked(false);
    ui->pushStart->setText(tr("Analyze"));
}

void TabBiomes::onSort(int column, Qt::SortOrder)
{
    QHeaderView *header = ui->table->horizontalHeader();

    if (proxy->order == Qt::DescendingOrder && proxy->column == column)
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

void TabBiomes::on_pushStart_clicked()
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
        thread.samples = ~0ULL;
    }
    else
    {
        thread.samples = ui->lineSamples->text().toULongLong();
        scale = 4;
        s = 2;
    }

    if (ui->tabWidget->currentWidget() == ui->tabLocate)
    {
        ui->treeWidget->clear();
        thread.locate = str2biome[ui->comboBiome->currentText()];
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
        thread.locate = -1;
    }

    thread.scale = scale;
    thread.x1 = x1 >> s;
    thread.z1 = z1 >> s;
    thread.x2 = x2 >> s;
    thread.z2 = z2 >> s;

    ui->pushExport->setEnabled(false);
    ui->pushStart->setChecked(true);
    ui->pushStart->setText(tr("Stop"));
    thread.start();
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

    QTextStream stream(&file);
    stream << "#X1; " << thread.x1 << "; (" << (thread.x1*thread.scale) << ")\n";
    stream << "#Z1; " << thread.z1 << "; (" << (thread.z1*thread.scale) << ")\n";
    stream << "#X2; " << thread.x2 << "; (" << (thread.x2*thread.scale) << ")\n";
    stream << "#Z2; " << thread.z2 << "; (" << (thread.z2*thread.scale) << ")\n";
    stream << "#scale; 1:" << thread.scale << "\n";

    if (thread.locate < 0)
    {
        if (thread.samples != ~0ULL)
            stream << "#samples; " << thread.samples << "\n";

        QList<QString> header;
        header.append(tr("biome\\seed"));
        for (int col = 0, ncol = proxy->columnCount(); col < ncol; col++)
            header.append(proxy->headerData(col, Qt::Horizontal).toString());
        stream << header.join("; ") << "\n";

        for (int row = 0, nrow = proxy->rowCount(); row < nrow; row++)
        {
            QList<QString> entries;
            entries.append(proxy->headerData(row, Qt::Vertical).toString());
            for (int col = 0, ncol = proxy->columnCount(); col < ncol; col++)
            {
                QString cntstr = proxy->data(proxy->index(row, col)).toString();
                entries.append(cntstr == "" ? "0" : cntstr);
            }
            stream << entries.join("; ") << "\n";
        }
    }
    else
    {
        stream << "#biome; " << biome2str(MC_NEWEST, thread.locate) << "\n";
        stream << "seed; area; x; z;\n";
        QTreeWidgetItemIterator it(ui->treeWidget);
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
            stream << cols.join(";") << "\n";
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

void TabBiomes::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
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

