#include "filterdialog.h"
#include "ui_filterdialog.h"

#include "cutil.h"

#include <QCheckBox>
#include <QIntValidator>
#include <QColor>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>


#define SETUP_BIOME_CHECKBOX(B) do {\
        biomecboxes[B] = new QCheckBox(#B);\
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
            tip += QString("\n") + biome2str(j);
    for (int j = 0; j < 64; j++)
        if (mM & (1ULL << j))
            tip += QString("\n") + biome2str(128+j);
    return tip;
}


FilterDialog::FilterDialog(MainWindow *parent, QListWidgetItem *item, Condition *initcond) :
    QDialog(parent, Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
    ui(new Ui::FilterDialog),
    item(item)
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

    QIntValidator *intval = new QIntValidator(this);
    ui->lineRadius->setValidator(intval);
    ui->lineEditX1->setValidator(intval);
    ui->lineEditZ1->setValidator(intval);
    ui->lineEditX2->setValidator(intval);
    ui->lineEditZ2->setValidator(intval);

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

    memset(tempsboxes, 0, sizeof(tempsboxes));

    SETUP_TEMPCAT_SPINBOX(Oceanic);
    SETUP_TEMPCAT_SPINBOX(Warm);
    SETUP_TEMPCAT_SPINBOX(Lush);
    SETUP_TEMPCAT_SPINBOX(Cold);
    SETUP_TEMPCAT_SPINBOX(Freezing);
    SETUP_TEMPCAT_SPINBOX(Special+Warm);
    SETUP_TEMPCAT_SPINBOX(Special+Lush);
    SETUP_TEMPCAT_SPINBOX(Special+Cold);


    ui->tabWidget->setStyleSheet(
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

        ui->comboBoxType->setCurrentIndex(cond.type);
        ui->comboBoxRelative->setCurrentIndex(initindex);

        updateMode();

        if (!ui->tabWidget->isEnabled())
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
                bool c2 = cond.exclb & (1ULL << i);
                biomecboxes[i]->setCheckState(c2 ? Qt::Checked : c1 ? Qt::PartiallyChecked : Qt::Unchecked);
            }

            if (biomecboxes[i+128])
            {
                bool c1 = (cond.bfilter.riverToFindM) & (1ULL << i);
                bool c2 = cond.exclm & (1ULL << i);
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
    ui->textDescription->setText(ft.desription);
    ui->buttonOk->setEnabled(filterindex != F_SELECT);
}

void FilterDialog::updateBiomeSelection()
{
    int filterindex = ui->comboBoxType->currentData().toInt();
    const FilterInfo &ft = g_filterinfo.list[filterindex];
;
    if (filterindex == F_TEMPS)
    {
        ui->tabWidget->setEnabled(true);
        ui->tabWidget->setCurrentWidget(ui->tabTemps);
        ui->tabTemps->setEnabled(true);
        ui->tabBiomes->setEnabled(false);
    }
    else if (filterindex >= F_BIOME && filterindex <= F_BIOME_256_OTEMP)
    {
        ui->tabWidget->setEnabled(true);
        ui->tabWidget->setCurrentWidget(ui->tabBiomes);
        ui->tabTemps->setEnabled(false);
        ui->tabBiomes->setEnabled(true);
    }
    else
    {
        ui->tabWidget->setEnabled(false);
        ui->tabTemps->setEnabled(false);
        ui->tabBiomes->setEnabled(false);
    }

    if (ft.layer == L13_OCEAN_TEMP_256)
    {
        for (int i = 0; i < 256; i++)
        {
            QCheckBox *cb = biomecboxes[i];
            if (cb)
            {
                cb->setEnabled(false);
                cb->setToolTip("");
            }
        }
        biomecboxes[warm_ocean]->setEnabled(true);
        biomecboxes[lukewarm_ocean]->setEnabled(true);
        biomecboxes[ocean]->setEnabled(true);
        biomecboxes[cold_ocean]->setEnabled(true);
        biomecboxes[frozen_ocean]->setEnabled(true);
    }
    else if (ft.layer)
    {
        for (int i = 0; i < 256; i++)
        {
            QCheckBox *cb = biomecboxes[i];
            if (!cb)
                continue;

            uint64_t mL = 0, mM = 0;
            genPotential(&mL, &mM, ft.layer, MC_1_16, i);
            if (mL || mM)
            {
                cb->setEnabled(true);
                if (ft.layer != L_VORONOI_ZOOM_1)
                {
                    QString tip = "Generates any of:";
                    for (int j = 0; j < 64; j++)
                    {
                        if (mL & (1ULL << j))
                            tip += QString("\n") + biome2str(j);
                    }
                    for (int j = 0; j < 64; j++)
                    {
                        if (mM & (1ULL << j))
                            tip += QString("\n") + biome2str(j+128);
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
}


void FilterDialog::on_comboBoxType_activated(int)
{
    updateMode();
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

    if (ui->tabBiomes->isEnabled())
    {
        int b[256], in = 0, ex = 0;
        cond.exclb = 0;
        cond.exclm = 0;
        for (int i = 0; i < 256; i++)
        {
            QCheckBox *cb = biomecboxes[i];
            if (cb && cb->isEnabled())
            {
                if (cb->checkState() == Qt::PartiallyChecked)
                    b[in++] = i;
                else if (cb->checkState() == Qt::Checked)
                {
                    if (i < 128)
                        cond.exclb |= 1ULL << i;
                    else
                        cond.exclm |= 1ULL << (i-128);
                    ex++;
                }
            }
        }

        cond.bfilter = setupBiomeFilter(b, in);
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
