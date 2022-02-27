#include "examplesdialog.h"
#include "ui_examplesdialog.h"

#include "cutil.h"

#include <QPushButton>
#include <QRadioButton>


ExamplesDialog::ExamplesDialog(QWidget *parent, WorldInfo wi) :
    QDialog(parent),
    ui(new Ui::ExamplesDialog)
{
    ui->setupUi(this);
    ui->labelMC->setText(tr("MC ") + mc2str(wi.mc));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    addExample(":/examples/quadhut_stronghold_mushroom.txt",
        tr("Technical Base (1.4+)"),
        tr("An ideal Quad-hut next to a Stronghold with a Mushroom Island close by."),
        "", wi.mc >= MC_1_4);

    /*
    addExample(":/examples/mushroom_icespike.txt",
        tr("Analyze for location with rare biomes together (1.7+)"),
        tr("Check a large area for a Mushroom Island that is next to Ice Spikes."),
        tr("Use this in the Analysis Tab with the Condition trigger enabled to "
        "find an instance in the current seed."),
        wi.mc >= MC_1_7);
    */

    addExample(":/examples/village_portal_stronghold.txt",
        tr("Speedrunner Village (1.16+)"),
        tr("Spawn in a Village with a Ruined Portal leading to a Stronghold."),
        tr("Works best with a large search item size and with the 48-bit family search."),
        wi.mc >= MC_1_16);
}

ExamplesDialog::~ExamplesDialog()
{
    delete ui;
}

void ExamplesDialog::addExample(QString rc, QString title, QString desc, QString comment, bool enabled)
{
    QRadioButton *radio = new QRadioButton(title, this);
    examples[radio] = Texts{rc, title, desc, comment};
    radio->setEnabled(enabled);
    connect(radio, &QRadioButton::toggled, [=]() { this->onSelectionChange(radio); });

    int row = ui->gridLayout->rowCount();
    ui->gridLayout->addWidget(radio, row, 0);
}

QString ExamplesDialog::getExample()
{
    for (auto& e : examples)
    {
        if (e.first->isChecked())
            return e.second.rc;
    }
    return "";
}

void ExamplesDialog::onSelectionChange(QRadioButton *ex)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    if (ex)
    {
        QString txt = examples[ex].desc;
        if (!examples[ex].comment.isEmpty())
            txt += "\n\n" + examples[ex].comment;
        ui->textDesc->setText(txt);
    }
}

