#include "extgendialog.h"
#include "ui_extgendialog.h"

#include "util.h"

#include <QGridLayout>

ExtGenDialog::ExtGenDialog(QWidget *parent, ExtGenConfig *extgen)
    : QDialog(parent)
    , ui(new Ui::ExtGenDialog)
    , checkSalts{}
    , lineSalts{}
    , checkSaltsStronghold(nullptr)
    , lineSaltsStronghold(nullptr)
{
    ui->setupUi(this);

    int stv[] = {
        // Stronghold first
        // Overworld
        Ancient_City,
        Desert_Pyramid,
        Igloo,
        Jungle_Pyramid,
        Mansion,
        // Mineshaft,
        Monument,
        Ocean_Ruin,
        Outpost,
        Ruined_Portal,
        Shipwreck,
        Swamp_Hut,
        Treasure,
        Trial_Chambers,
        Village,

        // Nether
        Bastion,
        Fortress,
        Ruined_Portal_N,

        // End
        End_City,
        End_Gateway,
    };

    QGridLayout *grid = new QGridLayout(ui->groupSalts);
    
    grid->addWidget((checkSaltsStronghold = new QCheckBox("stronghold")), 0, 0);
    grid->addWidget((lineSaltsStronghold = new QLineEdit()), 0, 1);
    connect(checkSaltsStronghold, &QCheckBox::toggled, this, &ExtGenDialog::updateToggles);

    for (size_t i = 1; i < sizeof(stv)/sizeof(stv[0]); i++)
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

void ExtGenDialog::initSettings(ExtGenConfig *extgen)
{
    ui->checkExperimental->setChecked(extgen->experimentalVers);

    ui->checkEstimate->setChecked(extgen->estimateTerrain);

    // start checked, otherwise Qt doesn't respond to initial uncheck
    ui->groupSalts->setChecked(true);

    checkSaltsStronghold->setChecked(extgen->saltOverrideStronghold);
    lineSaltsStronghold->setText(QString::asprintf("%" PRIu64, extgen->saltStronghold & MASK48));

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

ExtGenConfig ExtGenDialog::getSettings()
{
    extgen.experimentalVers = ui->checkExperimental->isChecked();
    extgen.estimateTerrain = ui->checkEstimate->isChecked();
    extgen.saltOverride = ui->groupSalts->isChecked();
    
    extgen.saltOverrideStronghold = checkSaltsStronghold->isChecked();
    extgen.saltStronghold = lineSaltsStronghold->text().toULongLong();

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
    if (checkSaltsStronghold)
        lineSaltsStronghold->setEnabled(checkSaltsStronghold->isChecked());
    
    for (int i = 0; i < FEATURE_NUM; i++)
    {
        if (!checkSalts[i])
            continue;
        lineSalts[i]->setEnabled(checkSalts[i]->isChecked());
    }
}
