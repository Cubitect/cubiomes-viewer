#include "protobasedialog.h"
#include "ui_protobasedialog.h"

ProtoBaseDialog::ProtoBaseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProtoBaseDialog)
{
    ui->setupUi(this);
}

ProtoBaseDialog::~ProtoBaseDialog()
{
    delete ui;
}

bool ProtoBaseDialog::closeOnDone()
{
    return ui->checkBox->isChecked();
}

void ProtoBaseDialog::setPath(QString path)
{
    ui->label->setText(
            "This may take a moment.\n"
            "Results will be saved to \"" + path + "\" so subsequent searches will start faster.");
}
