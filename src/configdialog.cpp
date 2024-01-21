#include "configdialog.h"
#include "ui_configdialog.h"

#include "biomecolordialog.h"
#include "maptoolsdialog.h"
#include "world.h"
#include "util.h"

#include <QThread>
#include <QFileInfo>
#include <QDirIterator>
#include <QLocale>


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
    ui->lineIconScale->setValidator(new QDoubleValidator(1.0/8, 16.0, 3, ui->lineIconScale));

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

    QSize size = sizeHint();
    int hsa = ui->scrollArea->sizeHint().height();
    int hsc = ui->scrollAreaWidgetContents->sizeHint().height();
    int hpa = parent->size().height();
    int h = size.height();
    int m1, m2;
    layout()->getContentsMargins(0, &m1, 0, &m2);
    h += hsc - hsa + m1 + m2;
    if (h > hpa) h = hpa;
    size.setHeight(h);
    resize(size);
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
    ui->checkWindowPos->setChecked(config->restoreWindow);
    ui->checkUpdates->setChecked(config->checkForUpdates);
    ui->checkAutosave->setChecked(config->autosaveCycle != 0);
    if (config->autosaveCycle)
        ui->spinAutosave->setValue(config->autosaveCycle);
    ui->comboStyle->setCurrentIndex(config->uistyle);
    ui->lineMatching->setText(QString::number(config->maxMatching));
    ui->lineGridSpacing->setText(config->gridSpacing ? QString::number(config->gridSpacing) : "");
    ui->comboGridMult->setCurrentText(config->gridMultiplier ? QString::number(config->gridMultiplier) : tr("None"));
    ui->spinCacheSize->setValue(config->mapCacheSize);
    ui->spinThreads->setValue(config->mapThreads ? config->mapThreads : (QThread::idealThreadCount() + 1) / 2);
    ui->comboLang->setCurrentIndex(ui->comboLang->findData(config->lang));
    ui->lineSep->setText(config->separator);
    int idx = config->quote == "\'" ? 1 : config->quote== "\"" ? 2 : 0;
    ui->comboQuote->setCurrentIndex(idx);
    ui->fontComboNorm->setCurrentFont(config->fontNorm);
    ui->fontComboMono->setCurrentFont(config->fontMono);
    ui->spinFontSizeNorm->setValue(config->fontNorm.pointSize());
    ui->spinFontSizeMono->setValue(config->fontMono.pointSize());
    ui->lineIconScale->setText(QString::number(config->iconScale));
}

Config ConfigDialog::getConfig()
{
    conf.smoothMotion = ui->checkSmooth->isChecked();
    conf.showBBoxes = ui->checkBBoxes->isChecked();
    conf.restoreSession = ui->checkRestore->isChecked();
    conf.restoreWindow = ui->checkWindowPos->isChecked();
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

    conf.iconScale = ui->lineIconScale->text().toDouble();

    if (!conf.maxMatching) conf.maxMatching = 65536;

    return conf;
}

void ConfigDialog::onUpdateMapConfig()
{
    emit updateMapConfig();
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

void ConfigDialog::on_lineGridSpacing_textChanged(const QString &text)
{
    ui->comboGridMult->setEnabled(!text.isEmpty());
}

