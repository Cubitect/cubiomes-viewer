#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QFontDatabase>

#include "world.h"

#include "cubiomes/generator.h"
#include "cubiomes/util.h"

unsigned char biomeColors[256][3];
unsigned char tempsColors[256][3];

int main(int argc, char *argv[])
{
    initBiomes();
    initBiomeColors(biomeColors);
    initBiomeTypeColors(tempsColors);

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
