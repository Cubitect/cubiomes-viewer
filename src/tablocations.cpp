#include "tablocations.h"
#include "ui_tablocations.h"

#include "config.h"
#include "message.h"
#include "util.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QIntValidator>
#include <QTextStream>

#include <algorithm>
#include <random>
#include <vector>

#define SAMPLES_MAX 99999999

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

        QString summary = c.summary(false);
        item->setText(0, summary.mid(1, 2));
        item->setText(1, summary.mid(5));

        if ((p.x == -1 && p.z == -1) || c.type == F_LOGIC_NOT)
            posval = false;
        if (posval)
        {
            const FilterInfo& finfo = g_filterinfo.list[c.type];
            double dist = sqrt((double)p.x*p.x + (double)p.z*p.z);
            item->setText(2, QString::number(p.x));
            item->setText(3, QString::number(p.z));
            item->setText(4, QString::asprintf("%.0f", dist));
            item->setData(0, Qt::UserRole+0, QVariant::fromValue(seed));
            item->setData(0, Qt::UserRole+1, QVariant::fromValue(finfo.dim));
            item->setData(0, Qt::UserRole+2, QVariant::fromValue(p));
            item->setData(0, Qt::UserRole+3, QVariant::fromValue(node));
        }
    }
    if (!branches.empty())
    {
        for (char b : branches)
            setConditionTreeItems(ctree, b, seed, cpos, item, posval);
    }
    return item;
}

QString AnalysisLocations::set(WorldInfo wi, const std::vector<Condition>& conds)
{
    this->wi = wi;
    QString err = condtree.set(conds, wi.mc);
    if (err.isEmpty())
        err = env.init(wi.mc, wi.large, condtree);
    return err;
}

void AnalysisLocations::run()
{
    stop = false;

    for (sidx = 0; sidx < (long)seeds.size(); sidx++, pidx = 0) // update sidx and pidx together
    {
        uint64_t seed = seeds[sidx.load()];
        env.setSeed(seed);

        for (; pidx < (long)pos.size(); pidx++)
        {
            if (stop) return;

            Pos at = pos[pidx.load()];
            Pos cpos[MAX_INSTANCES] = {};
            if (testTreeAt(at, &env, PASS_FULL_64, &stop, cpos)
                != COND_OK)
            {
                continue;
            }

            double dist = sqrt((double)at.x*at.x + (double)at.z*at.z);

            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, tr("@"));
            item->setText(1, QString::asprintf("%" PRId64, seed));
            item->setText(2, QString::number(at.x));
            item->setText(3, QString::number(at.z));
            item->setText(4, QString::asprintf("%.0f", dist));
            item->setData(0, Qt::UserRole+0, QVariant::fromValue(seed));
            item->setData(0, Qt::UserRole+1, QVariant::Invalid);
            item->setData(0, Qt::UserRole+2, QVariant::fromValue(at));

            setConditionTreeItems(condtree, 0, seed, cpos, item, true);
            emit itemDone(item);
        }
    }
}


enum { SMODE_RADIAL_GRID, SMODE_SQUARE_SPIRAL, SMODE_NORMAL };

TabLocations::TabLocations(MainWindow *parent)
    : QWidget(parent)
    , ui(new Ui::TabLocations)
    , parent(parent)
    , thread()
    , timer()
    , maxresults(1)
    , nextupdate()
    , updt(20)
{
    ui->setupUi(this);
    ui->treeWidget->setSortingEnabled(false); // sortable triggers are not necessary

    ui->comboSampling->addItem(tr("Lattice points in radial order") + ", ||α·n, α·m|| < r", SMODE_RADIAL_GRID);
    ui->comboSampling->addItem(tr("Square spiral") + ", (α·n, α·m)", SMODE_SQUARE_SPIRAL);
    ui->comboSampling->addItem(tr("Random Gaussian samples") + ", (α·norm(), α·norm())", SMODE_NORMAL);

    ui->lineN->setValidator(new QIntValidator(1, SAMPLES_MAX, this));
    ui->lineA->setValidator(new QIntValidator(1, (int)3e7, this));
    ui->lineX->setValidator(new QIntValidator((int)-3e7, (int)3e7, this));
    ui->lineZ->setValidator(new QIntValidator((int)-3e7, (int)3e7, this));

    connect(&thread, &AnalysisLocations::itemDone, this, &TabLocations::onAnalysisItemDone, Qt::BlockingQueuedConnection);
    connect(&thread, &AnalysisLocations::finished, this, &TabLocations::onAnalysisFinished);

    connect(&timer, &QTimer::timeout, this, QOverload<>::of(&TabLocations::onProgressTimeout));
}

TabLocations::~TabLocations()
{
    timer.stop();
    thread.stop = true;
    thread.wait(500);
    delete ui;
}

bool TabLocations::event(QEvent *e)
{
    if (e->type() == QEvent::LayoutRequest)
    {
        QFontMetrics fm = QFontMetrics(ui->treeWidget->font());
        ui->treeWidget->setColumnWidth(0, txtWidth(fm) * 9);
        ui->treeWidget->setColumnWidth(1, txtWidth(fm) * 36);
        ui->treeWidget->setColumnWidth(2, txtWidth(fm) * 9);
        ui->treeWidget->setColumnWidth(3, txtWidth(fm) * 9);
        ui->treeWidget->setColumnWidth(4, txtWidth(fm) * 9);
    }
    return QWidget::event(e);
}

void TabLocations::save(QSettings& settings)
{
    settings.setValue("analysis/seedsrc_loc", ui->comboSeedSource->currentIndex());
    settings.setValue("analysis/samplemode", ui->comboSampling->currentData().toInt());
    settings.setValue("analysis/samplecnt", ui->lineN->text().toInt());
    settings.setValue("analysis/samplesep", ui->lineA->text().toInt());
    settings.setValue("analysis/offx", ui->lineX->text().toInt());
    settings.setValue("analysis/offz", ui->lineZ->text().toInt());
}

static void loadLine(QSettings *s, QLineEdit *line, const char *key)
{
    qlonglong x = line->text().toLongLong();
    line->setText( QString::number(s->value(key, x).toLongLong()) );
}
void TabLocations::load(QSettings& settings)
{
    loadLine(&settings, ui->lineN, "analysis/samplecnt");
    loadLine(&settings, ui->lineA, "analysis/samplesep");
    loadLine(&settings, ui->lineX, "analysis/offx");
    loadLine(&settings, ui->lineZ, "analysis/offz");
    QVariant mode;
    mode = settings.value("analysis/samplemode", ui->comboSampling->currentData());
    ui->comboSampling->setCurrentIndex(ui->comboSampling->findData(mode));
    mode = settings.value("analysis/seedsrc_loc", ui->comboSeedSource->currentIndex());
    ui->comboSeedSource->setCurrentIndex(mode.toInt());
    maxresults = settings.value("config/maxMatching", maxresults).toInt();
}

void TabLocations::onAnalysisItemDone(QTreeWidgetItem *item)
{
    if (qbuf.size() + ui->treeWidget->topLevelItemCount() >= maxresults)
    {
        thread.stop = true;
    }

    qbuf.push_back(item);
    quint64 ns = elapsed.nsecsElapsed();
    if (ns > nextupdate)
    {
        nextupdate = ns + updt * 1e6;
        QTimer::singleShot(updt, this, &TabLocations::onBufferTimeout);
    }
}

void TabLocations::onAnalysisFinished()
{
    timer.stop();
    onBufferTimeout();
    ui->pushExport->setEnabled(ui->treeWidget->topLevelItemCount() > 0);
    ui->pushStart->setChecked(false);
    ui->pushStart->setText(tr("Analyze"));
    ui->labelStatus->setText(tr("Idle"));
    if (parent)
        parent->setProgressIndication();
}

void TabLocations::onBufferTimeout()
{
    uint64_t t = -elapsed.elapsed();

    if (!qbuf.empty())
    {
        ui->treeWidget->setUpdatesEnabled(false);
        ui->treeWidget->addTopLevelItems(qbuf);
        ui->treeWidget->setUpdatesEnabled(true);
        qbuf.clear();
        onProgressTimeout();
    }

    QApplication::processEvents(); // force processing of events so we can time correctly

    t += elapsed.elapsed();
    if (8*t > updt)
        updt = 4*t;
    nextupdate = elapsed.nsecsElapsed() + 1e6 * updt;
}

void TabLocations::onProgressTimeout()
{
    size_t total = thread.seeds.size() * thread.pos.size();
    size_t progress = thread.sidx.load() * thread.pos.size() + thread.pidx.load();
    double frac = progress / (double)total;
    ui->labelStatus->setText(QString::asprintf("%zu / %zu (%.1f%%)", progress, total, 100.0*frac));
    if (parent)
        parent->setProgressIndication(frac);
}

void TabLocations::on_pushStart_clicked()
{
    if (thread.isRunning())
    {
        thread.stop = true;
        return;
    }
    updt = 20;
    nextupdate = 0;
    elapsed.start();

    thread.pos.clear();
    int mode = ui->comboSampling->currentData().toInt();
    int a = ui->lineA->text().toInt();
    uint64_t n = ui->lineN->text().toULongLong();
    int x0 = ui->lineX->text().toInt();
    int z0 = ui->lineZ->text().toInt();

    if (a == 0)
        return;
    if (n > SAMPLES_MAX)
        return;

    WorldInfo wi;
    parent->getSeed(&wi);
    std::vector<Condition> conds = parent->formCond->getConditions();

    QString err = thread.set(wi, conds);
    if (!err.isEmpty())
    {
        warn(this, err);
        return;
    }

    thread.sidx = thread.pidx = 0;

    thread.seeds.clear();
    if (ui->comboSeedSource->currentIndex() == 0)
        thread.seeds.push_back(thread.wi.seed);
    else
        thread.seeds = parent->formControl->getResults();

    if (mode == SMODE_RADIAL_GRID)
    {
        QElapsedTimer t; t.start();
        struct P {
            int x, z;
            float rsq;
            bool operator< (const P& x) const { return rsq < x.rsq; }
        };
        std::vector<P> v;
        float rsqmax = n / M_PI;
        int r = (int) sqrt(rsqmax);

        v.reserve((size_t)r * (r + 1) / 2);

        for (int x = 1; x <= r; x++)
        {
            float x_sq = (float) x * x;
            for (int z = 0; z <= x; z++)
            {
                float rsq = x_sq + (float) z * z;
                if (rsq <= rsqmax)
                    v.push_back(P{x, z, rsq});
            }
        }
        std::stable_sort(v.begin(), v.end());
        thread.pos.push_back(Pos{0,0});

        for (uint64_t i = 0; i < v.size(); i++)
        {
            int x = a * v[i].x;
            int z = a * v[i].z;

            if (z == 0 || x == z)
            {
                thread.pos.push_back(Pos{ x0+x, z0+z });
                thread.pos.push_back(Pos{ x0-z, z0+x });
                thread.pos.push_back(Pos{ x0-x, z0-z });
                thread.pos.push_back(Pos{ x0+z, z0-x });
            }
            else
            {
                thread.pos.push_back(Pos{ x0+x, z0+z });
                thread.pos.push_back(Pos{ x0+z, z0+x });
                thread.pos.push_back(Pos{ x0-z, z0+x });
                thread.pos.push_back(Pos{ x0-x, z0+z });
                thread.pos.push_back(Pos{ x0-x, z0-z });
                thread.pos.push_back(Pos{ x0-z, z0-x });
                thread.pos.push_back(Pos{ x0+z, z0-x });
                thread.pos.push_back(Pos{ x0+x, z0-z });
            }
            if (thread.pos.size() >= n)
            {
                thread.pos.resize(n);
                break;
            }
        }
        //qDebug() << thread.pos.size() << ":" << thread.pos.back().x << thread.pos.back().z << " t=" << t.elapsed();
    }
    else if (mode == SMODE_SQUARE_SPIRAL)
    {
        int rx = 0;
        int rz = 0;
        int i = 0, dl = 1;
        int dx = 1, dz = 0;
        for (uint64_t j = 0; j < n; j++)
        {
            Pos p = { a*rx + x0, a*rz + z0 };
            thread.pos.push_back(p);
            rx += dx;
            rz += dz;
            if (++i == dl)
            {
                i = 0;
                int tmp = dx;
                dx = -dz;
                dz = tmp;
                if (dz == 0)
                    dl++;
            }
        }
    }
    else if (mode == SMODE_NORMAL)
    {
        thread_local RandGen rng;
        std::normal_distribution<double> norm = std::normal_distribution<double>(0.0, 1.0);

        for (uint64_t j = 0; j < n; j++)
        {
            int x = (int) round( a * norm(rng.mt) );
            int z = (int) round( a * norm(rng.mt) );
            Pos p = { x + x0, z + z0 };
            thread.pos.push_back(p);
        }
    }

    //ui->treeWidget->setSortingEnabled(false);
    while (ui->treeWidget->topLevelItemCount() > 0)
        delete ui->treeWidget->takeTopLevelItem(0);

    ui->pushExport->setEnabled(false);
    ui->pushStart->setChecked(true);
    ui->pushStart->setText(tr("Stop"));
    onProgressTimeout();
    thread.start();
    timer.start(250);
}

void TabLocations::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    (void) column;
    QVariant dat;
    dat = item->data(0, Qt::UserRole+0);
    if (!dat.isValid())
        return;

    uint64_t seed = qvariant_cast<uint64_t>(dat);
    dat = item->data(0, Qt::UserRole+1);
    int dim = dat.isValid() ? dat.toInt() : DIM_UNDEF;

    QVariant at = item->data(0, Qt::UserRole+2);

    std::vector<Shape> shapes;
    const std::vector<Condition>& conds = thread.condtree.condvec;
    int node = item->data(0, Qt::UserRole+3).toInt();

    if (node >= 0 || node < (int)conds.size())
    {
        const Condition& c = conds[node];
        const FilterInfo& ft = g_filterinfo.list[c.type];
        Shape s;
        s.dim = ft.dim;
        if (c.rmax)
        {
            s.type = Shape::CIRCLE;
            s.p1 = s.p2 = Pos{0,0};
            s.r = c.rmax - 1;
        }
        else
        {
            s.type = Shape::RECT;
            s.p1 = Pos{c.x1, c.z1};
            s.p2 = Pos{c.x2+1, c.z2+1};
            s.r = 0;
        }
        if (QTreeWidgetItem *conditem = item->parent())
        {
            Pos pos = qvariant_cast<Pos>(conditem->data(0, Qt::UserRole+2));
            s.p1.x += pos.x;
            s.p1.z += pos.z;
            s.p2.x += pos.x;
            s.p2.z += pos.z;
        }
        shapes.push_back(s);
    }

    WorldInfo wi;
    parent->getSeed(&wi, false);
    if (wi.seed != seed || (dim != DIM_UNDEF && dim != parent->getDim()))
    {
        wi.seed = seed;
        parent->getMapView()->deleteWorld();
    }
    if (at.isValid())
    {
        Pos p = qvariant_cast<Pos>(at);
        parent->getMapView()->setView(p.x+0.5, p.z+0.5);
    }
    parent->setSeed(wi, dim);
    parent->formCond->clearSelection();
    parent->getMapView()->setShapes(shapes);
}

void TabLocations::on_pushExpand_clicked()
{
    bool expand = false;
    for (QTreeWidgetItemIterator it(ui->treeWidget); *it; ++it)
        if (!(*it)->isExpanded())
            expand = true;
    if (expand)
        ui->treeWidget->expandAll();
    else
        ui->treeWidget->collapseAll();
}

static void csvline(QTextStream& stream, const QString& qte, const QString& sep, QStringList& cols)
{
    if (qte.isEmpty())
    {
        for (QString& s : cols)
            if (s.contains(sep))
                s = "\"" + s + "\"";
    }
    stream << qte << cols.join(sep) << qte << "\n";
}

void TabLocations::exportResults(QTextStream& stream)
{
    QString qte = parent->config.quote;
    QString sep = parent->config.separator;

    stream << "Sep=" + sep + "\n";
    sep = qte + sep + qte;

    QStringList header = { tr("id"), tr("seed/condition"), tr("x"), tr("z"), tr("distance") };
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
    stream.flush();
}

void TabLocations::on_pushExport_clicked()
{
#if WASM
    QByteArray content;
    QTextStream stream(&content);
    exportResults(stream);
    QFileDialog::saveFileContent(content, "locations.csv");
#else
    QString fnam = QFileDialog::getSaveFileName(
        this, tr("Export locations"), parent->prevdir, tr("Text files (*.txt *csv);;Any files (*)"));
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

    QTextStream stream(&file);
    exportResults(stream);
#endif
}

