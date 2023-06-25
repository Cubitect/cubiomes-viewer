#include "configdialog.h"
#include "ui_configdialog.h"

#include "biomecolordialog.h"
#include "structuredialog.h"
#include "world.h"
#include "cutil.h"

#include <QThread>
#include <QFileInfo>
#include <QFileDialog>
#include <QDirIterator>
#include <QLocale>
#include <QMessageBox>


ConfigDialog::ConfigDialog(QWidget *parent, Config *config)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
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

    ui->lineMatching->setValidator(new QIntValidator(1, 99999999, ui->lineMatching));
    ui->spinThreads->setRange(1, QThread::idealThreadCount());

    QString rclang = ":/lang";
    QDirIterator it(rclang, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString fnam = it.next();
        if(!fnam.endsWith(".qm"))
            continue;
        QString code = QFileInfo(fnam).baseName();
        QLocale locale(code);
        QString text = QLocale::languageToString(locale.language());
        text += " (" + QLocale::countryToString(locale.country()) + ")";
        ui->comboLang->addItem(text, code);
    }

    initConfig(config);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::initConfig(Config *config)
{
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
    ui->comboGridMult->setCurrentText(config->gridMultiplier ? QString::number(config->gridMultiplier) : tr("None"));
    ui->spinCacheSize->setValue(config->mapCacheSize);
    ui->spinThreads->setValue(config->mapThreads ? config->mapThreads : QThread::idealThreadCount());
    ui->comboLang->setCurrentIndex(ui->comboLang->findData(config->lang));
    ui->lineSep->setText(config->separator);
    int idx = config->quote == "\'" ? 1 : config->quote== "\"" ? 2 : 0;
    ui->comboQuote->setCurrentIndex(idx);
    ui->fontComboNorm->setCurrentFont(config->fontNorm);
    ui->fontComboMono->setCurrentFont(config->fontMono);
    ui->spinFontSizeNorm->setValue(config->fontNorm.pointSize());
    ui->spinFontSizeMono->setValue(config->fontMono.pointSize());

    setBiomeColorPath(config->biomeColorPath);
}

Config ConfigDialog::getConfig()
{
    conf.smoothMotion = ui->checkSmooth->isChecked();
    conf.showBBoxes = ui->checkBBoxes->isChecked();
    conf.restoreSession = ui->checkRestore->isChecked();
    conf.checkForUpdates = ui->checkUpdates->isChecked();
    conf.autosaveCycle = ui->checkAutosave->isChecked() ? ui->spinAutosave->value() : 0;
    conf.uistyle = ui->comboStyle->currentIndex();
    conf.maxMatching = ui->lineMatching->text().toInt();
    conf.gridSpacing = ui->lineGridSpacing->text().toInt();
    conf.gridMultiplier = ui->comboGridMult->currentText().toInt();
    conf.mapCacheSize = ui->spinCacheSize->value();
    conf.mapThreads = ui->spinThreads->value();
    conf.lang = ui->comboLang->currentData().toString();
    conf.separator = ui->lineSep->text();
    int idx = ui->comboQuote->currentIndex();
    conf.quote = idx == 1 ? "\'" : idx == 2 ? "\"" : "";

    conf.fontNorm = ui->fontComboNorm->currentFont();
    conf.fontMono = ui->fontComboMono->currentFont();
    conf.fontNorm.setPointSize(ui->spinFontSizeNorm->value());
    conf.fontMono.setPointSize(ui->spinFontSizeMono->value());
    conf.fontNorm.setStyleHint(QFont::AnyStyle);
    conf.fontMono.setStyleHint(QFont::Monospace);

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
    int role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::ResetRole)
    {
        conf.reset();
        initConfig(&conf);
    }
    else if (role == QDialogButtonBox::AcceptRole || role == QDialogButtonBox::ApplyRole)
    {
        getConfig().save();
        emit updateConfig();
    }
}

void ConfigDialog::on_buttonBiomeColorEditor_clicked()
{
    BiomeColorDialog *dialog = new BiomeColorDialog(this, conf.biomeColorPath, -1, DIM_UNDEF);
    if (dialog->exec() == QDialog::Accepted)
    {
        QString rc = dialog->getRc();
        setBiomeColorPath(rc);
    }
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
    connect(dialog, SIGNAL(updateMapConfig()), this, SLOT(onUpdateMapConfig()));
    dialog->show();
}

void ConfigDialog::onUpdateMapConfig()
{
    emit updateMapConfig();
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

void ConfigDialog::on_lineGridSpacing_textChanged(const QString &text)
{
    ui->comboGridMult->setEnabled(!text.isEmpty());
}
