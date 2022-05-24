#include "biomecolordialog.h"
#include "ui_biomecolordialog.h"
#include "mainwindow.h"
#include "cutil.h"

#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QIcon>
#include <QPushButton>
#include <QColorDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QDirIterator>


#define DIM_DIVIDER 6

static QIcon getColorIcon(const QColor& col)
{
    QPixmap pixmap(14,14);
    pixmap.fill(QColor(0,0,0,0));
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(QRectF(1, 1, 12, 12), 3, 3);
    QPen pen(Qt::black, 1);
    p.setPen(pen);

    p.fillPath(path, col);
    p.drawPath(path);
    return QIcon(pixmap);
}

BiomeColorDialog::BiomeColorDialog(MainWindow *parent, QString initrc)
    : QDialog(parent)
    , ui(new Ui::BiomeColorDialog)
    , parent(parent)
    , modified()
{
    ui->setupUi(this);
    ui->buttonOk->setIcon(style()->standardIcon(QStyle::SP_DialogOkButton));

    memset(buttons, 0, sizeof(buttons));
    memcpy(colors, biomeColors, sizeof(colors));

    unsigned char coldefault[256][3];
    initBiomeColors(coldefault);


    QPushButton *button;
    ui->gridLayout->setSpacing(2);
    QPixmap alignicon(14, 14);
    alignicon.fill(Qt::transparent);

    button = new QPushButton(QIcon(alignicon), tr("All to default"), this);
    connect(button, &QPushButton::clicked, this, &BiomeColorDialog::onAllToDefault);
    ui->gridLayout->addWidget(button, 0, 1);

    button = new QPushButton(QIcon(alignicon), tr("All to dimmed"), this);
    connect(button, &QPushButton::clicked, this, &BiomeColorDialog::onAllToDimmed);
    ui->gridLayout->addWidget(button, 0, 2);

    for (int i = 0; i < 256; i++)
    {
        const char *bname = biome2str(MC_NEWEST, i);
        if (!bname)
            continue;

        QColor col;
        col = QColor(colors[i][0], colors[i][1], colors[i][2]);
        buttons[i] = button = new QPushButton(getColorIcon(col), bname, this);
        connect(button, &QPushButton::clicked, [=]() { this->editBiomeColor(i); });
        ui->gridLayout->addWidget(button, i+1, 0);

        col = QColor(coldefault[i][0], coldefault[i][1], coldefault[i][2]);
        button = new QPushButton(getColorIcon(col), tr("Default reset"), this);
        connect(button, &QPushButton::clicked, [=]() { this->setBiomeColor(i, col); });
        ui->gridLayout->addWidget(button, i+1, 1);

        col = QColor(coldefault[i][0] / DIM_DIVIDER, coldefault[i][1] / DIM_DIVIDER, coldefault[i][2] / DIM_DIVIDER);
        button = new QPushButton(getColorIcon(col), tr("Dimmed reset"), this);
        connect(button, &QPushButton::clicked, [=]() { this->setBiomeColor(i, col); });
        ui->gridLayout->addWidget(button, i+1, 2);
    }

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
        if (line.length() < 2 || !line.startsWith(";"))
            continue;
        ui->comboColormaps->addItem(line.mid(1).trimmed(), rc);
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
}

BiomeColorDialog::~BiomeColorDialog()
{
    delete ui;
}

/// Saves the current colormap as the given rc.
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

void BiomeColorDialog::setBiomeColor(int id, const QColor &col)
{
    buttons[id]->setIcon(getColorIcon(col));
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
    QFile file(activerc);
    if (!activerc.isEmpty() && file.open(QIODevice::ReadOnly))
    {
        char buf[32*1024];
        qint64 siz = file.read(buf, sizeof(buf)-1);
        file.close();
        if (siz >= 0)
        {
            buf[siz] = 0;
            initBiomeColors(colors);
            parseBiomeColors(colors, buf);
        }
    }
    else
    {
        initBiomeColors(colors);
    }

    for (int i = 0; i < 256; i++)
    {
        if (buttons[i])
        {
            QColor col(colors[i][0], colors[i][1], colors[i][2]);
            buttons[i]->setIcon(getColorIcon(col));
        }
    }
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
    parent->setBiomeColorRc(activerc);
    accept();
}


void BiomeColorDialog::onAllToDefault()
{
    initBiomeColors(colors);
    for (int i = 0; i < 256; i++)
    {
        if (buttons[i])
        {
            QColor col(colors[i][0], colors[i][1], colors[i][2]);
            buttons[i]->setIcon(getColorIcon(col));
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
        if (buttons[i])
        {
            QColor col(colors[i][0], colors[i][1], colors[i][2]);
            buttons[i]->setIcon(getColorIcon(col));
        }
    }
    modified = true;
}
