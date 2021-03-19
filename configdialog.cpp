#include "configdialog.h"
#include "ui_configdialog.h"

#include <QThread>

void Config::reset()
{
    restoreSession = true;
    smoothMotion = true;
    seedsPerItem = 1024;
    queueSize = QThread::idealThreadCount();
    maxMatching = 65536;
}


ConfigDialog::ConfigDialog(QWidget *parent, Config *config) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);
    ui->lineQueueSize->setValidator(new QIntValidator(1, 9999, ui->lineQueueSize));
    ui->lineMatching->setValidator(new QIntValidator(1, 99999999, ui->lineMatching));
    for (int i = 0; i < 16; i++)
        ui->cboxItemSize->addItem(QString::number(1 << i));
    initSettings(config);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::initSettings(Config *config)
{
    ui->checkRestore->setChecked(config->restoreSession);
    ui->checkSmooth->setChecked(config->smoothMotion);
    ui->cboxItemSize->setCurrentText(QString::number(config->seedsPerItem));
    ui->lineQueueSize->setText(QString::number(config->queueSize));
    ui->lineMatching->setText(QString::number(config->maxMatching));
}

Config ConfigDialog::getConfig()
{
    conf.restoreSession = ui->checkRestore->isChecked();
    conf.smoothMotion = ui->checkSmooth->isChecked();
    conf.seedsPerItem = ui->cboxItemSize->currentText().toInt();
    conf.queueSize = ui->lineQueueSize->text().toInt();
    conf.maxMatching = ui->lineMatching->text().toInt();

    if (!conf.seedsPerItem) conf.seedsPerItem = 1024;
    if (!conf.queueSize) conf.queueSize = QThread::idealThreadCount();
    if (!conf.maxMatching) conf.maxMatching = 65536;

    return conf;
}

void ConfigDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
        conf.reset();
        initSettings(&conf);
    }
}
