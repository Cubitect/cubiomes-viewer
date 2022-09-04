#include "conditiondialog.h"
#include "ui_conditiondialog.h"

#include "mainwindow.h"

#include <QCheckBox>
#include <QIntValidator>
#include <QColor>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QMessageBox>
#include <QScrollBar>
#include <QSpacerItem>

#define SETUP_TEMPCAT_SPINBOX(B) do {\
        tempsboxes[B] = new SpinExclude();\
        QLabel *l = new QLabel(#B);\
        ui->gridLayoutTemps->addWidget(tempsboxes[B], (B) % Special, (B) / Special * 2 + 0);\
        ui->gridLayoutTemps->addWidget(l, (B) % Special, (B) / Special * 2 + 1);\
        l->setToolTip(getTip( MC_1_16, L_SPECIAL_1024, 0, (B) % Special + ((B)>=Special?256:0) ));\
    } while (0)

static QString getTip(int mc, int layer, uint32_t flags, int id)
{
    uint64_t mL = 0, mM = 0;
    genPotential(&mL, &mM, layer, mc, flags, id);
    QString tip = ConditionDialog::tr("Generates any of:");
    for (int j = 0; j < 64; j++)
        if (mL & (1ULL << j))
            tip += QString("\n") + biome2str(mc, j);
    for (int j = 0; j < 64; j++)
        if (mM & (1ULL << j))
            tip += QString("\n") + biome2str(mc, 128+j);
    return tip;
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

    QFont mono = QFont("Monospace", 10);
    mono.setStyleHint(QFont::TypeWriter);
    ui->lineSummary->setFont(mono);

    // prevent bold font of group box title getting inherited
    QFont dfont = font();
    dfont.setBold(false);
    const QList<QWidget*> children = ui->groupBoxPosition->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *w : children)
        w->setFont(dfont);

    int initindex = -1;
    const QVector<Condition> existing = parent->getConditions();
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
                .arg(initcond->relative, 2, 10, QChar('0'))
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

    QIntValidator *uintval = new QIntValidator(0, 30e6, this);
    ui->lineSquare->setValidator(uintval);
    ui->lineRadius->setValidator(uintval);

    ui->comboY->lineEdit()->setValidator(new QIntValidator(-64, 320, this));

    for (int i = 0; i < 256; i++)
    {
        const char *str = biome2str(mc, i);
        if (!str)
            continue;
        QCheckBox *cb = new QCheckBox(str);
        ui->gridLayoutBiomes->addWidget(cb, i % 128, i / 128);
        cb->setTristate(true);
        biomecboxes[i] = cb;
    }
    separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    ui->gridLayoutBiomes->addWidget(separator, 128, 0, 1, 2);

    memset(tempsboxes, 0, sizeof(tempsboxes));

    SETUP_TEMPCAT_SPINBOX(Oceanic);
    SETUP_TEMPCAT_SPINBOX(Warm);
    SETUP_TEMPCAT_SPINBOX(Lush);
    SETUP_TEMPCAT_SPINBOX(Cold);
    SETUP_TEMPCAT_SPINBOX(Freezing);
    SETUP_TEMPCAT_SPINBOX(Special+Warm);
    SETUP_TEMPCAT_SPINBOX(Special+Lush);
    SETUP_TEMPCAT_SPINBOX(Special+Cold);

    for (const StartPiece *sp = g_start_pieces; sp->stype >= 0; sp++)
    {
        VariantCheckBox *cb = new VariantCheckBox(sp);
        variantboxes.push_back(cb);
        if (sp->stype == Village)
            ui->gridVillagePieces->addWidget(cb, sp->row, sp->col);
        else if (sp->stype == Bastion)
            ui->gridBastionPieces->addWidget(cb, sp->row, sp->col);
        else if (sp->stype == Ruined_Portal)
            ui->gridPortalPieces->addWidget(cb, sp->row, sp->col);
    }
    connect(ui->checkStartPieces, SIGNAL(stateChanged(int)), this, SLOT(onCheckStartChanged(int)));
    connect(ui->checkStartBastion, SIGNAL(stateChanged(int)), this, SLOT(onCheckStartChanged(int)));
    connect(ui->checkStartPortal, SIGNAL(stateChanged(int)), this, SLOT(onCheckStartChanged(int)));

    QGridLayout *grids[] = { ui->gridVillagePieces, ui->gridBastionPieces, ui->gridPortalPieces };
    for (int i = 0; i < 3; i++)
    {
        QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Maximum, QSizePolicy::Expanding);
        grids[i]->addItem(spacer, grids[i]->rowCount(), 0);
    }

    QString tristyle =
        "QCheckBox::indicator:unchecked     { image: url(:/icons/check0.png); }\n"
        "QCheckBox::indicator:indeterminate { image: url(:/icons/check1.png); }\n"
        "QCheckBox::indicator:checked       { image: url(:/icons/check2.png); }\n";
    ui->scrollBiomes->setStyleSheet(tristyle);
    ui->scrollNoise->setStyleSheet(tristyle);

    memset(climaterange, 0, sizeof(climaterange));
    memset(climatecomplete, 0, sizeof(climatecomplete));
    const int *extremes = getBiomeParaExtremes(MC_NEWEST);
    struct { QString name; int idx; } climates[] =
    {
        {tr("Temperature:"),        NP_TEMPERATURE      },
        {tr("Humidity:"),           NP_HUMIDITY         },
        {tr("Continentalness:"),    NP_CONTINENTALNESS  },
        {tr("Erosion:"),            NP_EROSION          },
        // depth has more dependencies and is not supported
        {tr("Weirdness:"),          NP_WEIRDNESS        },
    };
    for (int i = 0; i < 5; i++)
    {
        QLabel *label = new QLabel(climates[i].name, this);
        int cmin = extremes[2*climates[i].idx + 0];
        int cmax = extremes[2*climates[i].idx + 1];
        LabeledRange *ok = new LabeledRange(this, cmin-1, cmax+1);
        LabeledRange *ex = new LabeledRange(this, cmin-1, cmax+1);
        ok->setLimitText(tr("-Inf"), tr("+Inf"));
        ex->setLimitText(tr("-Inf"), tr("+Inf"));
        ex->setHighlight(QColor(0,0,0,0), QColor(Qt::red));
        connect(ok, SIGNAL(onRangeChange()), this, SLOT(onClimateLimitChanged()));
        connect(ex, SIGNAL(onRangeChange()), this, SLOT(onClimateLimitChanged()));
        climaterange[0][climates[i].idx] = ok;
        climaterange[1][climates[i].idx] = ex;

        QCheckBox *all = new QCheckBox(this);
        all->setFixedWidth(20);
        all->setToolTip(tr("Require full range instead of intersection"));
        connect(all, SIGNAL(stateChanged(int)), this, SLOT(onClimateLimitChanged()));
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
    for (const auto& it : biomecboxes)
    {
        int id = it.first;
        QCheckBox *cb = it.second;
        QColor col(biomeColors[id][0], biomeColors[id][1], biomeColors[id][2]);
        p.fillPath(path, col);
        p.drawPath(path);
        cb->setIcon(QIcon(pixmap));
        if (noisebiomes.count(id))
            noisebiomes[id]->setIcon(QIcon(pixmap));
    }

    // defaults
    ui->checkEnabled->setChecked(true);
    ui->spinBox->setValue(1);
    ui->checkSkipRef->setChecked(false);
    ui->radioSquare->setChecked(true);
    ui->checkRadius->setChecked(false);
    onCheckStartChanged(false);

    if (initcond)
    {
        cond = *initcond;
        const FilterInfo &ft = g_filterinfo.list[cond.type];

        ui->checkEnabled->setChecked(!(cond.meta & Condition::DISABLED));
        ui->lineSummary->setPlaceholderText(QApplication::translate("Filter", ft.name));
        QByteArray txta = QByteArray(cond.text, sizeof(cond.text));
        ui->lineSummary->setText(QString::fromLocal8Bit(txta));

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
        ui->checkSkipRef->setChecked(cond.skipref);
        ui->lineEditX1->setText(QString::number(cond.x1));
        ui->lineEditZ1->setText(QString::number(cond.z1));
        ui->lineEditX2->setText(QString::number(cond.x2));
        ui->lineEditZ2->setText(QString::number(cond.z2));

        ui->checkApprox->setChecked(cond.flags & APPROX);
        ui->checkMatchAny->setChecked(cond.flags & MATCH_ANY);
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

        for (const auto& it : biomecboxes)
        {
            int id = it.first;
            if (id < 128)
            {
                bool c1 = cond.biomeToFind & (1ULL << id);
                bool c2 = cond.biomeToExcl & (1ULL << id);
                it.second->setCheckState(c2 ? Qt::Checked : c1 ? Qt::PartiallyChecked : Qt::Unchecked);
            }
            else
            {
                bool c1 = cond.biomeToFindM & (1ULL << (id-128));
                bool c2 = cond.biomeToExclM & (1ULL << (id-128));
                it.second->setCheckState(c2 ? Qt::Checked : c1 ? Qt::PartiallyChecked : Qt::Unchecked);
            }
        }
        for (int i = 0; i < 9; i++)
        {
            if (tempsboxes[i])
            {
                tempsboxes[i]->setValue(cond.temps[i]);
            }
        }

        ui->checkStartPieces->setChecked(cond.varflags & Condition::VAR_WITH_START);
        ui->checkAbandoned->setChecked(cond.varflags & Condition::VAR_ABANODONED);
        ui->checkEndShip->setChecked(cond.varflags & Condition::VAR_ENDSHIP);
        for (VariantCheckBox *cb : qAsConst(variantboxes))
        {
            int idx = cb->sp - g_start_pieces;
            cb->setChecked(cond.varstart & (1ULL << idx));
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

void ConditionDialog::updateMode()
{
    int filterindex = ui->comboBoxType->currentData().toInt();
    const FilterInfo &ft = g_filterinfo.list[filterindex];

    ui->lineSummary->setPlaceholderText(QApplication::translate("Filter", ft.name));

    QPalette pal;
    if (mc < ft.mcmin || mc > ft.mcmax)
        pal.setColor(QPalette::Normal, QPalette::Button, QColor(255,0,0,127));
    ui->comboBoxType->setPalette(pal);

    ui->groupBoxGeneral->setEnabled(filterindex != F_SELECT);
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
    ui->checkSkipRef->setEnabled(ft.count);

    ui->labelY->setEnabled(ft.hasy);
    ui->comboY->setEnabled(ft.hasy);

    if (filterindex == F_TEMPS)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageTemps);
    }
    else if (filterindex == F_CLIMATE_NOISE)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageClimates);
    }
    else if (ft.cat == CAT_BIOMES || ft.cat == CAT_NETHER || ft.cat == CAT_END)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageBiomes);
        ui->checkApprox->setEnabled(mc <= MC_1_17 || ft.step == 4);
        ui->checkMatchAny->setEnabled(true);
    }
    else if (filterindex == F_VILLAGE)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageVillage);
        ui->checkStartPieces->setEnabled(mc >= MC_1_14);
        ui->checkAbandoned->setEnabled(filterindex == F_VILLAGE && mc >= MC_1_10);
    }
    else if (filterindex == F_BASTION)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageBastion);
        ui->checkStartBastion->setEnabled(mc >= MC_1_16);
    }
    else if (filterindex == F_PORTAL || filterindex == F_PORTALN)
    {
        ui->stackedWidget->setCurrentWidget(ui->pagePortal);
        ui->checkStartPortal->setEnabled(mc >= MC_1_16);
    }
    else if (filterindex == F_ENDCITY)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageEndCity);
        ui->checkEndShip->setEnabled(mc >= MC_1_9);
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(ui->pageNone);
    }

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

void ConditionDialog::updateBiomeSelection()
{
    int filterindex = ui->comboBoxType->currentData().toInt();
    const FilterInfo &ft = g_filterinfo.list[filterindex];

    // clear tool tips
    for (const auto& it : biomecboxes)
        it.second->setToolTip("");

    std::vector<int> available;

    if (ft.cat == CAT_NETHER || ft.cat == CAT_END)
    {
        for (int i = 0; i < 256; i++)
            if (getDimension(i) == ft.dim)
                available.push_back(i);
    }
    if (filterindex == F_BIOME_256_OTEMP)
    {
        available.push_back(warm_ocean);
        available.push_back(lukewarm_ocean);
        available.push_back(ocean);
        available.push_back(cold_ocean);
        available.push_back(frozen_ocean);
    }
    else if (ft.cat == CAT_BIOMES && mc <= MC_1_17)
    {
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

        for (const auto& it : biomecboxes)
        {
            QCheckBox *cb = it.second;
            uint64_t mL = 0, mM = 0;
            uint32_t flags = 0;
            genPotential(&mL, &mM, layerId, mc, flags, it.first);

            if (mL || mM)
            {
                available.push_back(it.first);
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
        }
    }
    else if (ft.cat == CAT_BIOMES && ft.dim == DIM_OVERWORLD)
    {
        for (const auto& it : biomecboxes)
        {
            if (isOverworld(mc, it.first))
                available.push_back(it.first);
        }
    }

    // separate available biomes
    QLayoutItem *sep = ui->gridLayoutBiomes->takeAt(ui->gridLayoutBiomes->indexOf(separator));
    std::vector<int> unavailable;
    std::map<int, QLayoutItem*> items;
    for (const auto& it : biomecboxes)
    {
        int id = it.first;
        QCheckBox *cb = it.second;
        int idx = ui->gridLayoutBiomes->indexOf(cb);
        items[id] = ui->gridLayoutBiomes->takeAt(idx);
        if (std::find(available.begin(), available.end(), id) == available.end())
            unavailable.push_back(id);
    }

    IdCmp cmp = {IdCmp::SORT_LEX, mc, DIM_UNDEF};
    std::sort(available.begin(), available.end(), cmp);
    std::sort(unavailable.begin(), unavailable.end(), cmp);

    int row = 0;
    for (int i = 0, len = available.size(), mod = (len+1)/2; i < len; i++)
    {
        int id = available[i];
        biomecboxes[id]->setEnabled(true);
        QLayoutItem *item = items[id];
        ui->gridLayoutBiomes->addItem(item, row+i%mod, i/mod);
    }
    row = (available.size() + 1) / 2;
    ui->gridLayoutBiomes->addItem(sep, row, 0, 1, 2);
    row++;
    for (int i = 0, len = unavailable.size(), mod = (len+1)/2; i < len; i++)
    {
        int id = unavailable[i];
        biomecboxes[id]->setEnabled(false);
        QLayoutItem *item = items[id];
        ui->gridLayoutBiomes->addItem(item, row+i%mod, i/mod);
    }
}

int ConditionDialog::warnIfBad(Condition cond)
{
    const FilterInfo &ft = g_filterinfo.list[cond.type];
    if ((cond.varflags & Condition::VAR_WITH_START) && (cond.varstart == 0))
    {
        if (ui->checkStartPieces->isEnabled())
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
            uint64_t m = cond.biomeToFindM;
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
    for (const auto& it : biomecboxes)
        it.second->setCheckState(Qt::Unchecked);
}

void ConditionDialog::on_buttonInclude_clicked()
{
    for (const auto& it : biomecboxes)
        it.second->setCheckState(it.second->isEnabled() ? Qt::PartiallyChecked : Qt::Unchecked);
}

void ConditionDialog::on_buttonExclude_clicked()
{
    for (const auto& it : biomecboxes)
        it.second->setCheckState(it.second->isEnabled() ? Qt::Checked : Qt::Unchecked);
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
    c.version = Condition::VER_CURRENT;
    c.type = ui->comboBoxType->currentData().toInt();
    c.relative = ui->comboBoxRelative->currentData().toInt();
    c.count = ui->spinBox->value();
    c.skipref = ui->checkSkipRef->isChecked();

    if (ui->checkEnabled->isChecked())
        c.meta &= ~Condition::DISABLED;
    else
        c.meta |= Condition::DISABLED;

    QByteArray text = ui->lineSummary->text().toLocal8Bit().leftJustified(sizeof(c.text), '\0');
    memcpy(c.text, text.data(), sizeof(c.text));

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
    if (c.x1 > c.x2) std::swap(c.x1, c.x2);
    if (c.z1 > c.z2) std::swap(c.z1, c.z2);

    if (ui->checkRadius->isChecked())
        c.rmax = ui->lineRadius->text().toInt() + 1;
    else
        c.rmax = 0;

    if (ui->stackedWidget->currentWidget() == ui->pageBiomes)
    {
        c.biomeToFind = c.biomeToFindM = 0;
        c.biomeToExcl = c.biomeToExclM = 0;

        for (const auto& it : biomecboxes)
        {
            int id = it.first;
            QCheckBox *cb = it.second;
            if (cb && cb->isEnabled())
            {
                if (cb->checkState() == Qt::PartiallyChecked)
                {
                    if (id < 128) c.biomeToFind |= (1ULL << id);
                    else c.biomeToFindM |= (1ULL << (id-128));
                }
                if (cb->checkState() == Qt::Checked)
                {
                    if (id < 128) c.biomeToExcl |= (1ULL << id);
                    else c.biomeToExclM |= (1ULL << (id-128));
                }
            }
        }
        c.count = 0;
    }
    if (ui->stackedWidget->currentWidget() == ui->pageTemps)
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
        c.flags |= APPROX;
    if (ui->checkMatchAny->isChecked())
        c.flags |= MATCH_ANY;

    c.varflags = c.varstart = 0;
    c.varflags |= ui->checkStartPieces->isChecked() * Condition::VAR_WITH_START;
    c.varflags |= ui->checkAbandoned->isChecked() * Condition::VAR_ABANODONED;
    c.varflags |= ui->checkEndShip->isChecked() * Condition::VAR_ENDSHIP;
    for (VariantCheckBox *cb : qAsConst(variantboxes))
    {
        if (!cb->isChecked())
            continue;
        c.varstart |= 1ULL << (cb->sp - g_start_pieces);
    }

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

void ConditionDialog::onCheckStartChanged(int checked)
{   // synchronize stat piece checkboxes
    ui->checkStartPieces->setChecked(checked);
    ui->checkStartBastion->setChecked(checked);
    ui->checkStartPortal->setChecked(checked);
    ui->scrollVillagePieces->setEnabled(checked);
    ui->scrollBastionPieces->setEnabled(checked);
    ui->scrollPortalPieces->setEnabled(checked);
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

