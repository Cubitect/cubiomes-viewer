#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    QString text = ui->label->text();
    text.replace("_DATE_", QString(__DATE__));
    text.replace("_QT_MAJOR_", QString::number(QT_VERSION_MAJOR));
    text.replace("_QT_MINOR_", QString::number(QT_VERSION_MINOR));
    text.replace("_MAJOR_", QString::number(VERS_MAJOR));
    text.replace("_MINOR_", QString::number(VERS_MINOR));
    text.replace("_PATCH_", QString::number(VERS_PATCH));
    ui->label->setText(text);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
