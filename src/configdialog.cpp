#include "configdialog.h"
#include "ui_configdialog.h"

#include <QThread>

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
    ui->checkSmooth->setChecked(config->smoothMotion);
    ui->checkRestore->setChecked(config->restoreSession);
    ui->checkAutosave->setChecked(config->autosaveCycle != 0);
    if (config->autosaveCycle)
        ui->spinAutosave->setValue(config->autosaveCycle);
    ui->comboStyle->setCurrentIndex(config->uistyle);
    ui->cboxItemSize->setCurrentText(QString::number(config->seedsPerItem));
    ui->lineQueueSize->setText(QString::number(config->queueSize));
    ui->lineMatching->setText(QString::number(config->maxMatching));
}

Config ConfigDialog::getSettings()
{
    conf.restoreSession = ui->checkRestore->isChecked();
    conf.autosaveCycle = ui->checkAutosave->isChecked() ? ui->spinAutosave->value() : 0;
    conf.smoothMotion = ui->checkSmooth->isChecked();
    conf.uistyle = ui->comboStyle->currentIndex();
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
