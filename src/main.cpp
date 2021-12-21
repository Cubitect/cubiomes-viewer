#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QSettings>
#include <QLibraryInfo>
#include <QDebug>

#include "quad.h"

#include "cubiomes/generator.h"
#include "cubiomes/util.h"

unsigned char biomeColors[256][3];
unsigned char tempsColors[256][3];

bool loadTranslation(QTranslator *translatorCubiomes, QTranslator *translatorQt, QLocale lang)
{
    QString qtTrDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    for (const QString &locale : lang.uiLanguages()) {
        if (translatorCubiomes->load(QLocale(locale), "", "", ":/i18n", ".qm") &&
                translatorQt->load(QLocale(locale), "qt_", "", qtTrDir, ".qm")) {
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    initBiomes();
    initBiomeColors(biomeColors);
    initBiomeTypeColors(tempsColors);

    QApplication a(argc, argv);

    QSettings settings("cubiomes-viewer", "cubiomes-viewer");
    QString lang = settings.value("config/language", "C").toString();
    QLocale locale = QLocale(lang);
    if (QLocale::C == locale.language()) {
        locale = QLocale::system();
    }

//    if (!loadTranslation(&a, locale))
//        loadTranslation(&a, QLocale("en_US"));

    QTranslator translatorCubiomes, translatorQt;
    if (loadTranslation(&translatorCubiomes, &translatorQt, locale) ||
            loadTranslation(&translatorCubiomes, &translatorQt, QLocale("en_US")))
    {
        a.installTranslator(&translatorCubiomes);
        a.installTranslator(&translatorQt);
    }

    MainWindow mw;
    mw.show();
    int ret = a.exec();

    return ret;
}
