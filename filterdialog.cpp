#include "filterdialog.h"
#include "ui_filterdialog.h"

#include <QCheckBox>
#include <QIntValidator>


#define SETUP_BIOME_CHECKBOX(B) do {\
        biomecboxes[B] = new QCheckBox(#B);\
        ui->gridLayoutBiomes->addWidget(biomecboxes[B], B % 128, B / 128);\
    } while (0)

FilterDialog::FilterDialog(MainWindow *parent, Condition *initcond) :
    QDialog(parent, Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
    ui(new Ui::FilterDialog)
{
    memset(&cond, 0, sizeof(cond));
    ui->setupUi(this);

    for (int i = 1; i < FILTER_MAX; i++)
    {
        const FilterInfo &ft = g_filterinfo.list[i];
        if (ft.icon)
            ui->comboBoxType->addItem(QIcon(ft.icon), ft.name, i);
        else
            ui->comboBoxType->addItem(ft.name, i);
    }

    int initindex = 0;
    QVector<Condition> existing = parent->getConditions();
    for (Condition c : existing)
    {
        const FilterInfo &ft = g_filterinfo.list[c.type];
        if (initcond)
        {
            if (c.save == initcond->save)
                continue;
            if (c.save == initcond->relative)
                initindex = ui->comboBoxRelative->count();
        }
        ui->comboBoxRelative->addItem(QString::asprintf("[%02d] %s", c.save, ft.name), c.save);
    }

    ui->lineRadius->setValidator(new QIntValidator(this));
    ui->lineEditX1->setValidator(new QIntValidator(this));
    ui->lineEditZ1->setValidator(new QIntValidator(this));
    ui->lineEditX2->setValidator(new QIntValidator(this));
    ui->lineEditZ2->setValidator(new QIntValidator(this));

    memset(biomecboxes, 0, sizeof(biomecboxes));

    SETUP_BIOME_CHECKBOX(ocean);
    SETUP_BIOME_CHECKBOX(plains);
    SETUP_BIOME_CHECKBOX(desert);
    SETUP_BIOME_CHECKBOX(mountains);
    SETUP_BIOME_CHECKBOX(forest);
    SETUP_BIOME_CHECKBOX(taiga);
    SETUP_BIOME_CHECKBOX(swamp);
    SETUP_BIOME_CHECKBOX(river);
    SETUP_BIOME_CHECKBOX(frozen_ocean);
    SETUP_BIOME_CHECKBOX(frozen_river);
    SETUP_BIOME_CHECKBOX(snowy_tundra);
    SETUP_BIOME_CHECKBOX(snowy_mountains);
    SETUP_BIOME_CHECKBOX(mushroom_fields);
    SETUP_BIOME_CHECKBOX(mushroom_field_shore);
    SETUP_BIOME_CHECKBOX(beach);
    SETUP_BIOME_CHECKBOX(desert_hills);
    SETUP_BIOME_CHECKBOX(wooded_hills);
    SETUP_BIOME_CHECKBOX(taiga_hills);
    SETUP_BIOME_CHECKBOX(mountain_edge);
    SETUP_BIOME_CHECKBOX(jungle);
    SETUP_BIOME_CHECKBOX(jungle_hills);
    SETUP_BIOME_CHECKBOX(jungle_edge);
    SETUP_BIOME_CHECKBOX(deep_ocean);
    SETUP_BIOME_CHECKBOX(stone_shore);
    SETUP_BIOME_CHECKBOX(snowy_beach);
    SETUP_BIOME_CHECKBOX(birch_forest);
    SETUP_BIOME_CHECKBOX(birch_forest_hills);
    SETUP_BIOME_CHECKBOX(dark_forest);
    SETUP_BIOME_CHECKBOX(snowy_taiga);
    SETUP_BIOME_CHECKBOX(snowy_taiga_hills);
    SETUP_BIOME_CHECKBOX(giant_tree_taiga);
    SETUP_BIOME_CHECKBOX(giant_tree_taiga_hills);
    SETUP_BIOME_CHECKBOX(wooded_mountains);
    SETUP_BIOME_CHECKBOX(savanna);
    SETUP_BIOME_CHECKBOX(savanna_plateau);
    SETUP_BIOME_CHECKBOX(badlands);
    SETUP_BIOME_CHECKBOX(wooded_badlands_plateau);
    SETUP_BIOME_CHECKBOX(badlands_plateau);
    SETUP_BIOME_CHECKBOX(warm_ocean);
    SETUP_BIOME_CHECKBOX(lukewarm_ocean);
    SETUP_BIOME_CHECKBOX(cold_ocean);
    SETUP_BIOME_CHECKBOX(deep_warm_ocean);
    SETUP_BIOME_CHECKBOX(deep_lukewarm_ocean);
    SETUP_BIOME_CHECKBOX(deep_cold_ocean);
    SETUP_BIOME_CHECKBOX(deep_frozen_ocean);

    SETUP_BIOME_CHECKBOX(sunflower_plains);
    SETUP_BIOME_CHECKBOX(desert_lakes);
    SETUP_BIOME_CHECKBOX(gravelly_mountains);
    SETUP_BIOME_CHECKBOX(flower_forest);
    SETUP_BIOME_CHECKBOX(taiga_mountains);
    SETUP_BIOME_CHECKBOX(swamp_hills);
    SETUP_BIOME_CHECKBOX(ice_spikes);
    SETUP_BIOME_CHECKBOX(modified_jungle);
    SETUP_BIOME_CHECKBOX(modified_jungle_edge);
    SETUP_BIOME_CHECKBOX(tall_birch_forest);
    SETUP_BIOME_CHECKBOX(tall_birch_hills);
    SETUP_BIOME_CHECKBOX(dark_forest_hills);
    SETUP_BIOME_CHECKBOX(snowy_taiga_mountains);
    SETUP_BIOME_CHECKBOX(giant_spruce_taiga);
    SETUP_BIOME_CHECKBOX(giant_spruce_taiga_hills);
    SETUP_BIOME_CHECKBOX(modified_gravelly_mountains);
    SETUP_BIOME_CHECKBOX(shattered_savanna);
    SETUP_BIOME_CHECKBOX(shattered_savanna_plateau);
    SETUP_BIOME_CHECKBOX(eroded_badlands);
    SETUP_BIOME_CHECKBOX(modified_wooded_badlands_plateau);
    SETUP_BIOME_CHECKBOX(modified_badlands_plateau);
    SETUP_BIOME_CHECKBOX(bamboo_jungle);
    SETUP_BIOME_CHECKBOX(bamboo_jungle_hills);

    custom = false;

    if (initcond)
    {
        cond = *initcond;

        ui->comboBoxType->setCurrentIndex(cond.type);
        ui->comboBoxRelative->setCurrentIndex(initindex);

        updateMode();

        if (!ui->groupBoxBiomes->isEnabled())
            ui->spinBox->setValue(cond.count);

        ui->lineEditX1->setText(QString::number(cond.x1));
        ui->lineEditZ1->setText(QString::number(cond.z1));
        ui->lineEditX2->setText(QString::number(cond.x2));
        ui->lineEditZ2->setText(QString::number(cond.z2));

        if (cond.x1 == cond.z1 && cond.x1 == -cond.x2 && cond.x1 == -cond.z2)
        {
            ui->lineRadius->setText(QString::number(cond.x2 * 2));
            if (ui->buttonArea->isEnabled())
            {
                custom = false;
                ui->buttonArea->setChecked(false);
            }
        }
        else
        {
            if (ui->buttonArea->isEnabled())
            {
                ui->buttonArea->setChecked(true);
                custom = true;
            }
        }

        for (int i = 0; i < 64; i++)
        {
            if ((cond.bfilter.riverToFind | cond.bfilter.oceanToFind) & (1ULL << i))
                if (biomecboxes[i])
                    biomecboxes[i]->setChecked(true);

            if (cond.bfilter.riverToFindM & (1ULL << i))
                if (biomecboxes[i+128])
                    biomecboxes[i+128]->setChecked(true);
        }

        if (cond.bfilter.riverToFind & (1ULL << (bamboo_jungle & 0x3f)))
            biomecboxes[bamboo_jungle]->setChecked(true);
        if (cond.bfilter.riverToFindM & (1ULL << (bamboo_jungle_hills - 128)))
            biomecboxes[bamboo_jungle_hills]->setChecked(true);
    }

    updateMode();
}

FilterDialog::~FilterDialog()
{
    delete ui;
}

void FilterDialog::updateMode()
{
    int filterindex = ui->comboBoxType->currentData().toInt();
    const FilterInfo &ft = g_filterinfo.list[filterindex];

    ui->groupBoxPosition->setEnabled(filterindex != F_SELECT);

    ui->buttonArea->setEnabled(ft.area);

    ui->labelSquareArea->setEnabled(!custom && ft.area);
    ui->lineRadius->setEnabled(!custom && ft.area);

    ui->labelX1->setEnabled((custom && ft.coord) || !ft.area);
    ui->labelZ1->setEnabled((custom && ft.coord) || !ft.area);
    ui->labelX2->setEnabled(custom && ft.area);
    ui->labelZ2->setEnabled(custom && ft.area);
    ui->lineEditX1->setEnabled((custom && ft.coord) || !ft.area);
    ui->lineEditZ1->setEnabled((custom && ft.coord) || !ft.area);
    ui->lineEditX2->setEnabled(custom && ft.area);
    ui->lineEditZ2->setEnabled(custom && ft.area);

    ui->labelSpinBox->setEnabled(ft.count);
    ui->spinBox->setEnabled(ft.count);

    ui->groupBoxBiomes->setEnabled(ft.biomes);
    QString s = "Location";
    if (ft.step > 1)
        s += QString::asprintf(" (rounded to a multiple of %d)", ft.step);
    ui->groupBoxPosition->setTitle(s);
    ui->textDescription->setText(ft.desription);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(filterindex != F_SELECT);
}

void FilterDialog::on_comboBoxType_activated(int)
{
    updateMode();
}

void FilterDialog::on_buttonUncheck_clicked()
{
    for (int i = 0; i < 256; i++)
        if (biomecboxes[i])
            biomecboxes[i]->setChecked(false);
}

void FilterDialog::on_buttonCheck_clicked()
{
    for (int i = 0; i < 256; i++)
        if (biomecboxes[i])
            biomecboxes[i]->setChecked(true);
}

void FilterDialog::on_buttonCheckMajor_clicked()
{
    int majorcnt = sizeof(BIOMES_L_BIOME_256) / sizeof(BIOMES_L_BIOME_256[0]);

    for (int i = 0; i < majorcnt; i++)
    {
        int b = BIOMES_L_BIOME_256[i];
        if (biomecboxes[b])
            biomecboxes[b]->setChecked(true);
    }
}

void FilterDialog::on_buttonBox_accepted()
{
    cond.type = ui->comboBoxType->currentIndex();
    cond.relative = ui->comboBoxRelative->currentData().toInt();
    cond.count = ui->spinBox->text().toInt();

    if (ui->lineRadius->isEnabled())
    {
        int d = ui->lineRadius->text().toInt();
        cond.x1 = (-d) >> 1;
        cond.z1 = (-d) >> 1;
        cond.x2 = (d) >> 1;
        cond.z2 = (d) >> 1;
    }
    else
    {
        cond.x1 = ui->lineEditX1->text().toInt();
        cond.z1 = ui->lineEditZ1->text().toInt();
        cond.x2 = ui->lineEditX2->text().toInt();
        cond.z2 = ui->lineEditZ2->text().toInt();
    }

    int r = g_filterinfo.list[cond.type].step;
    if (r)
    {
        cond.rx = (cond.x1) / r - (cond.x1 < 0);
        cond.rz = (cond.z1) / r - (cond.x1 < 0);
        cond.rw = (cond.x2 + r-1) / r - (cond.x2 + r-1 < 0) - cond.rx;
        cond.rh = (cond.z2 + r-1) / r - (cond.z2 + r-1 < 0) - cond.rz;
    }

    if (ui->groupBoxBiomes->isEnabled())
    {
        int b[256], n = 0;
        for (int i = 0; i < 256; i++)
            if (biomecboxes[i] && biomecboxes[i]->isChecked())
                b[n++] = i;

        cond.bfilter = setupBiomeFilter(b, n);
        cond.count = n;
    }
}

void FilterDialog::on_buttonArea_toggled(bool checked)
{
    custom = checked;
    updateMode();
}
