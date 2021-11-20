#include "extgendialog.h"
#include "ui_extgendialog.h"

#include "cutil.h"

#include <QGridLayout>

ExtGenDialog::ExtGenDialog(QWidget *parent, ExtGenSettings *extgen)
    : QDialog(parent)
    , ui(new Ui::ExtGenDialog)
    , checkSalts{}
    , lineSalts{}
{
    ui->setupUi(this);

    int stv[] = {
        Desert_Pyramid,
        Jungle_Pyramid,
        Swamp_Hut,
        Igloo,
        Village,
        Ocean_Ruin,
        Shipwreck,
        Monument,
        Mansion,
        Outpost,
        Ruined_Portal,
        Ruined_Portal_N,
        Treasure,
        //Mineshaft,
        Fortress,
        Bastion,
        End_City,
        End_Gateway,
    };

    QGridLayout *grid = new QGridLayout(ui->groupSalts);
    for (size_t i = 0; i < sizeof(stv)/sizeof(stv[0]); i++)
    {
        int st = stv[i];
        grid->addWidget((checkSalts[st] = new QCheckBox(struct2str(st))), i, 0);
        grid->addWidget((lineSalts[st] = new QLineEdit()), i, 1);
        connect(checkSalts[st], &QCheckBox::toggled, this, &ExtGenDialog::updateToggles);
    }

    Pos dummy;
    if (!getStructurePos(Feature, INT_MAX, 0, 0, 0, &dummy))
    {
        // cubiomes was not built with salt override support
        ui->groupSalts->setEnabled(false);
    }

    initSettings(extgen);
}

ExtGenDialog::~ExtGenDialog()
{
    delete ui;
}

void ExtGenDialog::initSettings(ExtGenSettings *extgen)
{
    // start checked, otherwise Qt doesn't respond to initial uncheck
    ui->groupSalts->setChecked(true);

    for (int i = 0; i < FEATURE_NUM; i++)
    {
        if (!checkSalts[i])
            continue;
        uint64_t salt = extgen->salts[i];
        if (salt != ~(uint64_t)0)
        {
            checkSalts[i]->setChecked(salt <= MASK48);
            lineSalts[i]->setText(QString::asprintf("%" PRIu64, salt & MASK48));
        }
        else
        {
            checkSalts[i]->setChecked(false);
            lineSalts[i]->setText("");
        }
    }
    updateToggles();

    ui->groupSalts->setChecked(extgen->saltOverride);
}

ExtGenSettings ExtGenDialog::getSettings()
{
    extgen.saltOverride = ui->groupSalts->isChecked();

    for (int i = 0; i < FEATURE_NUM; i++)
    {
        if (!checkSalts[i])
            continue;

        bool ok;
        uint64_t salt;
        salt = lineSalts[i]->text().toULongLong(&ok);

        if (checkSalts[i]->isChecked())
            extgen.salts[i] = salt;
        else if (ok)
            extgen.salts[i] = ok | (1ULL << 63);
        else
            extgen.salts[i] = ~(uint64_t)0;
    }

    return extgen;
}

void ExtGenDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
        extgen.reset();
        initSettings(&extgen);
    }
}

void ExtGenDialog::updateToggles()
{
    for (int i = 0; i < FEATURE_NUM; i++)
    {
        if (!checkSalts[i])
            continue;
        lineSalts[i]->setEnabled(checkSalts[i]->isChecked());
    }
}
