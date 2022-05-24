#include "exportdialog.h"
#include "ui_exportdialog.h"

#include "mainwindow.h"
#include "mapview.h"
#include "cutil.h"

#include <QIntValidator>
#include <QFileDialog>
#include <QSettings>
#include <QRunnable>
#include <QProgressDialog>


struct ExportWorker : QRunnable
{
    ExportThread *mt;
    uint64_t seed;
    int tx, tz;
    QString fnam;

    ExportWorker(ExportThread *master) : mt(master) {}
    bool init(uint64_t seed, int tx, int tz)
    {
        this->seed = seed;
        this->tx = tx;
        this->tz = tz;
        fnam = mt->pattern;
        fnam.replace("%S", QString::number(seed));
        fnam.replace("%x", QString::number(tx));
        fnam.replace("%z", QString::number(tz));
        fnam = mt->dir.filePath(fnam);
        return QFileInfo(fnam).exists();
    }

    void run()
    {
        Generator g;
        setupGenerator(&g, mt->wi.mc, mt->wi.large | FORCE_OCEAN_VARIANTS);
        applySeed(&g, mt->dim, seed);

        int y = (mt->scale == 1 ? mt->y : mt->y >> 2);
        Range r = {mt->scale, mt->x, mt->z, mt->w, mt->h, y, 1};

        if (mt->tilesize > 0)
        {
            r.x = tx * mt->tilesize;
            r.z = tz * mt->tilesize;
            r.sx = r.sz = mt->tilesize;
        }

        int *ids = allocCache(&g, r);
        genBiomes(&g, ids, r);

        uchar *rgb = new uchar[r.sx * r.sz * 3];
        enum { BG_NONE, BG_TRANSP, BG_BLACK };
        biomesToImage(rgb, biomeColors, ids, r.sx, r.sz, 1, 1);

        QImage img(rgb, r.sx, r.sz, 3*r.sx, QImage::Format_RGB888);

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

        img.save(fnam);
        free(ids);
        delete [] rgb;
    }
};

ExportThread::~ExportThread()
{
    for (ExportWorker* worker : workers)
        delete worker;
}

void ExportThread::run()
{
    for (ExportWorker *& worker : workers)
    {
        //pool.start(worker);
        worker->run();
        emit workerDone();
        if (stop)
            break;
    }
    //pool.waitForDone();
    deleteLater();
}


static void setCombo(QComboBox *cb, const char *setting)
{
    QSettings settings("cubiomes-viewer", "cubiomes-viewer");
    int idx = settings.value(setting, cb->currentIndex()).toInt();
    if (idx < cb->count())
        cb->setCurrentIndex(idx);
}

ExportDialog::ExportDialog(MainWindow *parent)
    : QDialog(parent)
    , ui(new Ui::ExportDialog)
    , mainwindow(parent)
{
    ui->setupUi(this);

    QIntValidator *intval = new QIntValidator(this);
    ui->lineEditX1->setValidator(intval);
    ui->lineEditZ1->setValidator(intval);
    ui->lineEditX2->setValidator(intval);
    ui->lineEditZ2->setValidator(intval);

    connect(ui->lineEditX1, SIGNAL(editingFinished()), this, SLOT(update()));
    connect(ui->lineEditZ1, SIGNAL(editingFinished()), this, SLOT(update()));
    connect(ui->lineEditX2, SIGNAL(editingFinished()), this, SLOT(update()));
    connect(ui->lineEditZ2, SIGNAL(editingFinished()), this, SLOT(update()));
    connect(ui->comboSeed, SIGNAL(activated(int)), this, SLOT(update()));
    connect(ui->comboScale, SIGNAL(activated(int)), this, SLOT(update()));
    connect(ui->comboTileSize, SIGNAL(activated(int)), this, SLOT(update()));
    connect(ui->groupTiled, SIGNAL(toggled(bool)), this, SLOT(update()));

    QSettings settings("cubiomes-viewer", "cubiomes-viewer");

    ui->lineDir->setText(settings.value("export/prevdir", mainwindow->prevdir).toString());
    ui->linePattern->setText(settings.value("export/pattern", "%S_%x_%z.png").toString());

    setCombo(ui->comboSeed, "export/seedIdx");
    setCombo(ui->comboScale, "export/scaleIdx");
    setCombo(ui->comboTileSize, "export/tileSizeIdx");
    setCombo(ui->comboBackground, "export/bgIdx");

    ui->groupTiled->setChecked(settings.value("export/tiled", false).toBool());
    ui->lineEditX1->setText(settings.value("export/x0").toString());
    ui->lineEditZ1->setText(settings.value("export/z0").toString());
    ui->lineEditX2->setText(settings.value("export/x1").toString());
    ui->lineEditZ2->setText(settings.value("export/z1").toString());

    update();
}

ExportDialog::~ExportDialog()
{
    delete ui;
}

void ExportDialog::update()
{
    int seedcnt = 1;

    if (ui->comboSeed->currentIndex() == 1)
    {
        const QVector<uint64_t>& seeds = mainwindow->formControl->getResults();
        seedcnt = seeds.size();
    }

    ui->labelNumSeeds->setText(QString::number(seedcnt));

    int s = 2 * ui->comboScale->currentIndex();
    int x0 = ui->lineEditX1->text().toInt() >> s;
    int z0 = ui->lineEditZ1->text().toInt() >> s;
    int x1 = (ui->lineEditX2->text().toInt() >> s) + 1;
    int z1 = (ui->lineEditZ2->text().toInt() >> s) + 1;

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
        int seedmode = ui->comboSeed->currentIndex();
        QString pattern = ui->linePattern->text();
        bool tiled = ui->groupTiled->isChecked();

        WorldInfo wi;
        mainwindow->getSeed(&wi, true);

        QVector<uint64_t> seeds;
        if (seedmode == 0)
            seeds.push_back(wi.seed);
        else if (seedmode == 1)
            seeds = mainwindow->formControl->getResults();

        if (seeds.size() > 1 && !pattern.contains("%S"))
        {
            QMessageBox::warning(this, tr("Warning"),
                    tr("When exporting more than one seed, the file pattern "
                    "has to include the \"%S\" format specifier for the seed number."));
            return;
        }

        if (tiled && (!pattern.contains("%x") || !pattern.contains("%z")))
        {
            QMessageBox::warning(this, tr("Warning"),
                    tr("Exporting as tiled images requires both the \"%x\" and \"%z\" "
                    "format specifiers in the file pattern, representing the tile coordinates."));
            return;
        }

        int s = 2 * ui->comboScale->currentIndex();
        int x0 = ui->lineEditX1->text().toInt() >> s;
        int z0 = ui->lineEditZ1->text().toInt() >> s;
        int x1 = (ui->lineEditX2->text().toInt() >> s) + 1;
        int z1 = (ui->lineEditZ2->text().toInt() >> s) + 1;
        int y = (s == 0 ? wi.y : wi.y >> 2);

        if (x1 <= x0 || z1 <= z0)
        {
            QMessageBox::warning(this, tr("Warning"), tr("Invalid area."));
            return;
        }

        ExportThread *master = new ExportThread(mainwindow);
        master->dir = QDir(ui->lineDir->text());
        master->pattern = pattern;
        master->wi = wi;
        master->dim = mainwindow->getDim();
        master->scale = 1 << s;
        master->x = x0;
        master->z = z0;
        master->w = x1 - x0;
        master->h = z1 - z0;
        master->y = y;
        master->tilesize = -1;
        master->bgmode = 0;

        QVector<ExportWorker*> workers;
        bool existwarn = false;

        if (tiled)
        {
            int bgmode = ui->comboBackground->currentIndex();
            int tilesize = ui->comboTileSize->currentText().section('x', 0, 0).toInt();
            int tx0 = (int) floor(x0 / (qreal)tilesize);
            int tz0 = (int) floor(z0 / (qreal)tilesize);
            int tx1 = (int) ceil(x1 / (qreal)tilesize);
            int tz1 = (int) ceil(z1 / (qreal)tilesize);

            master->tilesize = tilesize;
            master->bgmode = bgmode;

            for (uint64_t seed : seeds)
            {
                for (int x = tx0; x < tx1; x++)
                {
                    for (int z = tz0; z < tz1; z++)
                    {
                        ExportWorker *worker = new ExportWorker(master);
                        existwarn |= worker->init(seed, x, z);
                        workers.push_back(worker);
                    }
                }
            }
        }
        else
        {
            int maxsiz = 0x8000;
            if (x1 - x0 >= maxsiz || z1 - z0 >= maxsiz)
            {
                int button = QMessageBox::warning(this, tr("Warning"),
                        tr("Consider tiling very large images into smaller sections.\n"
                           "Continue?"),
                        QMessageBox::Cancel | QMessageBox::Yes);
                if (button == QMessageBox::Cancel)
                {
                    delete master;
                    return;
                }
            }

            for (uint64_t seed : seeds)
            {
                ExportWorker *worker = new ExportWorker(master);
                existwarn |= worker->init(seed, 0, 0);
                workers.push_back(worker);
            }
        }

        if (existwarn)
        {
            int button = QMessageBox::warning(this, tr("Warning"),
                    tr("One or more of files already exist.\n"
                       "Continue and overwrite?"),
                    QMessageBox::Cancel | QMessageBox::Yes);
            if (button == QMessageBox::Cancel)
            {
                for (ExportWorker *worker : workers)
                    delete worker;
                workers.clear();
                delete master;
                return;
            }
        }

        QSettings settings("cubiomes-viewer", "cubiomes-viewer");
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

        QProgressDialog *progress = new QProgressDialog(
            tr("Export biome images..."), tr("Abort"), 0, workers.size(), mainwindow);
        progress->setValue(0);

        connect(progress, &QProgressDialog::canceled, master, &ExportThread::cancel);
        connect(master, &ExportThread::finished, progress, &QProgressDialog::close);
        connect(master, &ExportThread::workerDone, progress,
            [=]() { progress->setValue(progress->value() + 1); },
            Qt::QueuedConnection);

        progress->show();
        master->workers = workers;
        master->start();
    }
}



