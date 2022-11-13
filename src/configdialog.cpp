#include "configdialog.h"
#include "ui_configdialog.h"

#include "biomecolordialog.h"
#include "structuredialog.h"
#include "world.h"
#include "cutil.h"

#include <QThread>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>


ConfigDialog::ConfigDialog(QWidget *parent, Config *config)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
    , structVisModified()
{
    ui->setupUi(this);

#if !WITH_UPDATER
    int miscidx = ui->gridLayout->indexOf(ui->groupMisc);
    if (miscidx >= 0)
    {
        delete ui->gridLayout->takeAt(miscidx);
        ui->groupMisc->hide();
    }
#endif

    ui->buttonBiomeColor->setFont(*gp_font_mono);

    ui->lineMatching->setValidator(new QIntValidator(1, 99999999, ui->lineMatching));

    initSettings(config);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::initSettings(Config *config)
{
    ui->checkDockable->setChecked(config->dockable);
    ui->checkSmooth->setChecked(config->smoothMotion);
    ui->checkBBoxes->setChecked(config->showBBoxes);
    ui->checkRestore->setChecked(config->restoreSession);
    ui->checkUpdates->setChecked(config->checkForUpdates);
    ui->checkAutosave->setChecked(config->autosaveCycle != 0);
    if (config->autosaveCycle)
        ui->spinAutosave->setValue(config->autosaveCycle);
    ui->comboStyle->setCurrentIndex(config->uistyle);
    ui->lineMatching->setText(QString::number(config->maxMatching));
    ui->lineGridSpacing->setText(config->gridSpacing ? QString::number(config->gridSpacing) : "");
    ui->spinCacheSize->setValue(config->mapCacheSize);
    ui->lineSep->setText(config->separator);
    int idx = config->quote == "\'" ? 1 : config->quote== "\"" ? 2 : 0;
    ui->comboQuote->setCurrentIndex(idx);

    setBiomeColorPath(config->biomeColorPath);
}

Config ConfigDialog::getSettings()
{
    conf.dockable = ui->checkDockable->isChecked();
    conf.smoothMotion = ui->checkSmooth->isChecked();
    conf.showBBoxes = ui->checkBBoxes->isChecked();
    conf.restoreSession = ui->checkRestore->isChecked();
    conf.checkForUpdates = ui->checkUpdates->isChecked();
    conf.autosaveCycle = ui->checkAutosave->isChecked() ? ui->spinAutosave->value() : 0;
    conf.uistyle = ui->comboStyle->currentIndex();
    conf.maxMatching = ui->lineMatching->text().toInt();
    conf.gridSpacing = ui->lineGridSpacing->text().toInt();
    conf.mapCacheSize = ui->spinCacheSize->value();
    conf.separator = ui->lineSep->text();
    int idx = ui->comboQuote->currentIndex();
    conf.quote = idx == 1 ? "\'" : idx == 2 ? "\"" : "";

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
            QString txt = tr("[%n biome(s)] %1", "", n).arg(finfo.baseName());
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

void ConfigDialog::on_buttonBiomeColorEditor_clicked()
{
    BiomeColorDialog *dialog = new BiomeColorDialog(this, conf.biomeColorPath, -1, DIM_UNDEF);
    if (dialog->exec() == QDialog::Accepted)
        setBiomeColorPath(dialog->getRc());
}

void ConfigDialog::on_buttonBiomeColor_clicked()
{
    QFileInfo finfo(conf.biomeColorPath);
    QString fnam = QFileDialog::getOpenFileName(
        this, tr("Load biome color map"), finfo.absolutePath(), tr("Text files (*.txt);;Any files (*)"));
    if (!fnam.isNull())
    {
        conf.biomeColorPath = fnam;
        setBiomeColorPath(fnam);
    }
}

void ConfigDialog::on_buttonStructVisEdit_clicked()
{
    StructureDialog *dialog = new StructureDialog(this);
    if (dialog->exec() == QDialog::Accepted)
    {
        if ((structVisModified |= dialog->modified))
            saveStructVis(dialog->structvis);
    }
}

void ConfigDialog::on_buttonClear_clicked()
{
    conf.biomeColorPath.clear();
    setBiomeColorPath("");
}

void ConfigDialog::on_buttonColorHelp_clicked()
{
    QString msg = tr(
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
            );
    QMessageBox::information(this, tr("Help: custom biome colors"), msg, QMessageBox::Ok);
}


