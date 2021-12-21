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
    ui->label->setText(tr(
            "This may take a moment.\n"
            "Results will be saved to \"%1\" so subsequent searches will start faster.").arg(path));
}
