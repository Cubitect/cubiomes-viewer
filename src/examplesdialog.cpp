#include "examplesdialog.h"
#include "ui_examplesdialog.h"

#include <QPushButton>


ExamplesDialog::ExamplesDialog(QWidget *parent, WorldInfo wi) :
    QDialog(parent),
    ui(new Ui::ExamplesDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    addExample(":/examples/quadhut_stronghold_mushroom.txt", tr(
        "Quad-Hut with Stronghold and a close by Mushroom Island"
    ), wi.mc >= MC_1_4);

    addExample(":/examples/village_portal_stronghold.txt", tr(
        "Village near Spawn with a Portal to a Stronghold"
    ), true);

    addExample(":/examples/mushroom_icespike.txt", tr(
        "Mushroom Island anywhere next to an Ice Spike biome"
    ), wi.mc >= MC_1_7);
}

ExamplesDialog::~ExamplesDialog()
{
    delete ui;
}

void ExamplesDialog::addExample(QString rc, QString desc, bool enabled)
{
    QListWidgetItem *item = new QListWidgetItem(desc);
    item->setData(Qt::UserRole, QVariant::fromValue(rc));
    if (!enabled)
    {
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        item->setSelected(false);
        item->setBackground(QColor(128, 128, 128, 192));
    }
    ui->listExamples->addItem(item);
}

QString ExamplesDialog::getExample()
{
    QListWidgetItem *item = ui->listExamples->currentItem();
    if (item)
        return item->data(Qt::UserRole).toString();
    return "";
}


void ExamplesDialog::on_listExamples_currentRowChanged(int currentRow)
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(currentRow >= 0);
}

void ExamplesDialog::on_listExamples_itemDoubleClicked(QListWidgetItem *item)
{
    (void) item;
    done(1);
}
