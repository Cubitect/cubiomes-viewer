#include "layerdialog.h"
#include "ui_layerdialog.h"

#include "config.h"

#include <QApplication>

bool getLayerOptionInfo(LayerOptInfo *info, int mode, int disp, WorldInfo wi)
{
    QString txt;
    QString tip;
    int nptype = -1;

    switch (mode)
    {
    case LOPT_BIOMES:
        if (disp == 0) txt = "1:1";
        if (disp == 1) txt = "1:4";
        if (disp == 2) txt = "1:16";
        if (disp == 3) txt = "1:64";
        if (disp == 4) txt = "1:256";
        break;
    case LOPT_HEIGHT_4:
        if (disp == 0) txt = QApplication::translate("LayerDialog", "Grayscale");
        if (disp == 1) txt = QApplication::translate("LayerDialog", "Shaded biome map");
        if (disp == 2) txt = QApplication::translate("LayerDialog", "Contours on biomes");
        if (disp == 3) txt = QApplication::translate("LayerDialog", "Shaded with contours");
        break;
    case LOPT_NOISE_T_4: nptype = NP_TEMPERATURE; break;
    case LOPT_NOISE_H_4: nptype = NP_HUMIDITY; break;
    case LOPT_NOISE_C_4: nptype = NP_CONTINENTALNESS; break;
    case LOPT_NOISE_E_4: nptype = NP_EROSION; break;
    case LOPT_NOISE_W_4: nptype = NP_WEIRDNESS; break;
    }

    if (nptype != -1 && disp >= 0)
    {
        BiomeNoise bn;
        initBiomeNoise(&bn, wi.mc);
        setBiomeSeed(&bn, wi.seed, wi.large);

        DoublePerlinNoise *dpn = bn.climate + nptype;
        PerlinNoise *oct[2] = { dpn->octA.octaves, dpn->octB.octaves };
        int noct = dpn->octA.octcnt;
        int idx = noct-1;
        int ab = 1;
        if (disp > 0)
        {
            idx = (disp - 1) / 2;
            ab  = (disp - 1) % 2;
            if (idx >= noct)
                return false;
        }

        double ampsum = 0, amptot = 0;
        for (int i = 0; i < noct; i++)
        {
            for (int j = 0; j <= 1; j++)
            {
                double a = oct[j][i].amplitude;
                amptot += a;
                if (i < idx || (idx == i && j <= ab))
                    ampsum += a;
            }
        }
        amptot *= dpn->amplitude;
        ampsum *= dpn->amplitude;

        double f = 337.0 / 331.0;
        PerlinNoise *pn = &oct[ab][idx];
        double a = pn->amplitude * dpn->amplitude;
        double l = pn->lacunarity * (ab == 0 ? 1.0 : f);

        if (disp == 0)
        {
            txt += QString("%1..    (all)   x%2").arg(QChar(0x03A3)).arg(amptot, 0, 'f', 6);
            tip += QApplication::translate("LayerDialog", "All octaves");
        }
        else
        {
            txt += QString("%1..").arg(QChar(0x03A3));
            txt += QString::asprintf("%d%c  1:%-5.0f x%.6f", idx, ab?'B':'A', 4/l, a);
            tip += QApplication::translate("LayerDialog", "Contribution of the %n most significant octaves out of %1 total.", "", disp).arg(2*noct);
        }
        tip += "\n" + QApplication::translate("LayerDialog", "Total contribution: %1 = %2%").arg(ampsum).arg(100 * ampsum / amptot, 0, 'f', 1);
        tip += "\n" + QApplication::translate("LayerDialog", "Octave amplitude: %1").arg(a);
        tip += "\n" + QApplication::translate("LayerDialog", "Octave lacunarity: %1 = 1/%2").arg(l).arg(4/l);
    }

    if (info) {
        info->summary = txt;
        info->tooltip = tip;
    }
    return !txt.isEmpty();
}

LayerDialog::LayerDialog(QWidget *parent, WorldInfo wi)
  : QDialog(parent)
  , ui(new Ui::LayerDialog)
  , radio{}
  , combo{}
{
    ui->setupUi(this);
    radio[LOPT_BIOMES] = ui->radioBiomes;
    radio[LOPT_NOISE_T_4] = ui->radioNoiseT;
    radio[LOPT_NOISE_H_4] = ui->radioNoiseH;
    radio[LOPT_NOISE_C_4] = ui->radioNoiseC;
    radio[LOPT_NOISE_E_4] = ui->radioNoiseE;
    radio[LOPT_NOISE_D_4] = ui->radioNoiseD;
    radio[LOPT_NOISE_W_4] = ui->radioNoiseW;
    radio[LOPT_RIVER_4] = ui->radioRiver;
    radio[LOPT_OCEAN_256] = ui->radioOcean;
    radio[LOPT_NOOCEAN_1] = ui->radioNoOcean;
    radio[LOPT_BETA_T_1] = ui->radioBetaT;
    radio[LOPT_BETA_H_1] = ui->radioBetaH;
    radio[LOPT_HEIGHT_4] = ui->radioHeight;
    radio[LOPT_STRUCTS] = ui->radioStruct;

    combo[LOPT_BIOMES] = ui->comboBiomes;
    combo[LOPT_NOISE_T_4] = ui->comboNoiseT;
    combo[LOPT_NOISE_H_4] = ui->comboNoiseH;
    combo[LOPT_NOISE_C_4] = ui->comboNoiseC;
    combo[LOPT_NOISE_E_4] = ui->comboNoiseE;
    combo[LOPT_NOISE_W_4] = ui->comboNoiseW;
    combo[LOPT_HEIGHT_4] = ui->comboHeight;

    for (int i = 0; i < LOPT_MAX; i++)
    {
        if (!combo[i])
            continue;
        for (int j = 0; ; j++)
        {
            LayerOptInfo info;
            if (!getLayerOptionInfo(&info, i, j, wi))
                break;
            QString s = info.summary.leftJustified(24);
            if (j <= 9)
                s += "\tALT+"+QString::number(j);
            combo[i]->addItem(s);
            combo[i]->setItemData(combo[i]->count()-1, info.tooltip, Qt::ToolTipRole);
        }
        connect(combo[i], QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int){
            this->onComboChange(combo[i]);
        });
        onComboChange(combo[i]);
    }

    QFont fmono;
    fmono.setFamily(QString::fromUtf8("Monospace"));
    for (int i = 0; i < LOPT_MAX; i++)
    {
        if (!radio[i])
            continue;
        connect(radio[i], &QRadioButton::toggled, this, &LayerDialog::onRadioChange);
        if (combo[i])
            combo[i]->setFont(fmono);
        if (i >= LOPT_NOISE_T_4 && i <= LOPT_NOISE_W_4)
        {
            radio[i]->setEnabled(wi.mc > MC_1_17);
            if (combo[i])
                combo[i]->setEnabled(wi.mc > MC_1_17);
        }
        if (i == LOPT_RIVER_4 || i == LOPT_OCEAN_256)
        {
            radio[i]->setEnabled(wi.mc > MC_1_12 && wi.mc <= MC_1_17);
        }
        if (i == LOPT_NOOCEAN_1 || i == LOPT_BETA_T_1 || i == LOPT_BETA_H_1)
        {
            radio[i]->setEnabled(wi.mc <= MC_B1_7);
        }
    }
}

LayerDialog::~LayerDialog()
{
    delete ui;
}

void LayerDialog::setLayerOptions(LayerOpt opt)
{
    for (int i = 0; i < LOPT_MAX; i++)
    {
        if (radio[i])
            radio[i]->setChecked(opt.mode == i);
        if (combo[i])
            combo[i]->setCurrentIndex(opt.disp[i]);
    }
}

LayerOpt LayerDialog::getLayerOptions()
{
    LayerOpt opt;
    for (int i = 0; i < LOPT_MAX; i++)
    {
        if (radio[i] && radio[i]->isChecked())
            opt.mode = i;
        if (combo[i])
            opt.disp[i] = combo[i]->currentIndex();
    }
    return opt;
}

void LayerDialog::onRadioChange()
{
    for (int i = 0; i < LOPT_MAX; i++)
    {
        if (radio[i] && combo[i])
            combo[i]->setEnabled(radio[i]->isChecked());
    }
}

void LayerDialog::onComboChange(QComboBox *combo)
{
    combo->setToolTip(combo->currentData(Qt::ToolTipRole).toString());
}

void LayerDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::StandardButton b = ui->buttonBox->standardButton(button);
    if (b == QDialogButtonBox::Ok || b == QDialogButtonBox::Apply)
        emit apply();
}

