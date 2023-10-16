#include "biomecolordialog.h"
#include "ui_biomecolordialog.h"

#include "mainwindow.h"
#include "message.h"
#include "config.h"
#include "util.h"

#include <QPushButton>
#include <QColorDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QTextStream>
#include <QDirIterator>
#include <QMessageBox>

#define DIM_DIVIDER 6

BiomeColorDialog::BiomeColorDialog(MainWindow *parent, QString initrc, int mc, int dim)
    : QDialog(parent)
    , ui(new Ui::BiomeColorDialog)
    , mainwindow(parent)
    , mc(mc), dim(dim)
    , modified()
{
    ui->setupUi(this);
    ui->buttonOk->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));

    memcpy(colors, g_biomeColors, sizeof(colors));
    memset(buttons, 0, sizeof(buttons));

    unsigned char coldefault[256][3];
    initBiomeColors(coldefault);

    QPushButton *button;
    ui->gridLayout->setSpacing(2);

    separator = new QLabel(tr("Currently inactive biomes:"), this);
    separator->setVisible(false);

    button = new QPushButton(getColorIcon(QColor(0,0,0,0), QPen(Qt::transparent)), tr("Import..."), this);
    connect(button, &QPushButton::clicked, this, &BiomeColorDialog::onImport);
    ui->gridLayout->addWidget(button, 0, 0);

    button = new QPushButton(getColorIcon(QColor(255,255,255,128)), tr("All to default"), this);
    connect(button, &QPushButton::clicked, this, &BiomeColorDialog::onAllToDefault);
    ui->gridLayout->addWidget(button, 0, 1);

    button = new QPushButton(getColorIcon(QColor(0,0,0,128)), tr("All to dimmed"), this);
    connect(button, &QPushButton::clicked, this, &BiomeColorDialog::onAllToDimmed);
    ui->gridLayout->addWidget(button, 0, 2);

    for (int i = 0; i < 256; i++)
    {
        QString bname = getBiomeDisplay(mc, i);
        if (bname.isEmpty() || bname == "?")
            continue;

        QColor col;
        col = QColor(colors[i][0], colors[i][1], colors[i][2]);
        buttons[i][0] = button = new QPushButton(getColorIcon(col), bname, this);
        connect(button, &QPushButton::clicked, [=]() { this->editBiomeColor(i); });
        ui->gridLayout->addWidget(button, i+1, 0);

        col = QColor(coldefault[i][0], coldefault[i][1], coldefault[i][2]);
        buttons[i][1] = button = new QPushButton(getColorIcon(col), tr("Default reset"), this);
        connect(button, &QPushButton::clicked, [=]() { this->setBiomeColor(i, col); });
        ui->gridLayout->addWidget(button, i+1, 1);

        col = QColor(coldefault[i][0] / DIM_DIVIDER, coldefault[i][1] / DIM_DIVIDER, coldefault[i][2] / DIM_DIVIDER);
        buttons[i][2] = button = new QPushButton(getColorIcon(col), tr("Dimmed reset"), this);
        connect(button, &QPushButton::clicked, [=]() { this->setBiomeColor(i, col); });
        ui->gridLayout->addWidget(button, i+1, 2);
    }
    ui->gridLayout->addWidget(separator, 256, 0, 1, 3);

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDirIterator it(dir, QDirIterator::NoIteratorFlags);
    while (it.hasNext())
    {
        QString rc = it.next();
        if (!rc.endsWith(".colormap"))
            continue;
        QFile file(rc);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        QTextStream stream(&file);
        QString line = stream.readLine();
        if (line.length() >= 2 && line.startsWith(";"))
            ui->comboColormaps->addItem(line.mid(1).trimmed(), rc);
        else
            ui->comboColormaps->addItem(QFileInfo(file).fileName(), rc);
    }
    ui->comboColormaps->model()->sort(0, Qt::AscendingOrder);
    ui->comboColormaps->insertItem(0, tr("[default]"));
    int index = 0;
    for (int i = 0; i < ui->comboColormaps->count(); i++)
    {
        QString itemrc = qvariant_cast<QString>(ui->comboColormaps->itemData(i));
        if (initrc == itemrc)
        {
            index = i;
            break;
        }
    }
    ui->comboColormaps->setCurrentIndex(index);

    arrange(IdCmp::SORT_LEX);
}

BiomeColorDialog::~BiomeColorDialog()
{
    delete ui;
}

void BiomeColorDialog::arrange(int sort)
{
    QLayoutItem *sep = ui->gridLayout->takeAt(ui->gridLayout->indexOf(separator));
    QLayoutItem *items[256][3] = {};
    for (int i = 0; i < 256; i++)
    {
        if (!buttons[i][0])
            continue;
        for (int j = 0; j < 3; j++)
        {
            int idx = ui->gridLayout->indexOf(buttons[i][j]);
            items[i][j] = ui->gridLayout->takeAt(idx);
        }
    }
    IdCmp cmp = {sort, mc, dim};
    int ids[256];
    for (int i = 0; i < 256; i++)
        ids[i] = i;
    std::sort(ids, ids+256, cmp);

    bool isprim = true;
    int row = 1;
    for (int i = 0; i < 256; i++)
    {
        int id = ids[i];
        if (!items[id][0])
            continue;
        if (isprim && !cmp.isPrimary(id))
        {
            isprim = false;
            ui->gridLayout->addItem(sep, row, 0, 1, 3);
            sep = 0;
            row++;
        }
        for (int j = 0; j < 3; j++)
            ui->gridLayout->addItem(items[id][j], row, j);
        row++;
    }
    if (sep)
    {
        ui->gridLayout->addItem(sep, row, 0, 1, 3);
        separator->setVisible(false);
    }
    else
    {
        separator->setVisible(true);
    }
}

/// Saves the current colormap at the given path.
/// If the rc already corresponds to an item, the description
/// will be updated only if the new desc is non-empty.
int BiomeColorDialog::saveColormap(QString rc, QString desc)
{
    if (rc.isEmpty())
        return -1;
    QFile file(rc);
    if (!file.open(QIODevice::WriteOnly))
        return -1;

    int index = -1;
    for (int i = 0; i < ui->comboColormaps->count(); i++)
    {
        QString itemrc = qvariant_cast<QString>(ui->comboColormaps->itemData(i));
        if (rc == itemrc)
        {
            index = i;
            if (desc.isEmpty())
                desc = ui->comboColormaps->itemText(i);
            break;
        }
    }

    QTextStream stream(&file);
    if (!desc.isEmpty())
        stream << ";" << desc << "\n";
    for (int i = 0; i < 256; i++)
    {
        const char *bname = biome2str(MC_NEWEST, i);
        if (!bname)
            continue;
        stream << bname << " " << colors[i][0] << " " << colors[i][1] << " " << colors[i][2] << "\n";
    }

    modified = false;
    if (index > 0)
    {
        ui->comboColormaps->setItemText(index, desc);
    }
    else
    {
        index = ui->comboColormaps->count();
        ui->comboColormaps->addItem(desc, rc);
    }
    return index;
}

int BiomeColorDialog::loadColormap(QString path, bool reset)
{
    QFile file(path);
    int n = 0;
    if (!path.isEmpty() && file.open(QIODevice::ReadOnly))
    {
        char buf[32*1024];
        qint64 siz = file.read(buf, sizeof(buf)-1);
        file.close();
        if (siz >= 0)
        {
            buf[siz] = 0;
            if (reset)
                initBiomeColors(colors);
            n = parseBiomeColors(colors, buf);
        }
    }
    else if (reset)
    {
        initBiomeColors(colors);
    }

    for (int i = 0; i < 256; i++)
    {
        if (buttons[i][0])
        {
            QColor col(colors[i][0], colors[i][1], colors[i][2]);
            buttons[i][0]->setIcon(getColorIcon(col));
        }
    }
    return n;
}

QString BiomeColorDialog::getRc()
{
    return activerc;
}

void BiomeColorDialog::setBiomeColor(int id, const QColor &col)
{
    buttons[id][0]->setIcon(getColorIcon(col));
    colors[id][0] = col.red();
    colors[id][1] = col.green();
    colors[id][2] = col.blue();
    modified = true;
}

void BiomeColorDialog::editBiomeColor(int id)
{
    QColor col = QColor(colors[id][0], colors[id][1], colors[id][2]);
    QColorDialog *dialog = new QColorDialog(col, this);
    //dialog->setOption(QColorDialog::DontUseNativeDialog, true);
    if (dialog->exec())
    {
        setBiomeColor(id, dialog->selectedColor());
    }
}

void BiomeColorDialog::on_comboColormaps_currentIndexChanged(int index)
{
    ui->buttonRemove->setEnabled(index != 0);
    if (modified)
    {
        if (activerc.isEmpty())
            on_buttonSaveAs_clicked();
        else
            saveColormap(activerc, "");
    }

    activerc = qvariant_cast<QString>(ui->comboColormaps->currentData());
    loadColormap(activerc, true);
}

void BiomeColorDialog::on_buttonSaveAs_clicked()
{
    QString rc;
    int n;
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    for (n = 1; n < 1000; n++)
    {
        rc = path + "/" + QString::number(n) + ".colormap";
        if (!QFile::exists(rc))
            break;
    }
    bool ok;
    QString desc = QInputDialog::getText(
            this, tr("Save biome colors as..."),
            tr("Biome colors:"), QLineEdit::Normal,
            QString("Colormap#%1").arg(n), &ok);
    if (!ok || desc.isEmpty())
        return;

    int idx = saveColormap(rc, desc);
    ui->comboColormaps->setCurrentIndex(idx);
    activerc = rc;
}

void BiomeColorDialog::on_buttonExport_clicked()
{
    QFileInfo finfo(mainwindow->prevdir);
    QString fnam = QFileDialog::getSaveFileName(
        this, tr("Export biome color map"), finfo.absolutePath(), tr("Color map files (*.colormap *.txt);;Any files (*)"));
    if (fnam.isEmpty())
        return;
    QFile file(fnam);
    if (!file.open(QIODevice::WriteOnly))
    {
        warn(this, tr("Failed open file for writing."));
        return;
    }
    QTextStream stream(&file);
    for (int i = 0; i < 256; i++)
    {
        const char *bname = biome2str(MC_NEWEST, i);
        if (!bname)
            continue;
        stream << bname << " " << colors[i][0] << " " << colors[i][1] << " " << colors[i][2] << "\n";
    }
}

void BiomeColorDialog::on_buttonRemove_clicked()
{
    QString rc = qvariant_cast<QString>(ui->comboColormaps->currentData());
    if (!rc.isEmpty())
    {
        modified = false; // discard changes
        ui->comboColormaps->removeItem(ui->comboColormaps->currentIndex());
        QFile file(rc);
        file.remove();
    }
}

void BiomeColorDialog::on_buttonOk_clicked()
{
    on_comboColormaps_currentIndexChanged(ui->comboColormaps->currentIndex());
    emit yieldBiomeColorRc(activerc);
    accept();
}

void BiomeColorDialog::onImport()
{
    QFileInfo finfo(mainwindow->prevdir);
    QString fnam = QFileDialog::getOpenFileName(
        this, tr("Load biome color map"), finfo.absolutePath(), tr("Color map files (*.colormap *.txt);;Any files (*)"));
    if (!fnam.isNull())
    {
        int n = loadColormap(fnam, false);
        if (n == 0)
        {
            warn(this, tr("No biome colors found."));
            return;
        }
        info(this, tr("Replaced %1 biome colors.").arg(n));
        modified = true;
    }
}

void BiomeColorDialog::onColorHelp()
{
    QString msg = tr(
            "<html><head/><body><p>"
            "<b>Custom biome colors</b> should be defined in an ASCII text file, "
            "with one biome-color mapping per line. Each mapping should consist "
            "of a biome ID or biome resource name followed by a color that can be "
            "written as a hex code (prefixed with # or 0x) or as an RGB triplet. "
            "Special characters are ignored."
            "</p><p>"
            "<b>Examples:</b>"
            "</p><p>"
            "sunflower_plains:&nbsp;#FFFF00"
            "</p><p>"
            "128&nbsp;[255&nbsp;255&nbsp;0]"
            "</p></body></html>"
            );
    QMessageBox::information(this, tr("Help: custom biome colors"), msg, QMessageBox::Ok);
}

void BiomeColorDialog::onAllToDefault()
{
    initBiomeColors(colors);
    for (int i = 0; i < 256; i++)
    {
        if (buttons[i][0])
        {
            QColor col(colors[i][0], colors[i][1], colors[i][2]);
            buttons[i][0]->setIcon(getColorIcon(col));
        }
    }
    modified = true;
}

void BiomeColorDialog::onAllToDimmed()
{
    initBiomeColors(colors);
    for (int i = 0; i < 256; i++)
    {
        colors[i][0] /= DIM_DIVIDER;
        colors[i][1] /= DIM_DIVIDER;
        colors[i][2] /= DIM_DIVIDER;
        if (buttons[i][0])
        {
            QColor col(colors[i][0], colors[i][1], colors[i][2]);
            buttons[i][0]->setIcon(getColorIcon(col));
        }
    }
    modified = true;
}
