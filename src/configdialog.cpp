#include "configdialog.h"
#include "ui_configdialog.h"
#include "cutil.h"

#include <QThread>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

ConfigDialog::ConfigDialog(QWidget *parent, Config *config) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    QFont mono = QFont("Monospace", 9);
    mono.setStyleHint(QFont::TypeWriter);
    ui->buttonBiomeColor->setFont(mono);

    ui->lineQueueSize->setValidator(new QIntValidator(1, 9999, ui->lineQueueSize));
    ui->lineMatching->setValidator(new QIntValidator(1, 99999999, ui->lineMatching));
    for (int i = 0; i < 16; i++)
        ui->cboxItemSize->addItem(QString::number(1 << i));
    ui->lineGridSpacing->setValidator(new QIntValidator(0, 1048576, ui->lineQueueSize));

    initSettings(config);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::initSettings(Config *config)
{
    ui->checkSmooth->setChecked(config->smoothMotion);
    ui->checkBBoxes->setChecked(config->showBBoxes);
    ui->checkRestore->setChecked(config->restoreSession);
    ui->checkAutosave->setChecked(config->autosaveCycle != 0);
    if (config->autosaveCycle)
        ui->spinAutosave->setValue(config->autosaveCycle);
    ui->comboStyle->setCurrentIndex(config->uistyle);
    ui->cboxItemSize->setCurrentText(QString::number(config->seedsPerItem));
    ui->lineQueueSize->setText(QString::number(config->queueSize));
    ui->lineMatching->setText(QString::number(config->maxMatching));
    ui->lineGridSpacing->setText(config->gridSpacing ? QString::number(config->gridSpacing) : "");

    setBiomeColorPath(config->biomeColorPath);
}

Config ConfigDialog::getSettings()
{
    conf.restoreSession = ui->checkRestore->isChecked();
    conf.showBBoxes = ui->checkBBoxes->isChecked();
    conf.autosaveCycle = ui->checkAutosave->isChecked() ? ui->spinAutosave->value() : 0;
    conf.smoothMotion = ui->checkSmooth->isChecked();
    conf.uistyle = ui->comboStyle->currentIndex();
    conf.seedsPerItem = ui->cboxItemSize->currentText().toInt();
    conf.queueSize = ui->lineQueueSize->text().toInt();
    conf.maxMatching = ui->lineMatching->text().toInt();
    conf.gridSpacing = ui->lineGridSpacing->text().toInt();

    if (!conf.seedsPerItem) conf.seedsPerItem = 1024;
    if (!conf.queueSize) conf.queueSize = QThread::idealThreadCount();
    if (!conf.maxMatching) conf.maxMatching = 65536;

    return conf;
}

void ConfigDialog::setBiomeColorPath(QString path)
{
    unsigned char cols[256][3];
    initBiomeColors(cols);
    QPalette pal = this->palette();

    conf.biomeColorPath = path;

    if (path.isEmpty())
    {
        ui->buttonBiomeColor->setText("...");
    }
    else
    {
        QFileInfo finfo(path);
        QFile file(path);
        int n = -1;

        if (file.open(QIODevice::ReadOnly))
        {
            char buf[32*1024];
            qint64 siz = file.read(buf, sizeof(buf)-1);
            file.close();
            if (siz >= 0)
            {
                buf[siz] = 0;
                n = parseBiomeColors(cols, buf);
            }
        }

        if (n >= 0)
        {
            QString txt = QString::asprintf("[%d biomes] ", n) + finfo.baseName();
            ui->buttonBiomeColor->setText(txt);
        }
        else
        {
            ui->buttonBiomeColor->setText(finfo.baseName());
            pal.setColor(QPalette::ButtonText, QColor(Qt::red));
        }
    }

    ui->buttonBiomeColor->setPalette(pal);
}

void ConfigDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole)
    {
        conf.reset();
        initSettings(&conf);
    }
}

void ConfigDialog::on_buttonBiomeColor_clicked()
{
    QFileInfo finfo(conf.biomeColorPath);
    QString fnam = QFileDialog::getOpenFileName(this, "Load biome color map", finfo.absolutePath(), "Text files (*.txt);;Any files (*)");
    if (!fnam.isNull())
    {
        conf.biomeColorPath = fnam;
        setBiomeColorPath(fnam);
    }
}

void ConfigDialog::on_buttonClear_clicked()
{
    conf.biomeColorPath.clear();
    setBiomeColorPath("");
}

void ConfigDialog::on_buttonColorHelp_clicked()
{
    const char* msg =
            "<html><head/><body><p>"
            "<b>Custom biome colors</b> should be defined in an ASCII text file, "
            "with one biome-color mapping per line. Each mapping should consist "
            "of a biome ID or biome resource name followed by a color that can be "
            "written as a hex code (prefixed with # or 0x) or as an RGB triplet. "
            "Special characters are ignored."
            "</p><p>"
            "<b>Examples:</b>"
            "</p><p>"
            "sunflower_plains:&nbsp;#FFFF00"
            "</p><p>"
            "128&nbsp;[255&nbsp;255&nbsp;0]"
            "</p></body></html>"
            ;
    QMessageBox::information(this, "Help: custom biome colors", msg, QMessageBox::Ok);
}
