#include "exportdialog.h"
#include "ui_exportdialog.h"

#include "mainwindow.h"
#include "mapview.h"
#include "message.h"
#include "util.h"

#include <QFileDialog>
#include <QIntValidator>
#include <QProgressDialog>
#include <QRunnable>
#include <QSettings>

#if WASM
#include "qzipwriter.h"
#include <QBuffer>
#endif

void ExportWorker::runWorkItem(const ExportWorkItem& work)
{
    const ExportDialog *mt = parent;
    if (mt->stop)
        return;

    Generator g;
    setupGenerator(&g, mt->wi.mc, mt->wi.large | FORCE_OCEAN_VARIANTS);
    applySeed(&g, mt->dim, work.seed);
    Range r = {mt->scale, mt->x, mt->z, mt->w, mt->h, mt->y, 1};

    if (mt->tilesize > 0)
    {
        r.x = work.tx * mt->tilesize;
        r.z = work.tz * mt->tilesize;
        r.sx = r.sz = mt->tilesize;
    }

    int *ids = allocCache(&g, r);
    genBiomes(&g, ids, r);

    uchar *rgb = new uchar[r.sx * r.sz * 3];
    biomesToImage(rgb, g_biomeColors, ids, r.sx, r.sz, 1, 1);

    if (mt->heightvis >= 0 && mt->dim == DIM_OVERWORLD)
    {
        SurfaceNoise sn;
        initSurfaceNoise(&sn, DIM_OVERWORLD, mt->wi.seed);
        int stepbits = mt->scale == 1 ? 2 : 0;
        applyHeightShading(rgb, r, &g, &sn, stepbits, mt->heightvis, true, &mt->stop);
    }

    QImage img(rgb, r.sx, r.sz, 3*r.sx, QImage::Format_RGB888);

    enum { BG_NONE, BG_TRANSP, BG_BLACK };
    if (mt->tilesize > 0 && mt->bgmode != BG_NONE)
    {   // TODO: only generate needed sections
        QColor bg = QColor(Qt::black);
        if (mt->bgmode == BG_TRANSP)
        {
            bg = QColor(Qt::transparent);
            img = img.convertToFormat(QImage::Format_RGBA8888, Qt::AutoColor);
        }
        int zh = mt->z + mt->h;
        int xw = mt->x + mt->w;
        for (int j = 0; j < r.sz; j++)
        {
            for (int i = 0; i < r.sx; i++)
            {
                if (r.z+j < mt->z || r.z+j >= zh || r.x+i < mt->x || r.x+i >= xw)
                    img.setPixelColor(i, j, bg);
            }
        }
    }
    if (!mt->stop)
    {
        img.save(work.fnam);
    }
    free(ids);
    delete [] rgb;
}

void ExportWorker::run()
{
    ExportWorkItem work;
    while (parent->requestWork(&work))
    {
        runWorkItem(work);
        emit workItemDone();
    }
}

static void setCombo(QComboBox *cb, const char *setting)
{
    QSettings settings(APP_STRING, APP_STRING);
    int idx = settings.value(setting, cb->currentIndex()).toInt();
    if (idx < cb->count())
        cb->setCurrentIndex(idx);
}

ExportDialog::ExportDialog(MainWindow *parent)
    : QDialog(parent)
    , ui(new Ui::ExportDialog)
    , mainwindow(parent)
    , dir()
    , pattern()
    , wi()
    , dim()
    , scale()
    , x(), z(), w(), h(), y()
    , tilesize()
    , bgmode()
    , heightvis()
    , mutex()
    , workitems()
    , workers()
    , stop()
{
    ui->setupUi(this);

    for (int i = 0; i < QThread::idealThreadCount(); i++)
    {
        ExportWorker *worker = new ExportWorker(this);
        connect(worker, &ExportWorker::workItemDone, this, &ExportDialog::workItemDone, Qt::QueuedConnection);
        connect(worker, &ExportWorker::finished, this, &ExportDialog::onWorkerFinished, Qt::QueuedConnection);
        workers.push_back(worker);
    }

    QIntValidator *intval = new QIntValidator(this);
    ui->lineEditX1->setValidator(intval);
    ui->lineEditZ1->setValidator(intval);
    ui->lineEditX2->setValidator(intval);
    ui->lineEditZ2->setValidator(intval);

    connect(mainwindow, SIGNAL(mapUpdated()), this, SLOT(update()));
    connect(ui->lineEditX1, SIGNAL(editingFinished()), this, SLOT(update()));
    connect(ui->lineEditZ1, SIGNAL(editingFinished()), this, SLOT(update()));
    connect(ui->lineEditX2, SIGNAL(editingFinished()), this, SLOT(update()));
    connect(ui->lineEditZ2, SIGNAL(editingFinished()), this, SLOT(update()));
    connect(ui->comboSeed, SIGNAL(activated(int)), this, SLOT(update()));
    connect(ui->comboScale, SIGNAL(activated(int)), this, SLOT(update()));
    connect(ui->comboTileSize, SIGNAL(activated(int)), this, SLOT(update()));
    connect(ui->groupTiled, SIGNAL(toggled(bool)), this, SLOT(update()));

    QSettings settings(APP_STRING, APP_STRING);

    ui->lineDir->setText(settings.value("export/prevdir", mainwindow->prevdir).toString());
    ui->linePattern->setText(settings.value("export/pattern", "%S_%x_%z.png").toString());

    setCombo(ui->comboSeed, "export/seedIdx");
    setCombo(ui->comboScale, "export/scaleIdx");
    setCombo(ui->comboTileSize, "export/tileSizeIdx");
    setCombo(ui->comboBackground, "export/bgIdx");
    setCombo(ui->comboHeightVis, "export/heightvisIdx");

    ui->groupTiled->setChecked(settings.value("export/tiled", false).toBool());
    ui->lineEditX1->setText(settings.value("export/x0").toString());
    ui->lineEditZ1->setText(settings.value("export/z0").toString());
    ui->lineEditX2->setText(settings.value("export/x1").toString());
    ui->lineEditZ2->setText(settings.value("export/z1").toString());

#if WASM
    ui->labelDir->setVisible(false);
    ui->lineDir->setVisible(false);
    ui->buttonDirSelect->setVisible(false);
#endif

    update();
}

ExportDialog::~ExportDialog()
{
    cancel();
    for (ExportWorker *worker : qAsConst(workers))
    {
        worker->wait();
        delete worker;
    }
    delete ui;
}

bool ExportDialog::initWork(ExportWorkItem *work, uint64_t seed, int tx, int tz)
{
    work->seed = seed;
    work->tx = tx;
    work->tz = tz;
    work->fnam = pattern;
    work->fnam.replace("%S", QString::number((int64_t)seed));
    work->fnam.replace("%x", QString::number(tx));
    work->fnam.replace("%z", QString::number(tz));
#if WASM
    return false;
#else
    work->fnam = dir.filePath(work->fnam);
    return QFileInfo::exists(work->fnam);
#endif
}

bool ExportDialog::requestWork(ExportWorkItem *work)
{
    QMutexLocker locker(&mutex);
    if (workitems.empty())
        return false;
    *work = workitems.takeFirst();
    return true;
}

void ExportDialog::startWorkers()
{
    int threadlimit = mainwindow->config.mapThreads;
    int n = (int) workers.size();
    if (threadlimit && threadlimit < n)
        n = threadlimit;
    for (int i = 0; i < n; i++)
        workers[i]->start();
}

void ExportDialog::onWorkerFinished()
{
    for (ExportWorker *worker : qAsConst(workers))
        if (worker->isRunning())
            return;

#if WASM
    QByteArray content;
    QBuffer buffer(&content);
    buffer.open(QIODevice::WriteOnly);
    QZipWriter zipwriter(&buffer);
    zipwriter.setCompressionPolicy(QZipWriter::AutoCompress);

    for (const QString& fnam : qAsConst(paths))
    {
        QFile file(fnam);
        if (file.open(QFile::ReadOnly))
        {
            zipwriter.addFile(fnam, &file);
            file.close();
            QFile::remove(fnam);
        }
    }
    zipwriter.close();

    QFileDialog::saveFileContent(content, "images.zip");
#endif

    emit exportFinished();
}

void ExportDialog::update()
{
    int seedcnt = 1;

    if (ui->comboSeed->currentIndex() == 1)
    {
        const std::vector<uint64_t>& seeds = mainwindow->formControl->getResults();
        seedcnt = seeds.size();
    }

    ui->labelNumSeeds->setText(QString::number(seedcnt));

    int s = 2 * ui->comboScale->currentIndex();
    int x0 = ui->lineEditX1->text().toInt() >> s;
    int z0 = ui->lineEditZ1->text().toInt() >> s;
    int x1 = ui->lineEditX2->text().toInt() >> s;
    int z1 = ui->lineEditZ2->text().toInt() >> s;
    if (x0 > x1) std::swap(x0, x1);
    if (z0 > z1) std::swap(z0, z1);
    x1 += 1;
    z1 += 1;

    if (ui->groupTiled->isChecked())
    {
        int tilesize = ui->comboTileSize->currentText().section('x', 0, 0).toInt();
        int w = (int) ceil(x1 / (qreal)tilesize - (int) floor(x0 / (qreal)tilesize));
        int h = (int) ceil(z1 / (qreal)tilesize - (int) floor(z0 / (qreal)tilesize));
        int imgcnt = w * h * seedcnt;
        ui->labelImgSize->setText(ui->comboTileSize->currentText());
        ui->labelNumImg->setText(QString::number((w < 0 || h < 0) ? 0 : imgcnt));
    }
    else
    {
        int w = x1 > x0 ? x1 - x0 : 0;
        int h = z1 > z0 ? z1 - z0 : 0;
        ui->labelImgSize->setText(tr("%1x%2").arg(w).arg(h));
        ui->labelNumImg->setText(QString::number(seedcnt));
    }

    WorldInfo wi;
    mainwindow->getSeed(&wi, true);
    int dim = mainwindow->getDim();

    const char *p_mcs = mc2str(wi.mc);
    QString wgen = (p_mcs ? p_mcs : "");
    wgen += ", ";
    if (dim == 0) {
        wgen += tr("Overworld");
        if (wi.large) wgen += tr("/large");
    } else if (dim == -1) {
        wgen += tr("Nether");
    } else if (dim == +1) {
        wgen += tr("The End");
    }
    ui->labelWorldGen->setText(wgen);
    ui->labelY->setText(QString::number(wi.y));
}

void ExportDialog::on_buttonFromVisible_clicked()
{
    MapView *mapView = mainwindow->getMapView();

    qreal scale = mapView->getScale();
    qreal uiw = mapView->width() * scale;
    qreal uih = mapView->height() * scale;
    int bx0 = (int) floor(mapView->getX() - uiw/2);
    int bz0 = (int) floor(mapView->getZ() - uih/2);
    int bx1 = (int) ceil(mapView->getX() + uiw/2);
    int bz1 = (int) ceil(mapView->getZ() + uih/2);

    ui->lineEditX1->setText(QString::number(bx0));
    ui->lineEditZ1->setText(QString::number(bz0));
    ui->lineEditX2->setText(QString::number(bx1));
    ui->lineEditZ2->setText(QString::number(bz1));

    update();
}

void ExportDialog::on_buttonDirSelect_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(
            this, tr("Select Export Directory"),
            ui->lineDir->text(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty())
        return;
    ui->lineDir->setText(dir);
}

void ExportDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::StandardButton b = ui->buttonBox->standardButton(button);

    if (b == QDialogButtonBox::Ok)
    {
        for (ExportWorker *worker : qAsConst(workers))
            if (worker->isRunning())
                return;

        int seedmode = ui->comboSeed->currentIndex();
        QString pattern = ui->linePattern->text();
        bool tiled = ui->groupTiled->isChecked();

        WorldInfo wi;
        mainwindow->getSeed(&wi, true);

        std::vector<uint64_t> seeds;
        if (seedmode == 0)
            seeds.push_back(wi.seed);
        else if (seedmode == 1)
            seeds = mainwindow->formControl->getResults();

        if (seeds.size() > 1 && !pattern.contains("%S"))
        {
            warn(this, tr("When exporting more than one seed, the file pattern has to "
                          "include the \"%S\" format specifier for the seed number."));
            return;
        }

        if (tiled && (!pattern.contains("%x") || !pattern.contains("%z")))
        {
            warn(this, tr("Exporting as tiled images requires both the \"%x\" and \"%z\" "
                          "format specifiers in the file pattern, representing the tile coordinates."));
            return;
        }

        int s = 2 * ui->comboScale->currentIndex();
        int x0 = ui->lineEditX1->text().toInt() >> s;
        int z0 = ui->lineEditZ1->text().toInt() >> s;
        int x1 = ui->lineEditX2->text().toInt() >> s;
        int z1 = ui->lineEditZ2->text().toInt() >> s;
        if (x0 > x1) std::swap(x0, x1);
        if (z0 > z1) std::swap(z0, z1);
        x1 += 1;
        z1 += 1;
        int y = (s == 0 ? wi.y : wi.y >> 2);

        if (x1 <= x0 || z1 <= z0)
        {
            warn(this, tr("Invalid area."));
            return;
        }

        this->dir = QDir(ui->lineDir->text());
        this->pattern = pattern;
        this->wi = wi;
        this->dim = mainwindow->getDim();
        this->scale = 1 << s;
        this->x = x0;
        this->z = z0;
        this->w = x1 - x0;
        this->h = z1 - z0;
        this->y = y;
        this->tilesize = -1;
        this->heightvis = ui->comboHeightVis->currentIndex() - 1;
        this->bgmode = 0;
        this->paths.clear();
        bool existwarn = false;

        if (tiled)
        {
            int bgmode = ui->comboBackground->currentIndex();
            int tilesize = ui->comboTileSize->currentText().section('x', 0, 0).toInt();
            int tx0 = (int) floor(x0 / (qreal)tilesize);
            int tz0 = (int) floor(z0 / (qreal)tilesize);
            int tx1 = (int) ceil(x1 / (qreal)tilesize);
            int tz1 = (int) ceil(z1 / (qreal)tilesize);

            this->tilesize = tilesize;
            this->bgmode = bgmode;

            for (uint64_t seed : seeds)
            {
                for (int x = tx0; x < tx1; x++)
                {
                    for (int z = tz0; z < tz1; z++)
                    {
                        ExportWorkItem work;
                        existwarn |= initWork(&work, seed, x, z);
                        workitems.push_back(work);
                        paths.push_back(work.fnam);
                    }
                }
            }
        }
        else
        {
            int maxsiz = 0x8000;
            if (x1 - x0 >= maxsiz || z1 - z0 >= maxsiz)
            {
                int button = warn(this, tr("Warning"),
                        tr("Consider tiling very large images into smaller sections."),
                        tr("Continue?"), QMessageBox::Cancel | QMessageBox::Yes);
                if (button != QMessageBox::Yes)
                {
                    return;
                }
            }

            for (uint64_t seed : seeds)
            {
                ExportWorkItem work;
                existwarn |= initWork(&work, seed, 0, 0);
                workitems.push_back(work);
                paths.push_back(work.fnam);
            }
        }

        if (existwarn)
        {
            int button = warn(this, tr("Warning"),
                    tr("One or more of files already exist."),
                    tr("Continue and overwrite?"), QMessageBox::Cancel | QMessageBox::Yes);
            if (button != QMessageBox::Yes)
            {
                return;
            }
        }

        QSettings settings(APP_STRING, APP_STRING);
        settings.setValue("export/seedIdx", ui->comboSeed->currentIndex());
        settings.setValue("export/prevdir", ui->lineDir->text());
        settings.setValue("export/pattern", ui->linePattern->text());
        settings.setValue("export/scaleIdx", ui->comboScale->currentIndex());
        settings.setValue("export/x0", ui->lineEditX1->text());
        settings.setValue("export/z0", ui->lineEditZ1->text());
        settings.setValue("export/x1", ui->lineEditX2->text());
        settings.setValue("export/z1", ui->lineEditZ2->text());
        settings.setValue("export/tiled", ui->groupTiled->isChecked());
        settings.setValue("export/tileSizeIdx", ui->comboTileSize->currentIndex());
        settings.setValue("export/bgIdx", ui->comboBackground->currentIndex());
        settings.setValue("export/heightvisIdx", ui->comboHeightVis->currentIndex());

        QProgressDialog *progress = new QProgressDialog(
            tr("Exporting biome images..."), tr("Abort"), 0, workitems.size(), mainwindow);
        progress->setValue(0);
        progress->setWindowTitle(tr("Export"));

        connect(progress, &QProgressDialog::canceled, this, &ExportDialog::cancel);
        connect(this, &ExportDialog::exportFinished, progress, &QProgressDialog::close);
        connect(this, &ExportDialog::workItemDone, progress,
            [=]() { progress->setValue(progress->value() + 1); },
            Qt::QueuedConnection);

        progress->show();
        startWorkers();
    }
}



