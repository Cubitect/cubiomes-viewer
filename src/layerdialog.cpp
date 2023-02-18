#include "layerdialog.h"
#include "ui_layerdialog.h"

#include "settings.h"


const char *getLayerOptionText(int mode, int disp)
{
    /*
    Para 0:
      4096   0.952381
      1024   0.158730
    Para 1:
      1024   0.564374
       512   0.282187
    Para 2:
      2048   0.751468
      1024   0.375734
       512   0.375734
       256   0.187867
       128   0.093933
        64   0.023483
        32   0.011742
        16   0.005871
         8   0.002935
    Para 3:
      2048   0.716846
      1024   0.358423
       256   0.089606
       128   0.044803
    Para 4:
        32   0.666667
        16   0.333333
         8   0.166667
    Para 5:
       512   0.634921
       256   0.634921
       128   0.158730
    */
    switch (mode)
    {
    case LOPT_BIOMES:
        switch (disp) {
        case 0: return "1:1";
        case 1: return "1:4";
        case 2: return "1:16";
        case 3: return "1:64";
        case 4: return "1:256";
        default: return nullptr;
        }
    case LOPT_NOISE_T_4:
        switch (disp) {
        case 0: return "All";
        case 1: return "+A[0] 1:4096 x0.952381";
        case 2: return "+B[0] 1:4023 x0.952381";
        case 3: return "+A[1] 1:1024 x0.158730";
        case 4: return "+B[1] 1:1005 x0.158730";
        default: return nullptr;
        }
    case LOPT_NOISE_H_4:
        switch (disp) {
        case 0: return "All";
        case 1: return "+A[0] 1:1024 x0.564374";
        case 2: return "+B[0] 1:1005 x0.564374";
        case 3: return "+A[1] 1:512  x0.282187";
        case 4: return "+B[1] 1:502  x0.282187";
        default: return nullptr;
        }
    case LOPT_NOISE_C_4:
        switch (disp) {
        case 0: return "All";
        case 1: return "+A[0] 1:2048 x0.751468";
        case 2: return "+B[0] 1:2011 x0.751468";
        case 3: return "+A[1] 1:1024 x0.375734";
        case 4: return "+B[1] 1:1005 x0.375734";
        case 5: return "+A[2] 1:512  x0.375734";
        case 6: return "+B[2] 1:502  x0.375734";
        case 7: return "+A[3] 1:256  x0.187867";
        case 8: return "+B[3] 1:251  x0.187867";
        case 9: return "+A[4] 1:128  x0.093933";
        case 10: return "+B[4] 1:125  x0.093933";
        case 11: return "+A[5] 1:64   x0.023483";
        case 12: return "+B[5] 1:62   x0.023483";
        case 13: return "+A[6] 1:32   x0.011742";
        case 14: return "+B[6] 1:31   x0.011742";
        default: return nullptr;
        }
    case LOPT_NOISE_E_4:
        switch (disp) {
        case 0: return "All";
        case 1: return "+A[0] 1:2048 x0.716846";
        case 2: return "+B[0] 1:2011 x0.716846";
        case 3: return "+A[1] 1:1024 x0.358423";
        case 4: return "+B[1] 1:1005 x0.358423";
        case 5: return "+A[2] 1:256  x0.089606";
        case 6: return "+B[2] 1:251  x0.089606";
        case 7: return "+A[3] 1:128  x0.044803";
        case 8: return "+B[3] 1:125  x0.044803";
        default: return nullptr;
        }
    case LOPT_NOISE_W_4:
        switch (disp) {
        case 0: return "All";
        case 1: return "+A[0] 1:512 x0.634921";
        case 2: return "+B[0] 1:502 x0.634921";
        case 3: return "+A[1] 1:256 x0.634921";
        case 4: return "+B[1] 1:251 x0.634921";
        case 5: return "+A[2] 1:128 x0.158730";
        case 6: return "+B[2] 1:125 x0.158730";
        default: return nullptr;
        }
    case LOPT_HEIGHT_4:
        switch (disp) {
        case 0: return "Grayscale";
        case 1: return "Shaded biome map";
        case 2: return "Contours on biomes";
        case 3: return "Shaded with contours";
        default: return nullptr;
        }
    default:
        return nullptr;
    }
}

LayerDialog::LayerDialog(QWidget *parent, int mc)
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
        QStringList items;
        for (int j = 0; ; j++)
        {
            const char *item = getLayerOptionText(i, j);
            if (!item)
                break;
            QString s = QString::asprintf("%-24s", item);
            if (j < 9)
                s += "\tALT+"+QString::number(j+1);
            items.append(s);
        }
        combo[i]->addItems(items);
    }

    for (int i = 0; i < LOPT_MAX; i++)
    {
        if (!radio[i])
            continue;
        connect(radio[i], &QRadioButton::toggled, this, &LayerDialog::onRadioChange);
        if (combo[i])
            combo[i]->setFont(*gp_font_mono);
        if (i >= LOPT_NOISE_T_4 && i <= LOPT_NOISE_W_4)
        {
            radio[i]->setEnabled(mc > MC_1_17);
            if (combo[i])
                combo[i]->setEnabled(mc > MC_1_17);
        }
        if (i == LOPT_RIVER_4 || i == LOPT_OCEAN_256)
        {
            radio[i]->setEnabled(mc > MC_1_12 && mc <= MC_1_17);
        }
        if (i == LOPT_NOOCEAN_1)
        {
            radio[i]->setEnabled(mc <= MC_B1_7);
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

void LayerDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::StandardButton b = ui->buttonBox->standardButton(button);
    if (b == QDialogButtonBox::Ok || b == QDialogButtonBox::Apply)
        emit apply();
}

