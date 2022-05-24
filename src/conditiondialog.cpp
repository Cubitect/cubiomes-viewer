#include "conditiondialog.h"
#include "ui_conditiondialog.h"

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
    QString tip = ConditionDialog::tr("Generates any of:");
    for (int j = 0; j < 64; j++)
        if (mL & (1ULL << j))
            tip += QString("\n") + biome2str(mc, j);
    for (int j = 0; j < 64; j++)
        if (mM & (1ULL << j))
            tip += QString("\n") + biome2str(mc, 128+j);
    return tip;
}

void ConditionDialog::addVariant(QString name, int biome, int variant)
{
    VariantCheckBox *cb = new VariantCheckBox(name, biome, variant);
    ui->gridLayoutVariants->addWidget(cb, variantboxes.size(), 0);
    variantboxes.push_back(cb);
}

#define WARNING_CHAR QChar(0x26A0)

ConditionDialog::ConditionDialog(FormConditions *parent, Config *config, int mcversion, QListWidgetItem *item, Condition *initcond)
    : QDialog(parent, Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::ConditionDialog)
    , config(config)
    , item(item)
    , mc(mcversion)
{
    memset(&cond, 0, sizeof(cond));
    ui->setupUi(this);

    textDescription = new QTextEdit(this);
    textDescription->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    ui->collapseDescription->init(tr("Description/Notes"), textDescription, true);

    const char *p_mcs = mc2str(mc);
    QString mcs = tr("MC %1", "Minecraft version").arg(p_mcs ? p_mcs : "?");
    ui->labelMC->setText(mcs);


    int initindex = -1;
    QVector<Condition> existing = parent->getConditions();
    for (Condition c : existing)
    {
        if (initcond)
        {
            if (c.save == initcond->save)
                continue;
            if (c.save == initcond->relative)
                initindex = ui->comboBoxRelative->count();
        }
        QString condstr = c.summary().simplified();
        ui->comboBoxRelative->addItem(condstr, c.save);
    }
    if (initindex < 0)
    {
        if (initcond && initcond->relative > 0)
        {
            initindex = ui->comboBoxRelative->count();
            QString condstr = QString("[%1] %2 broken reference")
                .arg(initcond->relative, 2, 10, QLatin1Char('0'))
                .arg(WARNING_CHAR);
            ui->comboBoxRelative->addItem(condstr, initcond->relative);
        }
        else
        {
            initindex = 0;
        }
    }

    QIntValidator *intval = new QIntValidator(this);
    ui->lineEditX1->setValidator(intval);
    ui->lineEditZ1->setValidator(intval);
    ui->lineEditX2->setValidator(intval);
    ui->lineEditZ2->setValidator(intval);

    QIntValidator *uintval = new QIntValidator(0, INT_MAX, this);
    ui->lineSquare->setValidator(uintval);
    ui->lineRadius->setValidator(uintval);

    ui->comboY->lineEdit()->setValidator(new QIntValidator(-64, 320, this));

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

    SETUP_BIOME_CHECKBOX(deep_dark);
    SETUP_BIOME_CHECKBOX(mangrove_swamp);

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

    QString tristyle =
        "QCheckBox::indicator:unchecked     { image: url(:/icons/check0.png); }\n"
        "QCheckBox::indicator:indeterminate { image: url(:/icons/check1.png); }\n"
        "QCheckBox::indicator:checked       { image: url(:/icons/check2.png); }\n";
    ui->scrollBiomes->setStyleSheet(tristyle);
    ui->scrollNoiseBiomes->setStyleSheet(tristyle);

    memset(climaterange, 0, sizeof(climaterange));
    memset(climatecomplete, 0, sizeof(climatecomplete));
    struct { QString name; int idx; int min, max; } climates[] =
    {
        {tr("Temperature:"), 0, -4501, 5500},
        {tr("Humidity:"), 1, -3500, 6999},
        {tr("Continentalness:"), 2, -10500, 300},
        {tr("Erosion:"), 3, -7799, 5500},
        // depth has more dependencies and is not supported
        {tr("Weirdness:"), 5, -9333, 9333},
    };
    for (int i = 0; i < 5; i++)
    {
        QLabel *label = new QLabel(climates[i].name, this);
        LabeledRange *ok = new LabeledRange(this, climates[i].min-1, climates[i].max+1);
        LabeledRange *ex = new LabeledRange(this, climates[i].min-1, climates[i].max+1);
        ok->setLimitText(tr("-Inf"), tr("+Inf"));
        ex->setLimitText(tr("-Inf"), tr("+Inf"));
        ex->setHighlight(QColor(0,0,0,0), QColor(Qt::red));
        connect(ok, SIGNAL(onRangeChange(void)), this, SLOT(onClimateLimitChanged(void)));
        connect(ex, SIGNAL(onRangeChange(void)), this, SLOT(onClimateLimitChanged(void)));
        climaterange[0][climates[i].idx] = ok;
        climaterange[1][climates[i].idx] = ex;

        QCheckBox *all = new QCheckBox(this);
        all->setFixedWidth(20);
        all->setToolTip(tr("Require full range instead of intersection"));
        connect(all, SIGNAL(stateChanged(int)), this, SLOT(onClimateLimitChanged(void)));
        climatecomplete[climates[i].idx] = all;

        int row = ui->gridNoiseAllowed->rowCount();
        ui->gridNoiseName->addWidget(label, row, 0, 1, 2);
        ui->gridNoiseRequired->addWidget(all, row, 0, 1, 1);
        ui->gridNoiseRequired->addWidget(ok, row, 1, 1, 2);
        ui->gridNoiseAllowed->addWidget(ex, row, 0, 1, 2);
    }

    for (int i = 0; i < 256; i++)
    {
        const int *lim = getBiomeParaLimits(mc, i);
        if (lim == NULL)
            continue;
        NoiseBiomeIndicator *cb = new NoiseBiomeIndicator(biome2str(mc, i), this);
        QString tip = "<pre>";
        for (int j = 0; j < 5; j++)
        {
            tip += climates[j].name.leftJustified(18);
            const int *l = lim + 2 * climates[j].idx;
            tip += (l[0] == INT_MIN) ? tr("  -Inf") : QString::asprintf("%6d", (int)l[0]);
            tip += " - ";
            tip += (l[1] == INT_MAX) ? tr("  +Inf") : QString::asprintf("%6d", (int)l[1]);
            if (j < 4) tip += "\n";
        }
        tip += "</pre>";
        cb->setFocusPolicy(Qt::NoFocus);
        cb->setCheckState(Qt::Unchecked);
        cb->setToolTip(tip);
        cb->setTristate(true);
        int cols = 3;
        int n = noisebiomes.size();
        ui->gridNoiseBiomes->addWidget(cb, n/cols, n%cols);
        noisebiomes[i] = cb;
    }

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
        if (noisebiomes.find(i) != noisebiomes.end())
            noisebiomes[i]->setIcon(QIcon(pixmap));
    }

    // defaults
    ui->spinBox->setValue(1);
    ui->radioSquare->setChecked(true);
    ui->checkRadius->setChecked(false);

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
        on_comboBoxRelative_activated(initindex);

        updateMode();

        ui->spinBox->setValue(cond.count);
        ui->lineEditX1->setText(QString::number(cond.x1));
        ui->lineEditZ1->setText(QString::number(cond.z1));
        ui->lineEditX2->setText(QString::number(cond.x2));
        ui->lineEditZ2->setText(QString::number(cond.z2));

        ui->checkApprox->setChecked(cond.flags & CFB_APPROX);
        ui->checkMatchAny->setChecked(cond.flags & CFB_MATCH_ANY);
        int i, n = ui->comboY->count();
        for (i = 0; i < n; i++)
            if (ui->comboY->itemText(i).section(' ', 0, 0).toInt() == cond.y)
                break;
        if (i >= n)
            ui->comboY->addItem(QString::number(cond.y));
        ui->comboY->setCurrentIndex(i);

        if (cond.x1 == cond.z1 && cond.x1 == -cond.x2 && cond.x1 == -cond.z2)
        {
            ui->lineSquare->setText(QString::number(cond.x2 * 2));
            ui->radioSquare->setChecked(true);
        }
        else if (cond.x1 == cond.z1 && cond.x1+1 == -cond.x2 && cond.x1+1 == -cond.z2)
        {
            ui->lineSquare->setText(QString::number(cond.x2 * 2 + 1));
            ui->radioSquare->setChecked(true);
        }
        else
        {
            ui->radioCustom->setChecked(true);
        }

        if (cond.rmax > 0)
        {
            ui->lineRadius->setText(QString::number(cond.rmax - 1));
            ui->checkRadius->setChecked(true);
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

        int *lim = (int*) &cond.limok[0][0];
        if (lim[0] == 0 && !memcmp(lim, lim + 1, (6 * 4 - 1) * sizeof(int)))
        {   // limits are all zero -> assume uninitialzed
            for (int i = 0; i < 6; i++)
            {
                cond.limok[i][0] = cond.limex[i][0] = INT_MIN;
                cond.limok[i][1] = cond.limex[i][1] = INT_MAX;
            }
        }
        setClimateLimits(climaterange[0], cond.limok, true);
        setClimateLimits(climaterange[1], cond.limex, false);
    }

    on_lineSquare_editingFinished();

    onClimateLimitChanged();
    updateMode();
}

ConditionDialog::~ConditionDialog()
{
    if (item)
        delete item;
    delete ui;
}

void ConditionDialog::setActiveTab(QWidget *tab)
{
    ui->tabWidget->setEnabled(true);
    ui->tabWidget->setCurrentWidget(tab);
    ui->tabBiomes->setEnabled(ui->tabBiomes == tab);
    ui->tabTemps->setEnabled(ui->tabTemps == tab);
    ui->tabNoise->setEnabled(ui->tabNoise == tab);
    ui->tabVariants->setEnabled(ui->tabVariants == tab);
}

void ConditionDialog::updateMode()
{
    int filterindex = ui->comboBoxType->currentData().toInt();
    const FilterInfo &ft = g_filterinfo.list[filterindex];

    QPalette pal;
    if (mc < ft.mcmin || mc > ft.mcmax)
        pal.setColor(QPalette::Normal, QPalette::Button, QColor(255,0,0,127));
    ui->comboBoxType->setPalette(pal);


    ui->groupBoxPosition->setEnabled(filterindex != F_SELECT);


    ui->checkRadius->setEnabled(ft.rmax);

    if (ui->checkRadius->isEnabled() && ui->checkRadius->isChecked())
    {
        ui->lineRadius->setEnabled(true);

        ui->radioSquare->setEnabled(false);
        ui->radioCustom->setEnabled(false);

        ui->lineSquare->setEnabled(false);

        ui->labelX1->setEnabled(false);
        ui->labelZ1->setEnabled(false);
        ui->labelX2->setEnabled(false);
        ui->labelZ2->setEnabled(false);
        ui->lineEditX1->setEnabled(false);
        ui->lineEditZ1->setEnabled(false);
        ui->lineEditX2->setEnabled(false);
        ui->lineEditZ2->setEnabled(false);
    }
    else
    {
        bool custom = ui->radioCustom->isChecked();

        ui->lineRadius->setEnabled(false);

        ui->radioSquare->setEnabled(ft.area);
        ui->radioCustom->setEnabled(ft.area);

        ui->lineSquare->setEnabled(!custom && ft.area);

        ui->labelX1->setEnabled(ft.coord && (custom || !ft.area));
        ui->labelZ1->setEnabled(ft.coord && (custom || !ft.area));
        ui->labelX2->setEnabled(custom && ft.area);
        ui->labelZ2->setEnabled(custom && ft.area);
        ui->lineEditX1->setEnabled(ft.coord && (custom || !ft.area));
        ui->lineEditZ1->setEnabled(ft.coord && (custom || !ft.area));
        ui->lineEditX2->setEnabled(custom && ft.area);
        ui->lineEditZ2->setEnabled(custom && ft.area);
    }

    ui->labelSpinBox->setEnabled(ft.count);
    ui->spinBox->setEnabled(ft.count);

    ui->labelY->setEnabled(ft.hasy);
    ui->comboY->setEnabled(ft.hasy);

    if (filterindex == F_TEMPS)
    {
        setActiveTab(ui->tabTemps);
    }
    else if (filterindex == F_CLIMATE_NOISE)
    {
        setActiveTab(ui->tabNoise);
    }
    else if (ft.cat == CAT_BIOMES || ft.cat == CAT_NETHER || ft.cat == CAT_END)
    {
        setActiveTab(ui->tabBiomes);
        ui->checkApprox->setEnabled(mc <= MC_1_17 || ft.step == 4);
        ui->checkMatchAny->setEnabled(mc >= MC_1_18);
    }
    else if (filterindex == F_VILLAGE)
    {
        setActiveTab(ui->tabVariants);
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
        QString multxt = QString("%1%2").arg(QChar(0xD7)).arg(ft.step);
        loc = tr("Location (coordinates are multiplied by %1)").arg(multxt);
        areatip = tr("From floor(-x/2)%1 to floor(x/2)%1 on both axes (inclusive)").arg(multxt);
        lowtip = tr("Lower bound %1 (inclusive)").arg(multxt);
        uptip = tr("Upper bound %1 (inclusive)").arg(multxt);
    }
    else
    {
        loc = tr("Location");
        areatip = tr("From floor(-x/2) to floor(x/2) on both axes (inclusive)");
        lowtip = tr("Lower bound (inclusive)");
        uptip = tr("Upper bound (inclusive)");
    }
    ui->groupBoxPosition->setTitle(loc);
    ui->radioSquare->setToolTip(areatip);
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

void ConditionDialog::enableSet(const int *ids, int n)
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

void ConditionDialog::updateBiomeSelection()
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
        ui->comboY->setEnabled(false);

        int layerId = ft.layer;
        if (layerId == 0)
        {
            Generator tmp;
            setupGenerator(&tmp, mc, 0);
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
                    QString tip = tr("Generates any of:");
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

int ConditionDialog::warnIfBad(Condition cond)
{
    const FilterInfo &ft = g_filterinfo.list[cond.type];
    if (ui->scrollVariants->isEnabled())
    {
        if ((cond.variants & ((1ULL << 60) - 1)) == 0)
        {
            QString text = tr("No allowed start pieces specified. Condition can never be true.");
            QMessageBox::warning(this, tr("Missing Start Piece"), text, QMessageBox::Ok);
            return QMessageBox::Cancel;
        }
    }
    if (cond.type == F_CLIMATE_NOISE)
    {
        for (int i = 0; i < 6; i++)
        {
            if (cond.limok[i][0] == INT_MAX || cond.limok[i][1] == INT_MIN)
            {
                QString text = tr(
                    "The condition contains a climate range which is unbounded "
                    "with the full range required, which can never be satisfied."
                    );
                QMessageBox::warning(this, tr("Bad Climate Range"), text,
                    QMessageBox::Ok);
                return QMessageBox::Cancel;
            }
        }
    }
    else if (ft.cat == CAT_BIOMES)
    {
        if (mc >= MC_1_18)
        {
            uint64_t m = cond.bfilter.riverToFindM;
            uint64_t underground = (1ULL << (dripstone_caves-128)) | (1ULL << (lush_caves-128));
            if ((m & underground) && cond.y > 246)
            {
                return QMessageBox::warning(this, tr("Bad Surface Height"),
                    tr("Cave biomes do not generate above Y = 246. "
                    "You should consider lowering the sampling height."
                    "\n\n"
                    "Continue anyway?"),
                    QMessageBox::Ok | QMessageBox::Cancel);
            }
        }
    }
    return QMessageBox::Ok;
}

void ConditionDialog::on_comboBoxType_activated(int)
{
    updateMode();
}

void ConditionDialog::on_comboBoxRelative_activated(int)
{
    QPalette pal;
    if (ui->comboBoxRelative->currentText().contains(WARNING_CHAR))
        pal.setColor(QPalette::Normal, QPalette::Button, QColor(255,0,0,127));
    ui->comboBoxRelative->setPalette(pal);
}

void ConditionDialog::on_buttonUncheck_clicked()
{
    for (int i = 0; i < 256; i++)
    {
        QCheckBox *cb = biomecboxes[i];
        if (cb)
            cb->setCheckState(Qt::Unchecked);
    }
}

void ConditionDialog::on_buttonInclude_clicked()
{
    for (int i = 0; i < 256; i++)
    {
        QCheckBox *cb = biomecboxes[i];
        if (cb)
            cb->setCheckState(cb->isEnabled() ? Qt::PartiallyChecked : Qt::Unchecked);
    }
}

void ConditionDialog::on_buttonExclude_clicked()
{
    for (int i = 0; i < 256; i++)
    {
        QCheckBox *cb = biomecboxes[i];
        if (cb)
            cb->setCheckState(cb->isEnabled() ? Qt::Checked : Qt::Unchecked);
    }
}

void ConditionDialog::on_buttonAreaInfo_clicked()
{
    QMessageBox mb(this);
    mb.setIcon(QMessageBox::Information);
    mb.setWindowTitle(tr("Help: area entry"));
    mb.setText(tr(
        "<html><head/><body><p>"
        "The area can be entered via <b>custom</b> rectangle, that is defined "
        "by its two opposing corners, relative to a center point. These bounds "
        "are inclusive."
        "</p><p>"
        "Alternatively, the area can be defined as a <b>centered square</b> "
        "with a certain side length. In this case the area has the bounds: "
        "[-X/2, -X/2] on both axes, rounding down and bounds included. For "
        "example a centered square with side 3 will go from -2 to 1 for both "
        "the X and Z axes."
        "</p><p>"
        "Important to note is that some filters have a scaling associtated with "
        "them. This means that the area is not defined in blocks, but on a grid "
        "with the given spacing (such as chunks instead of blocks). A scaling "
        "of 1:16, for example, means that the aformentioned centered square of "
        "side 3 will range from -32 to 31 in block coordinates. (Chunk 1 has "
        "blocks 16 to 31.)"
        "</p></body></html>"
        ));
    mb.exec();
}

void ConditionDialog::on_checkRadius_toggled(bool)
{
    updateMode();
}

void ConditionDialog::on_radioSquare_toggled(bool)
{
    updateMode();
}

void ConditionDialog::on_radioCustom_toggled(bool)
{
    updateMode();
}

void ConditionDialog::on_lineSquare_editingFinished()
{
    int v = ui->lineSquare->text().toInt();
    int area = (v+1) * (v+1);
    for (int i = 0; i < 9; i++)
    {
        if (tempsboxes[i])
            tempsboxes[i]->setMaximum(area);
    }
}

void ConditionDialog::on_buttonCancel_clicked()
{
    close();
}

void ConditionDialog::on_buttonOk_clicked()
{
    Condition c = cond;
    c.type = ui->comboBoxType->currentData().toInt();
    c.relative = ui->comboBoxRelative->currentData().toInt();
    c.count = ui->spinBox->value();

    if (ui->radioSquare->isChecked())
    {
        int d = ui->lineSquare->text().toInt();
        c.x1 = (-d) >> 1;
        c.z1 = (-d) >> 1;
        c.x2 = (d) >> 1;
        c.z2 = (d) >> 1;
    }
    else
    {
        c.x1 = ui->lineEditX1->text().toInt();
        c.z1 = ui->lineEditZ1->text().toInt();
        c.x2 = ui->lineEditX2->text().toInt();
        c.z2 = ui->lineEditZ2->text().toInt();
    }

    if (ui->checkRadius->isChecked())
        c.rmax = ui->lineRadius->text().toInt() + 1;
    else
        c.rmax = 0;

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
        c.bfilter = setupBiomeFilter(bin, in, bex, ex);
        c.count = in + ex;
    }
    if (ui->tabTemps->isEnabled())
    {
        c.count = 0;
        for (int i = 0; i < 9; i++)
        {
            if (!tempsboxes[i])
                continue;
            int cnt = tempsboxes[i]->value();
            c.temps[i] = cnt;
            if (cnt > 0)
                c.count += cnt;
        }
    }

    c.y = ui->comboY->currentText().section(' ', 0, 0).toInt();

    c.flags = 0;
    if (ui->checkApprox->isChecked())
        c.flags |= CFB_APPROX;
    if (ui->checkMatchAny->isChecked())
        c.flags |= CFB_MATCH_ANY;

    c.variants = 0;
    c.variants |= ui->checkStartPiece->isChecked() * Condition::START_PIECE_MASK;
    c.variants |= ui->checkAbandoned->isChecked() * Condition::ABANDONED_MASK;
    for (VariantCheckBox *cb : variantboxes)
        if (cb->isChecked())
            c.variants |= cb->getMask();

    getClimateLimits(c.limok, c.limex);

    if (warnIfBad(c) != QMessageBox::Ok)
        return;
    cond = c;
    emit setCond(item, cond, 1);
    item = 0;
    close();
}

void ConditionDialog::on_ConditionDialog_finished(int result)
{
    if (item)
        emit setCond(item, cond, result);
    item = 0;
}

void ConditionDialog::on_comboBoxCat_currentIndexChanged(int idx)
{
    ui->comboBoxType->setEnabled(idx != CAT_NONE);
    ui->comboBoxType->clear();

    int slot = 0;
    ui->comboBoxType->insertItem(slot, tr("Select type"), QVariant::fromValue((int)F_SELECT));

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
            ui->comboBoxType->insertItem(slot, QApplication::translate("Filter", ft->name), vidx);

        if (mc < ft->mcmin || mc > ft->mcmax)
            ui->comboBoxType->setItemData(slot, false, Qt::UserRole-1); // deactivate
        if (ft == g_filterinfo.list + F_FORTRESS)
            ui->comboBoxType->insertSeparator(slot++);
        if (ft == g_filterinfo.list + F_ENDCITY)
            ui->comboBoxType->insertSeparator(slot++);
    }

    updateMode();
}

void ConditionDialog::on_checkStartPiece_stateChanged(int state)
{
    ui->scrollVariants->setEnabled(state);
}

void ConditionDialog::getClimateLimits(int limok[6][2], int limex[6][2])
{
    getClimateLimits(climaterange[0], limok);
    getClimateLimits(climaterange[1], limex);

    // the required climates can be complete or partial
    // for the partial (default) requirement, we flip the bounds
    for (int i = 0; i < 6; i++)
    {
        if (!climatecomplete[i])
            continue;
        if (climatecomplete[i]->isChecked())
        {
            int tmp = limok[i][0];
            limok[i][0] = limok[i][1];
            limok[i][1] = tmp;
        }
        if (climaterange[0][i])
        {
            QColor col = QColor(Qt::darkCyan);
            if (climatecomplete[i]->isChecked())
                col = QColor(Qt::darkGreen);
            climaterange[0][i]->setHighlight(col, QColor(0,0,0,0));
        }
    }
}

void ConditionDialog::getClimateLimits(LabeledRange *ranges[6], int limits[6][2])
{
    for (int i = 0; i < 6; i++)
    {
        int lmin = INT_MIN, lmax = INT_MAX;
        if (ranges[i])
        {
            lmin = ranges[i]->slider->pos0;
            if (lmin == ranges[i]->slider->vmin)
                lmin = INT_MIN;
            lmax = ranges[i]->slider->pos1;
            if (lmax == ranges[i]->slider->vmax)
                lmax = INT_MAX;
        }
        limits[i][0] = lmin;
        limits[i][1] = lmax;
    }
}

void ConditionDialog::setClimateLimits(LabeledRange *ranges[6], int limits[6][2], bool complete)
{
    for (int i = 0; i < 6; i++)
    {
        if (!ranges[i])
            continue;
        int lmin = limits[i][0], lmax = limits[i][1];
        if (complete && climatecomplete[i])
        {
            climatecomplete[i]->setChecked(lmin > lmax);
        }
        if (lmin > lmax)
        {
            int tmp = lmin;
            lmin = lmax;
            lmax = tmp;
        }
        if (lmin == INT_MIN)
            lmin = ranges[i]->slider->vmin;
        if (lmax == INT_MAX)
            lmax = ranges[i]->slider->vmax;
        ranges[i]->setValues(lmin, lmax);
    }
}

void ConditionDialog::onClimateLimitChanged()
{
    int limok[6][2], limex[6][2];
    char ok[256], ex[256];

    getClimateLimits(limok, limex);

    getPossibleBiomesForLimits(ok, mc, limok);
    getPossibleBiomesForLimits(ex, mc, limex);

    for (auto& it : noisebiomes)
    {
        int id = it.first;
        NoiseBiomeIndicator *cb = it.second;

        Qt::CheckState state = Qt::Unchecked;
        if (ok[id]) state = Qt::PartiallyChecked;
        if (!ex[id]) state = Qt::Checked;
        cb->setCheckState(state);
    }
}


