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
    QString msg = tr(
            "This may take a moment. Results will be saved to:"
            "\n\n\"%1\"\n\n"
            "so subsequent searches will start faster.").arg(path);
    ui->label->setText(msg);
}
