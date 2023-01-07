#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "lua/src/lua.hpp"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    QString text = ui->label->text();
    text.replace("_DATE_", QString(__DATE__));
    text.replace("_QT_MAJOR_", QString::number(QT_VERSION_MAJOR));
    text.replace("_QT_MINOR_", QString::number(QT_VERSION_MINOR));
    text.replace("_VERSION_", getVersStr());
    text.replace("_LUA_VERS_", LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "." LUA_VERSION_RELEASE);

    ui->label->setText(text);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
