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

QFont g_font_default;
QFont g_font_mono;

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

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("cubiomes-viewer");

    QTranslator translator;
    translator.load("en_US", ":/lang");
    app.installTranslator(&translator);

    //int fontid = QFontDatabase::addApplicationFont(":/fonts/test.ttf");
    int fontid = QFontDatabase::addApplicationFont(":/fonts/DejaVuSans.ttf");
    if (fontid >= 0)
    {
        QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-Bold.ttf");
        int fontid_mono = QFontDatabase::addApplicationFont(":/fonts/DejaVuSansMono.ttf");

        g_font_default = QFontDatabase::applicationFontFamilies(fontid).at(0);
        g_font_default.setPointSize(10);
        g_font_mono = QFontDatabase::applicationFontFamilies(fontid_mono).at(0);
        g_font_mono.setPointSize(9);

        app.setFont(g_font_default);
    }

    MainWindow mw;
    mw.show();
    int ret = app.exec();

    return ret;
}
