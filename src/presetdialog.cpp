#include "presetdialog.h"
#include "ui_presetdialog.h"

#include "cutil.h"
#include "aboutdialog.h"
#include "mainwindow.h"

#include <QPushButton>
#include <QListWidgetItem>
#include <QTextStream>
#include <QFile>
#include <QStandardPaths>
#include <QDirIterator>
#include <QInputDialog>


bool loadConditions(Preset& preset, QString rc)
{
    QFile file(rc);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QTextStream stream(&file);
    while (stream.status() == QTextStream::Ok && !stream.atEnd())
    {
        QString line = stream.readLine();
        if (line.startsWith("#Cond:"))
        {
            Condition c;
            if (c.readHex(line.mid(6).trimmed()))
                preset.condvec.push_back(c);
        }
        else if (line.startsWith("#Title:"))
            preset.title = line.mid(7).trimmed().replace("\\n", "\n");
        else if (line.startsWith("#Desc:"))
            preset.desc = line.mid(6).trimmed().replace("\\n", "\n");
    }
    return true;
}

bool saveConditions(const Preset& preset, QString rc)
{
    QFile file(rc);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QTextStream stream(&file);
    stream << "#Version:  " << VERS_MAJOR << "." << VERS_MINOR << "." << VERS_PATCH << "\n";
    stream << "#Title:    " << QString(preset.title).replace("\n", "\\n") << "\n";
    stream << "#Desc:     " << QString(preset.desc).replace("\n", "\\n") << "\n";
    for (const Condition &c : preset.condvec)
        stream << "#Cond: " << c.toHex() << "\n";
    return true;
}


PresetDialog::PresetDialog(QWidget *parent, WorldInfo wi, bool showEamples)
    : QDialog(parent)
    , ui(new Ui::PresetDialog)
{
    ui->setupUi(this);
    ui->labelMC->setText(tr("MC ") + mc2str(wi.mc));
    connect(ui->buttonOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->buttonCancel, &QPushButton::clicked, this, &QDialog::reject);

    QFont mono = QFont("Monospace", 9);
    mono.setStyleHint(QFont::TypeWriter);
    ui->listFilters->setFont(mono);

    if (showEamples)
        ui->tabWidget->setCurrentWidget(ui->tabExamples);
    else
        ui->tabWidget->setCurrentWidget(ui->tabFilters);

    ui->splitterH->setSizes(QList<int>({7500, 10000}));

    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDirIterator it(path, QDirIterator::NoIteratorFlags);
    while (it.hasNext())
    {
        QString fnam = it.next();
        if (fnam.endsWith(".preset"))
            addPreset(fnam, "", "", true);
    }

    addPreset(":/examples/quadhut_stronghold_mushroom.txt",
        tr("Technical Base (1.4+)"),
        tr("An ideal Quad-hut next to a Stronghold with a Mushroom Island close by."),
        wi.mc >= MC_1_4);

    /*
    addPreset(":/examples/mushroom_icespike.txt",
        tr("Analyze for location with rare biomes together (1.7+)"),
        tr("Check a large area for a Mushroom Island that is next to Ice Spikes.\n\n"
        "Use this in the Analysis Tab with the Condition trigger enabled to "
        "find an instance in the current seed."),
        wi.mc >= MC_1_7);
    */

    addPreset(":/examples/village_portal_stronghold.txt",
        tr("Speedrunner Village (1.16+)"),
        tr("Spawn in a Village with a Ruined Portal leading to a Stronghold.\n\n"
        "Works best with a large search item size and with the 48-bit family search."),
        wi.mc >= MC_1_16);

    /*
    addPreset(":/examples/biome_diversity_1_17.txt",
        tr("Biome Diverity (1.7-1.17)"),
        tr("Get a large biomes diversity within 1000 blocks of the origin."),
        wi.mc >= MC_1_7 && wi.mc <= MC_1_17);
    */

    addPreset(":/examples/biome_diversity_1_18.txt",
        tr("Biome Diverity (1.18+)"),
        tr("A wide range of climates near the origin.\n\n"
        "(Does not look for any particular biomes.)"),
        wi.mc >= MC_1_18);

    addPreset(":/examples/huge_jungle_1_18.txt",
        tr("Large Jungle (1.18+)"),
        tr("A large Jungle biome at the origin.\n\n"
        "Looks for a suitable climate that primarily supports Jungle variants."),
        wi.mc >= MC_1_18);

    addPreset(":/examples/large_birch_forest_1_18.txt",
        tr("Large Birch Forest (1.18+)"),
        tr("A large Birch Forest biome at the origin.\n\n"
        "Looks for a climate that supports Birch Forest variants. "
        "Swamps and Meadows can generate in the same climates and are "
        "explicitly excluded."),
        wi.mc >= MC_1_18);

    addPreset(":/examples/old_growth_taiga_somewhere.txt",
        tr("Large Old Growth Taiga somewhere (1.18+)"),
        tr("A large Old Growth Taiga biome somewhere within 2500 blocks.\n\n"
        "Searches an area of +/-2500 blocks for a large climate region that "
        "primarily supports Old Growth Taiga variants"),
        wi.mc >= MC_1_18);
}

PresetDialog::~PresetDialog()
{
    delete ui;
}

void PresetDialog::setActiveFilter(const QVector<Condition>& condvec)
{
    activeFilter = condvec;
}

QListWidgetItem *PresetDialog::addPreset(QString rc, QString title, QString desc, bool enabled)
{
    presets[rc].title = title;
    presets[rc].desc = desc;
    loadConditions(presets[rc], rc);
    QListWidgetItem *item = new QListWidgetItem(presets[rc].title);
    item->setData(Qt::UserRole, QVariant::fromValue(rc));
    if (!enabled)
    {
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        item->setSelected(false);
        item->setBackground(QColor(128, 128, 128, 128));
    }
    if (rc.startsWith(":"))
        ui->listExamples->addItem(item);
    else
        ui->listFilters->addItem(item);
    return item;
}

QString PresetDialog::getPreset()
{
    QListWidget *list;
    if (ui->tabWidget->currentWidget() == ui->tabFilters)
        list = ui->listFilters;
    else
        list = ui->listExamples;
    QList<QListWidgetItem*> selected = list->selectedItems();
    if (selected.size() > 0)
        return qvariant_cast<QString>(selected.first()->data(Qt::UserRole));
    return "";
}

void PresetDialog::updateSelection()
{
    rc = getPreset();
    if (!rc.isEmpty())
    {
        ui->textDesc->setText(presets[rc].desc);
        ui->formCond->on_buttonRemoveAll_clicked();
        for (Condition& c : presets[rc].condvec)
            ui->formCond->addItemCondition(new QListWidgetItem(), c);
        ui->buttonOk->setEnabled(true);
    }
    else
    {
        ui->textDesc->clear();
        ui->formCond->on_buttonRemoveAll_clicked();
        ui->buttonOk->setEnabled(false);
    }
}

void PresetDialog::on_tabWidget_currentChanged(int)
{
    if (ui->tabWidget->currentWidget() == ui->tabFilters)
        ui->listExamples->clearSelection();
    else
        ui->listFilters->clearSelection();
}

void PresetDialog::on_listFilters_itemSelectionChanged()
{
    updateSelection();
}

void PresetDialog::on_listExamples_itemSelectionChanged()
{
    updateSelection();
}

void PresetDialog::on_buttonSave_clicked()
{
    QString rc;
    int n;
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    for (n = 1; n < 1000; n++)
    {
        rc = path + "/" + QString::number(n) + ".preset";
        if (!QFile::exists(rc))
            break;
    }
    bool ok;
    Preset preset;
    preset.title = QInputDialog::getText(
                this, tr("New Preset"),
                tr("Preset titel:"), QLineEdit::Normal,
                QString("Filter#%1").arg(n), &ok);
    if (!ok || preset.title.isEmpty())
        return;
    preset.desc = ui->textDesc->toPlainText();
    preset.condvec = activeFilter;

    presets.erase(rc);
    saveConditions(preset, rc);
    QListWidgetItem *item = addPreset(rc, "", "", true);
    ui->listFilters->setCurrentItem(item);
}

void PresetDialog::on_buttonDelete_clicked()
{
    QList<QListWidgetItem*> selected = ui->listFilters->selectedItems();
    for (QListWidgetItem *item : selected)
    {
        QString filerc = qvariant_cast<QString>(item->data(Qt::UserRole));
        QFile file(filerc);
        file.remove();
        delete ui->listFilters->takeItem(ui->listFilters->row(item));
    }
}



