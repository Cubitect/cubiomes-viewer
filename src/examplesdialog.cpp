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

    /*
    addExample(":/examples/biome_diversity_1_17.txt",
        tr("Biome Diverity (1.7-1.17)"),
        tr("Get a large biomes diversity within 1000 blocks of the origin."),
        "", wi.mc >= MC_1_7 && wi.mc <= MC_1_17);
    */

    addExample(":/examples/biome_diversity_1_18.txt",
        tr("Biome Diverity (1.18)"),
        tr("A wide range of climates near the origin."),
        tr("(Does not look for any particular biomes.)"),
        wi.mc >= MC_1_18);

    addExample(":/examples/huge_jungle_1_18.txt",
        tr("Large Jungle (1.18)"),
        tr("A large Jungle biome at the origin."),
        tr("Looks for a suitable climate that primarily supports Jungle variants."),
        wi.mc >= MC_1_18);

    addExample(":/examples/large_birch_forest_1_18.txt",
        tr("Large Birch Forest (1.18)"),
        tr("A large Birch Forest biome at the origin."),
        tr("Looks for a climate that supports Birch Forest variants. "
        "Swamps and Meadows can generate in the same climates and are explicitly excluded."),
        wi.mc >= MC_1_18);

    addExample(":/examples/old_growth_taiga_somewhere.txt",
        tr("Large Old Growth Taiga somewhere (1.18)"),
        tr("A large Old Growth Taiga biome somewhere within 2500 blocks."),
        tr("Searches an area of +/-2500 blocks for a large climate region that "
        "primarily supports Old Growth Taiga variants"),
        wi.mc >= MC_1_18);
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

