#include "filterdialog.h"
#include "ui_filterdialog.h"

#include "mainwindow.h"
#include "cutil.h"

#include <QCheckBox>
#include <QIntValidator>
#include <QColor>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QMessageBox>
#include <QScrollBar>


#define SETUP_BIOME_CHECKBOX(B) do {\
        biomecboxes[B] = new QCheckBox(biome2str(mc, B));\
        ui->gridLayoutBiomes->addWidget(biomecboxes[B], (B) % 128, (B) / 128);\
        biomecboxes[B]->setTristate(true);\
    } while (0)

#define SETUP_TEMPCAT_SPINBOX(B) do {\
        tempsboxes[B] = new SpinExclude();\
        QLabel *l = new QLabel(#B);\
        ui->gridLayoutTemps->addWidget(tempsboxes[B], (B) % Special, (B) / Special * 2 + 0);\
        ui->gridLayoutTemps->addWidget(l, (B) % Special, (B) / Special * 2 + 1);\
        l->setToolTip(getTip( MC_1_16, L_SPECIAL_1024, (B) % Special + ((B)>=Special?256:0) ));\
    } while (0)

static QString getTip(int mc, int layer, int id)
{
    uint64_t mL = 0, mM = 0;
    genPotential(&mL, &mM, layer, mc, id);
    QString tip = "Generates any of:";
    for (int j = 0; j < 64; j++)
        if (mL & (1ULL << j))
            tip += QString("\n") + biome2str(mc, j);
    for (int j = 0; j < 64; j++)
        if (mM & (1ULL << j))
            tip += QString("\n") + biome2str(mc, 128+j);
    return tip;
}

void FilterDialog::addVariant(QString name, int biome, int variant)
{
    VariantCheckBox *cb = new VariantCheckBox(name, biome, variant);
    ui->gridLayoutVariants->addWidget(cb, variantboxes.size(), 0);
    variantboxes.push_back(cb);
}

FilterDialog::FilterDialog(FormConditions *parent, Config *config, int mcversion, QListWidgetItem *item, Condition *initcond)
    : QDialog(parent, Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::FilterDialog)
    , config(config)
    , item(item)
    , mc(mcversion)
{
    memset(&cond, 0, sizeof(cond));
    ui->setupUi(this);

    textDescription = new QTextEdit(this);
    textDescription->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    ui->collapseDescription->init("Description", textDescription, true);

    QString mcs = "MC ";
    mcs += (mc2str(mc) ? mc2str(mc) : "?");
    ui->labelMC->setText(mcs);


    int initindex = -1;
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
    if (initindex < 0)
    {
        if (initcond && initcond->relative > 0)
        {
            initindex = ui->comboBoxRelative->count();
            ui->comboBoxRelative->addItem(
                QString::asprintf("[%02d] broken reference", initcond->relative), initcond->relative);
        }
        else
        {
            initindex = 0;
        }
    }

    QIntValidator *intval = new QIntValidator(this);
    ui->lineRadius->setValidator(intval);
    ui->lineEditX1->setValidator(intval);
    ui->lineEditZ1->setValidator(intval);
    ui->lineEditX2->setValidator(intval);
    ui->lineEditZ2->setValidator(intval);

    ui->lineY->setValidator(new QIntValidator(-64, 320, this));

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

    SETUP_BIOME_CHECKBOX(nether_wastes);
    SETUP_BIOME_CHECKBOX(the_end);
    SETUP_BIOME_CHECKBOX(small_end_islands);
    SETUP_BIOME_CHECKBOX(end_midlands);
    SETUP_BIOME_CHECKBOX(end_highlands);
    SETUP_BIOME_CHECKBOX(end_barrens);
    SETUP_BIOME_CHECKBOX(soul_sand_valley);
    SETUP_BIOME_CHECKBOX(crimson_forest);
    SETUP_BIOME_CHECKBOX(warped_forest);
    SETUP_BIOME_CHECKBOX(basalt_deltas);

    SETUP_BIOME_CHECKBOX(dripstone_caves);
    SETUP_BIOME_CHECKBOX(lush_caves);
    SETUP_BIOME_CHECKBOX(meadow);
    SETUP_BIOME_CHECKBOX(grove);
    SETUP_BIOME_CHECKBOX(snowy_slopes);
    SETUP_BIOME_CHECKBOX(stony_peaks);
    SETUP_BIOME_CHECKBOX(jagged_peaks);
    SETUP_BIOME_CHECKBOX(frozen_peaks);

    memset(tempsboxes, 0, sizeof(tempsboxes));

    SETUP_TEMPCAT_SPINBOX(Oceanic);
    SETUP_TEMPCAT_SPINBOX(Warm);
    SETUP_TEMPCAT_SPINBOX(Lush);
    SETUP_TEMPCAT_SPINBOX(Cold);
    SETUP_TEMPCAT_SPINBOX(Freezing);
    SETUP_TEMPCAT_SPINBOX(Special+Warm);
    SETUP_TEMPCAT_SPINBOX(Special+Lush);
    SETUP_TEMPCAT_SPINBOX(Special+Cold);

    addVariant("plains_fountain_01", plains, 0);
    addVariant("plains_meeting_point_1", plains, 1);
    addVariant("plains_meeting_point_2", plains, 2);
    addVariant("plains_meeting_point_3", plains, 3);
    addVariant("desert_meeting_point_1", desert, 1);
    addVariant("desert_meeting_point_2", desert, 2);
    addVariant("desert_meeting_point_3", desert, 3);
    addVariant("savanna_meeting_point_1", savanna, 1);
    addVariant("savanna_meeting_point_2", savanna, 2);
    addVariant("savanna_meeting_point_3", savanna, 3);
    addVariant("savanna_meeting_point_4", savanna, 4);
    addVariant("taiga_meeting_point_1", taiga, 1);
    addVariant("taiga_meeting_point_2", taiga, 2);
    addVariant("snowy_meeting_point_1", snowy_tundra, 1);
    addVariant("snowy_meeting_point_2", snowy_tundra, 2);
    addVariant("snowy_meeting_point_3", snowy_tundra, 3);

    ui->scrollBiomes->setStyleSheet(
            "QCheckBox::indicator:unchecked     { image: url(:/icons/check0.png); }\n"
            "QCheckBox::indicator:indeterminate { image: url(:/icons/check1.png); }\n"
            "QCheckBox::indicator:checked       { image: url(:/icons/check2.png); }\n"
            );

    QPixmap pixmap(14,14);
    pixmap.fill(QColor(0,0,0,0));
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(QRectF(1, 1, 12, 12), 3, 3);
    QPen pen(Qt::black, 1);
    p.setPen(pen);
    for (int i = 0; i < 256; i++)
    {
        QCheckBox *cb = biomecboxes[i];
        if (!cb)
            continue;
        QColor col(biomeColors[i][0], biomeColors[i][1], biomeColors[i][2]);
        p.fillPath(path, col);
        p.drawPath(path);
        biomecboxes[i]->setIcon(QIcon(pixmap));
    }

    custom = false;

    if (initcond)
    {
        cond = *initcond;
        const FilterInfo &ft = g_filterinfo.list[cond.type];

        ui->comboBoxCat->setCurrentIndex(ft.cat);
        for (int i = 0; i < ui->comboBoxType->count(); i++)
        {
            int type = ui->comboBoxType->itemData(i, Qt::UserRole).toInt();
            if (type == cond.type)
            {
                ui->comboBoxType->setCurrentIndex(i);
                break;
            }
        }

        ui->comboBoxRelative->setCurrentIndex(initindex);

        updateMode();

        if (!ui->tabWidget->isEnabled())
            ui->spinBox->setValue(cond.count);

        ui->lineEditX1->setText(QString::number(cond.x1));
        ui->lineEditZ1->setText(QString::number(cond.z1));
        ui->lineEditX2->setText(QString::number(cond.x2));
        ui->lineEditZ2->setText(QString::number(cond.z2));

        ui->checkApprox->setChecked(cond.approx);
        ui->lineY->setText(QString::number(cond.y));

        if (cond.x1 == cond.z1 && cond.x1 == -cond.x2 && cond.x1 == -cond.z2)
        {
            ui->lineRadius->setText(QString::number(cond.x2 * 2));
            if (ui->buttonArea->isEnabled())
            {
                custom = false;
                ui->buttonArea->setChecked(false);
            }
        }
        else if (cond.x1 == cond.z1 && cond.x1+1 == -cond.x2 && cond.x1+1 == -cond.z2)
        {
            ui->lineRadius->setText(QString::number(cond.x2 * 2 + 1));
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

        // remember bamboo_jungle=168 has a bit at (bamboo_jungle & 0x3f) in cond.bfilter.edgesToFind
        for (int i = 0; i < 64; i++)
        {
            if (biomecboxes[i])
            {
                bool c1 = (cond.bfilter.riverToFind | cond.bfilter.oceanToFind) & (1ULL << i);
                bool c2 = cond.bfilter.biomeToExcl & (1ULL << i);
                biomecboxes[i]->setCheckState(c2 ? Qt::Checked : c1 ? Qt::PartiallyChecked : Qt::Unchecked);
            }

            if (biomecboxes[i+128])
            {
                bool c1 = (cond.bfilter.riverToFindM) & (1ULL << i);
                bool c2 = cond.bfilter.biomeToExclM & (1ULL << i);
                biomecboxes[i+128]->setCheckState(c2 ? Qt::Checked : c1 ? Qt::PartiallyChecked : Qt::Unchecked);
            }
        }
        for (int i = 0; i < 9; i++)
        {
            if (tempsboxes[i])
            {
                tempsboxes[i]->setValue(cond.temps[i]);
            }
        }

        ui->checkStartPiece->setChecked(cond.variants & Condition::START_PIECE_MASK);
        ui->checkAbandoned->setChecked(cond.variants & Condition::ABANDONED_MASK);
        for (VariantCheckBox *cb : variantboxes)
        {
            cb->setChecked(cond.variants & cb->getMask());
        }
    }

    on_lineRadius_editingFinished();

    updateMode();
}

FilterDialog::~FilterDialog()
{
    if (item)
        delete item;
    delete ui;
}

void FilterDialog::updateMode()
{
    int filterindex = ui->comboBoxType->currentData().toInt();
    const FilterInfo &ft = g_filterinfo.list[filterindex];

    QPalette pal;
    if (mc < ft.mcmin || mc > ft.mcmax)
        pal.setColor(QPalette::Normal, QPalette::Button, QColor(255,0,0,127));
    ui->comboBoxType->setPalette(pal);


    ui->groupBoxPosition->setEnabled(filterindex != F_SELECT);

    ui->buttonArea->setEnabled(ft.area);

    ui->labelSquareArea->setEnabled(!custom && ft.area);
    ui->lineRadius->setEnabled(!custom && ft.area);

    ui->labelX1->setEnabled(ft.coord && (custom || !ft.area));
    ui->labelZ1->setEnabled(ft.coord && (custom || !ft.area));
    ui->labelX2->setEnabled(custom && ft.area);
    ui->labelZ2->setEnabled(custom && ft.area);
    ui->lineEditX1->setEnabled(ft.coord && (custom || !ft.area));
    ui->lineEditZ1->setEnabled(ft.coord && (custom || !ft.area));
    ui->lineEditX2->setEnabled(custom && ft.area);
    ui->lineEditZ2->setEnabled(custom && ft.area);

    ui->labelSpinBox->setEnabled(ft.count);
    ui->spinBox->setEnabled(ft.count);

    ui->labelY->setEnabled(ft.hasy);
    ui->lineY->setEnabled(ft.hasy);

    if (filterindex == F_TEMPS)
    {
        ui->tabWidget->setEnabled(true);
        ui->tabWidget->setCurrentWidget(ui->tabTemps);
        ui->tabTemps->setEnabled(true);
        ui->tabBiomes->setEnabled(false);
        ui->tabVariants->setEnabled(false);
    }
    else if (ft.cat == CAT_BIOMES || ft.cat == CAT_NETHER || ft.cat == CAT_END)
    {
        ui->tabWidget->setEnabled(true);
        ui->tabWidget->setCurrentWidget(ui->tabBiomes);
        ui->tabTemps->setEnabled(false);
        ui->tabBiomes->setEnabled(true);
        ui->tabVariants->setEnabled(false);
    }
    else if (filterindex == F_VILLAGE)
    {
        ui->tabWidget->setEnabled(true);
        ui->tabWidget->setCurrentWidget(ui->tabVariants);
        ui->tabTemps->setEnabled(false);
        ui->tabBiomes->setEnabled(false);
        ui->tabVariants->setEnabled(true);
    }
    else
    {
        ui->tabWidget->setEnabled(false);
        ui->tabTemps->setEnabled(false);
        ui->tabBiomes->setEnabled(false);
        ui->tabVariants->setEnabled(false);
    }

    ui->checkStartPiece->setEnabled(mc >= MC_1_14);
    ui->scrollVariants->setEnabled(mc >= MC_1_14 && (cond.variants & Condition::START_PIECE_MASK));

    updateBiomeSelection();

    QString loc = "";
    QString areatip = "";
    QString lowtip = "";
    QString uptip = "";

    if (ft.step > 1)
    {
        loc = QString::asprintf("Location (coordinates are multiplied by x%d)", ft.step);
        areatip = QString::asprintf("From floor(-[S] / 2) x%d to floor([S] / 2) x%d on both axes (inclusive)", ft.step, ft.step);
        lowtip = QString::asprintf("Lower bound x%d (inclusive)", ft.step);
        uptip = QString::asprintf("Upper bound x%d (inclusive)", ft.step);
    }
    else
    {
        loc = "Location";
        areatip = "From floor(-[S] / 2) to floor([S] / 2) on both axes (inclusive)";
        lowtip = QString::asprintf("Lower bound (inclusive)");
        uptip = QString::asprintf("Upper bound (inclusive)");
    }
    ui->groupBoxPosition->setTitle(loc);
    ui->labelSquareArea->setToolTip(areatip);
    ui->labelX1->setToolTip(lowtip);
    ui->labelZ1->setToolTip(lowtip);
    ui->labelX2->setToolTip(uptip);
    ui->labelZ2->setToolTip(uptip);
    ui->lineEditX1->setToolTip(lowtip);
    ui->lineEditZ1->setToolTip(lowtip);
    ui->lineEditX2->setToolTip(uptip);
    ui->lineEditZ2->setToolTip(uptip);
    ui->buttonOk->setEnabled(filterindex != F_SELECT);
    textDescription->setText(ft.description);
}

void FilterDialog::enableSet(const int *ids, int n)
{
    for (int i = 0; i < 256; i++)
    {
        if (biomecboxes[i])
        {
            biomecboxes[i]->setEnabled(false);
            biomecboxes[i]->setToolTip("");
        }
    }
    for (int i = 0; i < n; i++)
        biomecboxes[ids[i]]->setEnabled(true);
}

void FilterDialog::updateBiomeSelection()
{
    int filterindex = ui->comboBoxType->currentData().toInt();
    const FilterInfo &ft = g_filterinfo.list[filterindex];

    if (ft.cat == CAT_NETHER)
    {
        const int ids[] = {
            nether_wastes, soul_sand_valley, crimson_forest,
            warped_forest, basalt_deltas
        };
        enableSet(ids, sizeof(ids) / sizeof(int));
    }
    else if (ft.cat == CAT_END)
    {
        const int ids[] = {
            the_end, small_end_islands, end_midlands,
            end_highlands, end_barrens
        };
        enableSet(ids, sizeof(ids) / sizeof(int));
    }
    if (filterindex == F_BIOME_256_OTEMP)
    {
        const int ids[] = {
            warm_ocean, lukewarm_ocean, ocean,
            cold_ocean, frozen_ocean
        };
        enableSet(ids, sizeof(ids) / sizeof(int));
    }
    else if (ft.cat == CAT_BIOMES && mc <= MC_1_17)
    {
        ui->labelY->setEnabled(false);
        ui->lineY->setEnabled(false);

        int layerId = ft.layer;
        if (layerId == 0)
        {
            Generator tmp;
            tmp.mc = mc;
            const Layer *l = getLayerForScale(&tmp, ft.step);
            if (l)
                layerId = l - tmp.ls.layers;
        }
        if (layerId <= 0 || layerId >= L_NUM)
            return; // error

        for (int i = 0; i < 256; i++)
        {
            QCheckBox *cb = biomecboxes[i];
            if (!cb)
                continue;

            uint64_t mL = 0, mM = 0;
            genPotential(&mL, &mM, layerId, mc, i);
            if (mL || mM)
            {
                cb->setEnabled(true);
                if (ft.layer != L_VORONOI_1)
                {
                    QString tip = "Generates any of:";
                    for (int j = 0; j < 64; j++)
                    {
                        if (mL & (1ULL << j))
                            tip += QString("\n") + biome2str(mc, j);
                    }
                    for (int j = 0; j < 64; j++)
                    {
                        if (mM & (1ULL << j))
                            tip += QString("\n") + biome2str(mc, j+128);
                    }
                    cb->setToolTip(tip);
                }
                else
                {
                    cb->setToolTip(cb->text());
                }
            }
            else
            {
                cb->setEnabled(false);
                cb->setToolTip("");
            }
        }
    }
    else if (ft.cat == CAT_BIOMES && ft.dim == 0)
    {
        for (int i = 0; i < 256; i++)
        {
            QCheckBox *cb = biomecboxes[i];
            if (!cb)
                continue;
            cb->setEnabled(isOverworld(mc, i));
            cb->setToolTip("");
        }
    }
}

int FilterDialog::warnIfBad(Condition cond)
{
    const FilterInfo &ft = g_filterinfo.list[cond.type];
    if (ui->scrollVariants->isEnabled())
    {
        if ((cond.variants & ((1ULL << 60) - 1)) == 0)
        {
            QString text =
                    "No allowed start pieces specified. Condition can never be true.";
            QMessageBox::warning(this, "Invalid condition", text, QMessageBox::Ok);
            return QMessageBox::Cancel;
        }
    }
    if (ft.cat == CAT_BIOMES)
    {
        int w = cond.x2 - cond.x1 + 1;
        int h = cond.z2 - cond.z1 + 1;
        uint64_t workitemsize = (uint64_t)config->seedsPerItem * w * h;
        uint64_t workwarn = (mc >= MC_1_18 ? 1e6 : 1e9);

        if (workitemsize > workwarn)
        {
            QString text =
                    "The biome filter you have entered may take a while to check. "
                    "You should consider using a smaller area with a larger scaling "
                    "instead and/or reduce the number of seeds per work item "
                    "under Edit>Perferences."
                    "\n\n"
                    "Are you sure you want to continue?";
            return QMessageBox::warning(this, "Performance Expensive Condition", text, QMessageBox::Ok | QMessageBox::Cancel);
        }
    }
    return QMessageBox::Ok;
}

void FilterDialog::on_comboBoxType_activated(int)
{
    updateMode();
}

void FilterDialog::on_comboBoxRelative_activated(int)
{
    QPalette pal;
    if (ui->comboBoxRelative->currentText().contains("broken"))
        pal.setColor(QPalette::Normal, QPalette::Button, QColor(255,0,0,127));
    ui->comboBoxRelative->setPalette(pal);
}

void FilterDialog::on_buttonUncheck_clicked()
{
    for (int i = 0; i < 256; i++)
    {
        QCheckBox *cb = biomecboxes[i];
        if (cb)
            cb->setCheckState(Qt::Unchecked);
    }
}

void FilterDialog::on_buttonInclude_clicked()
{
    for (int i = 0; i < 256; i++)
    {
        QCheckBox *cb = biomecboxes[i];
        if (cb)
            cb->setCheckState(cb->isEnabled() ? Qt::PartiallyChecked : Qt::Unchecked);
    }
}

void FilterDialog::on_buttonExclude_clicked()
{
    for (int i = 0; i < 256; i++)
    {
        QCheckBox *cb = biomecboxes[i];
        if (cb)
            cb->setCheckState(cb->isEnabled() ? Qt::Checked : Qt::Unchecked);
    }
}

void FilterDialog::on_buttonArea_toggled(bool checked)
{
    custom = checked;
    updateMode();
}

void FilterDialog::on_lineRadius_editingFinished()
{
    int v = ui->lineRadius->text().toInt();
    int area = (v+1) * (v+1);
    for (int i = 0; i < 9; i++)
    {
        if (tempsboxes[i])
            tempsboxes[i]->setMaximum(area);
    }
}

void FilterDialog::on_buttonCancel_clicked()
{
    close();
}

void FilterDialog::on_buttonOk_clicked()
{
    cond.type = ui->comboBoxType->currentData().toInt();
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

    if (ui->tabBiomes->isEnabled())
    {
        int bin[256], bex[256], in = 0, ex = 0;
        for (int i = 0; i < 256; i++)
        {
            QCheckBox *cb = biomecboxes[i];
            if (cb && cb->isEnabled())
            {
                if (cb->checkState() == Qt::PartiallyChecked)
                    bin[in++] = i;
                if (cb->checkState() == Qt::Checked)
                    bex[ex++] = i;
            }
        }
        cond.bfilter = setupBiomeFilter(bin, in, bex, ex);
        cond.count = in + ex;
    }
    if (ui->tabTemps->isEnabled())
    {
        cond.count = 0;
        for (int i = 0; i < 9; i++)
        {
            if (!tempsboxes[i])
                continue;
            int cnt = tempsboxes[i]->value();
            cond.temps[i] = cnt;
            if (cnt > 0)
                cond.count += cnt;
        }
    }

    cond.y = ui->lineY->text().toInt();

    cond.approx = ui->checkApprox->isChecked();

    cond.variants = 0;
    cond.variants |= ui->checkStartPiece->isChecked() * Condition::START_PIECE_MASK;
    cond.variants |= ui->checkAbandoned->isChecked() * Condition::ABANDONED_MASK;
    for (VariantCheckBox *cb : variantboxes)
        if (cb->isChecked())
            cond.variants |= cb->getMask();

    if (warnIfBad(cond) != QMessageBox::Ok)
        return;

    emit setCond(item, cond);
    item = 0;
    close();
}

void FilterDialog::on_FilterDialog_finished(int)
{
    if (item)
        emit setCond(item, cond);
    item = 0;
}

void FilterDialog::on_comboBoxCat_currentIndexChanged(int idx)
{
    ui->comboBoxType->setEnabled(idx != CAT_NONE);
    ui->comboBoxType->clear();

    int slot = 0;
    ui->comboBoxType->insertItem(slot, "Select filter", QVariant::fromValue((int)F_SELECT));

    const FilterInfo *ft_list[FILTER_MAX] = {};
    const FilterInfo *ft;

    for (int i = 1; i < FILTER_MAX; i++)
    {
        ft = &g_filterinfo.list[i];
        if (ft->cat == idx)
            ft_list[ft->disp] = ft;
    }

    for (int i = 1; i < FILTER_MAX; i++)
    {
        ft = ft_list[i];
        if (!ft)
            continue;
        slot++;
        QVariant vidx = QVariant::fromValue((int)(ft - g_filterinfo.list));
        if (ft->icon)
            ui->comboBoxType->insertItem(slot, QIcon(ft->icon), ft->name, vidx);
        else
            ui->comboBoxType->insertItem(slot, ft->name, vidx);

        if (mc < ft->mcmin || mc > ft->mcmax)
            ui->comboBoxType->setItemData(slot, false, Qt::UserRole-1); // deactivate
        if (ft == g_filterinfo.list + F_FORTRESS)
            ui->comboBoxType->insertSeparator(slot++);
        if (ft == g_filterinfo.list + F_ENDCITY)
            ui->comboBoxType->insertSeparator(slot++);
    }

    updateMode();
}

void FilterDialog::on_checkStartPiece_stateChanged(int state)
{
    ui->scrollVariants->setEnabled(state);
}

