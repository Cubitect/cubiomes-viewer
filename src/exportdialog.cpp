#include "exportdialog.h"
#include "ui_exportdialog.h"

#include "mainwindow.h"
#include "mapview.h"
#include "cutil.h"

#include <QIntValidator>
#include <QFileDialog>
#include <QDir>
#include <QSettings>


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

    connect(ui->lineEditX1, &QLineEdit::editingFinished, this, &ExportDialog::update);
    connect(ui->lineEditZ1, &QLineEdit::editingFinished, this, &ExportDialog::update);
    connect(ui->lineEditX2, &QLineEdit::editingFinished, this, &ExportDialog::update);
    connect(ui->lineEditZ2, &QLineEdit::editingFinished, this, &ExportDialog::update);

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

void ExportDialog::on_comboSeed_activated(int)
{
    update();
}

void ExportDialog::on_comboScale_activated(int)
{
    update();
}

void ExportDialog::on_comboTileSize_activated(int)
{
    update();
}

void ExportDialog::on_groupTiled_toggled(bool)
{
    update();
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

int ExportDialog::saveimg(QString dir, QString pattern, uint64_t seed, int tx, int tz, QImage *img)
{
    QString fnam = pattern;
    fnam.replace("%S", QString::number(seed));
    fnam.replace("%x", QString::number(tx));
    fnam.replace("%z", QString::number(tz));
    fnam = QDir(dir).filePath(fnam);

    QFileInfo finfo(fnam);
    if (img)
    {
        if (!img->save(fnam))
        {
            QMessageBox::warning(this, tr("Warning"),
                    tr("Error while saving image."));
            return 2;
        }
    }
    else
    {
        if (finfo.exists())
        {
            int button = QMessageBox::warning(this, tr("Warning"),
                    tr("One or more of files already exist.\n"
                       "Continue and overwrite?"),
                    QMessageBox::Cancel | QMessageBox::Yes);
            if (button == QMessageBox::Cancel)
                return 2;
            return 1;
        }
    }
    return 0;
}

void ExportDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::StandardButton b = ui->buttonBox->standardButton(button);

    if (b == QDialogButtonBox::Ok)
    {
        int seedmode = ui->comboSeed->currentIndex();
        QString dir = ui->lineDir->text();
        QString pattern = ui->linePattern->text();
        bool tiled = ui->groupTiled->isChecked();

        int dim = mainwindow->getDim();
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

        Generator g;
        setupGenerator(&g, wi.mc, wi.large);// | FORCE_OCEAN_VARIANTS);

        int s = 2 * ui->comboScale->currentIndex();
        int x0 = ui->lineEditX1->text().toInt() >> s;
        int z0 = ui->lineEditZ1->text().toInt() >> s;
        int x1 = (ui->lineEditX2->text().toInt() >> s) + 1;
        int z1 = (ui->lineEditZ2->text().toInt() >> s) + 1;
        int y = (s == 0 ? wi.y : wi.y >> 2);

        if (x1 <= x0 || z1 <= z0)
        {
            QMessageBox::warning(this, tr("Warning"),
                    tr("Invalid area."));
            return;
        }

        if (tiled)
        {
            int tilesize = ui->comboTileSize->currentText().section('x', 0, 0).toInt();
            int tx0 = (int) floor(x0 / (qreal)tilesize);
            int tz0 = (int) floor(z0 / (qreal)tilesize);
            int tx1 = (int) ceil(x1 / (qreal)tilesize);
            int tz1 = (int) ceil(z1 / (qreal)tilesize);

            for (uint64_t seed : seeds)
            {
                for (int x = tx0; x < tx1; x++)
                {
                    for (int z = tz0; z < tz1; z++)
                    {
                        int st = saveimg(dir, pattern, seed, x, z, nullptr);
                        if (st == 2)
                            return;
                        if (st == 1)
                            goto L_tiles_contin;
                    }
                }
            }

        L_tiles_contin:
            uchar *rgb = new uchar[tilesize * tilesize * 3];
            enum { BG_NONE, BG_TRANSP, BG_BLACK };
            int bgmode = ui->comboBackground->currentIndex();

            for (uint64_t seed : seeds)
            {
                for (int x = tx0; x < tx1; x++)
                {
                    for (int z = tz0; z < tz1; z++)
                    {
                        Range r = {1 << s, x * tilesize, z * tilesize, tilesize, tilesize, y, 1};
                        applySeed(&g, dim, seed);

                        int *ids = allocCache(&g, r);
                        genBiomes(&g, ids, r);
                        biomesToImage(rgb, biomeColors, ids, tilesize, tilesize, 1, 1);

                        QImage img(rgb, tilesize, tilesize, 3*tilesize, QImage::Format_RGB888);

                        if (bgmode != BG_NONE)
                        {
                            QColor bg = QColor(Qt::black);
                            if (bgmode == BG_TRANSP)
                            {
                                bg = QColor(Qt::transparent);
                                img = img.convertToFormat(QImage::Format_RGBA8888, Qt::AutoColor);
                            }
                            for (int j = 0; j < r.sz; j++)
                            {
                                for (int i = 0; i < r.sx; i++)
                                    if (r.z+j < z0 || r.z+j > z1 || r.x+i < x0 || r.x+i > x1)
                                        img.setPixelColor(i, j, bg);
                            }
                        }

                        int err = saveimg(dir, pattern, seed, x, z, &img);
                        free(ids);
                        if (err)
                            goto L_tiles_cleanup;
                    }
                }
            }

        L_tiles_cleanup:
            delete[] rgb;
        }
        else
        {
            int maxsiz = 0x8000;
            int w = x1 - x0, h = z1 - z0;
            if (w >= maxsiz || h >= maxsiz)
            {
                int button = QMessageBox::warning(this, tr("Warning"),
                        tr("Consider tiling very large images into smaller sections.\n"
                           "Continue?"),
                        QMessageBox::Cancel | QMessageBox::Yes);
                if (button == QMessageBox::Cancel)
                    return;
            }

            for (uint64_t seed : seeds)
            {
                if (saveimg(dir, pattern, seed, 0, 0, nullptr))
                    return;
            }

            Range r = {1 << s, x0, z0, w, h, y, 1};
            int *ids = allocCache(&g, r);
            uchar *rgb = new uchar[r.sx * r.sz * 3];

            for (uint64_t seed : seeds)
            {
                applySeed(&g, dim, seed);
                genBiomes(&g, ids, r);
                biomesToImage(rgb, biomeColors, ids, r.sx, r.sz, 1, 1);
                QImage img(rgb, r.sx, r.sz, 3*r.sx, QImage::Format_RGB888);
                if (saveimg(dir, pattern, seed, 0, 0, &img))
                    break;
            }

            delete[] rgb;
            free(ids);
        }

        QSettings settings("cubiomes-viewer", "cubiomes-viewer");
        settings.setValue("export/seedIdx", ui->comboSeed->currentIndex());
        settings.setValue("export/prevdir", dir);
        settings.setValue("export/pattern", pattern);
        settings.setValue("export/scaleIdx", ui->comboScale->currentIndex());
        settings.setValue("export/x0", ui->lineEditX1->text());
        settings.setValue("export/z0", ui->lineEditZ1->text());
        settings.setValue("export/x1", ui->lineEditX2->text());
        settings.setValue("export/z1", ui->lineEditZ2->text());
        settings.setValue("export/tiled", ui->groupTiled->isChecked());
        settings.setValue("export/tileSizeIdx", ui->comboTileSize->currentIndex());
        settings.setValue("export/bgIdx", ui->comboBackground->currentIndex());
    }
}
