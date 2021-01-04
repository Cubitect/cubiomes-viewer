#include "gotodialog.h"
#include "ui_gotodialog.h"

#include "mainwindow.h"

#include <QDoubleValidator>

GotoDialog::GotoDialog(MainWindow *parent, qreal x, qreal z)
    : QDialog(parent)
    , ui(new Ui::GotoDialog)
    , mainwindow(parent)
{
    ui->setupUi(this);

    QDoubleValidator *val = new QDoubleValidator(-3e7, 3e7, 0, this);
    ui->lineX->setValidator(val);
    ui->lineZ->setValidator(val);

    ui->lineX->setText(QString::asprintf("%.1f", x));
    ui->lineZ->setText(QString::asprintf("%.1f", z));
}

GotoDialog::~GotoDialog()
{
    delete ui;
}

void GotoDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::StandardButton b = ui->buttonBox->standardButton(button);

    if (b == QDialogButtonBox::Ok || b == QDialogButtonBox::Apply)
        mainwindow->mapGoto(ui->lineX->text().toDouble(), ui->lineZ->text().toDouble());
    else if (b == QDialogButtonBox::Reset)
    {
        ui->lineX->setText("0");
        ui->lineZ->setText("0");
    }
}
