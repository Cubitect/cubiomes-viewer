#include "biomecolordialog.h"
#include "ui_biomecolordialog.h"

#include "config.h"
#include "mainwindow.h"
#include "message.h"
#include "util.h"

#include <QBuffer>
#include <QByteArray>
#include <QColorDialog>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>

#define DIM_DIVIDER 6

BiomeColorDialog::BiomeColorDialog(MainWindow *parent, QString initrc, int mc, int dim)
    : QDialog(parent)
    , ui(new Ui::BiomeColorDialog)
    , mainwindow(parent)
    , mc(mc), dim(dim)
    , modified()
{
    ui->setupUi(this);
    connect(ui->buttonRemove, &QPushButton::clicked, this, &BiomeColorDialog::onRemove);
    connect(ui->buttonSaveAs, &QPushButton::clicked, this, &BiomeColorDialog::onSaveAs);
    connect(ui->buttonExport, &QPushButton::clicked, this, &BiomeColorDialog::onExport);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &BiomeColorDialog::onAccept);

    memcpy(colors, g_biomeColors, sizeof(colors));
    memset(buttons, 0, sizeof(buttons));

    unsigned char coldefault[256][3];
    initBiomeColors(coldefault);

    ui->gridLayout->setSpacing(2);

    separator = new QLabel(tr("Currently inactive biomes:"), this);
    separator->setVisible(false);

    QPushButton *button;

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
    QDirIterator it(dir);
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
    stream.flush();
    file.close();

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

int BiomeColorDialog::loadColormap(QIODevice *iodevice, bool reset)
{
    int n = 0;
    if (iodevice && iodevice->open(QIODevice::ReadOnly))
    {
        char buf[32*1024];
        qint64 siz = iodevice->read(buf, sizeof(buf)-1);
        iodevice->close();
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

    if (!reset)
    {
        if (n == 0)
        {
            warn(this, tr("No biome colors found."));
            return n;
        }
        info(this, tr("Replaced %n biome color(s).", "", n));
        modified = true;
    }
    return n;
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
    connect(dialog, &QDialog::accepted, [=] {
        setBiomeColor(id, dialog->selectedColor());
    });
    dialog->show();
}

void BiomeColorDialog::on_comboColormaps_currentIndexChanged(int index)
{
    ui->buttonRemove->setEnabled(index != 0);
    if (modified)
    {
        if (activerc.isEmpty())
            onSaveAs();
        else
            saveColormap(activerc, "");
    }

    activerc = qvariant_cast<QString>(ui->comboColormaps->currentData());
    if (!activerc.isEmpty())
    {
        QFile file(activerc);
        loadColormap(&file, true);
    }
    else
    {
        loadColormap(nullptr, true);
    }
}

void BiomeColorDialog::onSaveAs()
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
    QInputDialog *dialog = new QInputDialog(this);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(tr("Save biome colors as..."));
    dialog->setLabelText(tr("Biome colors:"));
    dialog->setTextValue(QString("Colormap#%1").arg(n));
    connect(dialog, &QInputDialog::textValueSelected, [=](const QString &text) {
        this->onSaveSelect(rc, text);
    });
    dialog->show();
}

void BiomeColorDialog::onSaveSelect(const QString& rc, const QString &text)
{
    int idx = saveColormap(rc, text);
    ui->comboColormaps->setCurrentIndex(idx);
    emit yieldBiomeColorRc(rc);
    accept();
}

void BiomeColorDialog::exportColors(QTextStream& stream)
{
    for (int i = 0; i < 256; i++)
    {
        const char *bname = biome2str(MC_NEWEST, i);
        if (!bname)
            continue;
        stream << bname << " " << colors[i][0] << " " << colors[i][1] << " " << colors[i][2] << "\n";
    }
    stream.flush();
}

void BiomeColorDialog::onExport()
{
#if WASM
    QByteArray content;
    QTextStream stream(&content);
    exportColors(stream);
    QFileDialog::saveFileContent(content, "biomecolors.txt");
#else
    QString fnam = QFileDialog::getSaveFileName(
        this, tr("Export biome color map"), mainwindow->prevdir, tr("Color map files (*.colormap *.txt);;Any files (*)"));
    if (fnam.isEmpty())
        return;

    QFileInfo finfo(fnam);
    QFile file(fnam);
    mainwindow->prevdir = finfo.absolutePath();

    if (!file.open(QIODevice::WriteOnly))
    {
        warn(this, tr("Failed to open file for export:\n\"%1\"").arg(fnam));
        return;
    }

    QTextStream stream(&file);
    exportColors(stream);
#endif
}

void BiomeColorDialog::onRemove()
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

void BiomeColorDialog::onAccept()
{
    if (modified)
    {
        if (activerc.isEmpty())
        {
            onSaveAs();
            return; // accept only after save-as dialog
        }
        saveColormap(activerc, "");
    }
    emit yieldBiomeColorRc(activerc);
    accept();
}

void BiomeColorDialog::onImport()
{
    QString filter = tr("Color map files (*.colormap *.txt);;Any files (*)");
#if WASM
    auto fileOpenCompleted = [=](const QString &fnam, const QByteArray &content) {
        if (!fnam.isEmpty()) {
            QBuffer buffer;
            buffer.setData(content);
            loadColormap(&buffer, false);
        }
    };
    QFileDialog::getOpenFileContent(filter, fileOpenCompleted);
#else
    QString fnam = QFileDialog::getOpenFileName(
        this, tr("Load biome color map"), mainwindow->prevdir, filter);
    if (fnam.isEmpty())
        return;
    QFileInfo finfo(fnam);
    QFile file(fnam);
    mainwindow->prevdir = finfo.absolutePath();
    loadColormap(&file, false);
#endif
}

void BiomeColorDialog::onColorHelp()
{
    QMessageBox *mb = new QMessageBox(this);
    mb->setIcon(QMessageBox::Information);
    mb->setWindowTitle(tr("Help: custom biome colors"));
    mb->setText(tr(
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
        ));
    mb->show();
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
