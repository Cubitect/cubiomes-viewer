#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QFontDatabase>

#include "world.h"

#include "cubiomes/generator.h"
#include "cubiomes/util.h"

/// globals

unsigned char g_biomeColors[256][3];
unsigned char g_tempsColors[256][3];

ExtGenSettings g_extgen;

extern "C"
int getStructureConfig_override(int stype, int mc, StructureConfig *sconf)
{
    if unlikely(mc == INT_MAX) // to check if override is enabled in cubiomes
        mc = 0;
    int ok = getStructureConfig(stype, mc, sconf);
    if (ok && g_extgen.saltOverride)
    {
        uint64_t salt = g_extgen.salts[stype];
        if (salt <= MASK48)
            sconf->salt = salt;
    }
    return ok;
}

int main(int argc, char *argv[])
{
    initBiomes();
    initBiomeColors(g_biomeColors);
    initBiomeTypeColors(g_tempsColors);

    QApplication a(argc, argv);
    QCoreApplication::setApplicationName("cubiomes-viewer");

    QTranslator translator;
    translator.load("en_US", ":/lang");
    a.installTranslator(&translator);

    //int fontid = QFontDatabase::addApplicationFont(":/fonts/test.ttf");
    int fontid = QFontDatabase::addApplicationFont(":/fonts/DejaVuSans.ttf");
    if (fontid >= 0)
    {
        QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-Bold.ttf");
        QFont fontdef = QFontDatabase::applicationFontFamilies(fontid).at(0);
        fontdef.setPointSize(10);
        a.setFont(fontdef);
    }

    MainWindow mw;
    mw.show();
    int ret = a.exec();

    return ret;
}
