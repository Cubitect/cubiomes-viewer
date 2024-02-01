#include "presetdialog.h"
#include "ui_presetdialog.h"

#include "aboutdialog.h"
#include "mainwindow.h"
#include "util.h"

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
    resize(txtWidth(fontMetrics()) * 128, fontMetrics().height() * 32);

    ui->labelMC->setText(tr("MC ") + mc2str(wi.mc));
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

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

    addPreset(":/examples/two_zombie_villages.txt",
        tr("Abandoned Villages in separate biomes (1.10+)"),
        tr("Two abandoned Villages close together, one in a Plains, the other in a Desert.\n\n"
        "Works best with the 48-bit family search."),
        wi.mc >= MC_1_9);

    addPreset(":/examples/all_fish.txt",
        tr("All the Fish (1.13+)"),
        tr("A River bordering a Lukewarm Ocean somewhere within 2000 blocks, "
           "a combination where all fish variants can spawn."),
        wi.mc >= MC_1_13);

    addPreset(":/examples/portal_village_or_treasure.txt",
        tr("Village or Treasure with Portal (1.16+)"),
        tr("Spawn at a Ruined Portal right beside a Village <b>OR</b> Buried Treasure."),
        wi.mc >= MC_1_16_1);

    addPreset(":/examples/village_portal_stronghold.txt",
        tr("Speedrunner Village (1.16+)"),
        tr("Spawn in a Village with a Ruined Portal leading to a Stronghold.\n\n"
        "Works best with the 48-bit family search."),
        wi.mc >= MC_1_16_1);

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

    addPreset(":/examples/sinkhole.txt",
        tr("Likely sinkhole (1.18+)"),
        tr("Extreme climate weirdness that can generate holes to the world floor.\n\n"
        "In versions 1.19 - 1.19.2, the world generation can have interesting artifacts at these places."),
        wi.mc >= MC_1_18);
    /*
    addPreset(":/examples/old_growth_taiga_somewhere.txt",
        tr("Large Old Growth Taiga somewhere (1.18+)"),
        tr("A large Old Growth Taiga biome somewhere within 2500 blocks.\n\n"
        "Searches an area of +/-2500 blocks for a large climate region that "
        "primarily supports Old Growth Taiga variants"),
        wi.mc >= MC_1_18);
    */
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
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    else
    {
        ui->textDesc->clear();
        ui->formCond->on_buttonRemoveAll_clicked();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
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
                tr("Preset title:"), QLineEdit::Normal,
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
    const QList<QListWidgetItem*> selected = ui->listFilters->selectedItems();
    for (QListWidgetItem *item : selected)
    {
        QString filerc = qvariant_cast<QString>(item->data(Qt::UserRole));
        QFile file(filerc);
        file.remove();
        delete ui->listFilters->takeItem(ui->listFilters->row(item));
    }
}



