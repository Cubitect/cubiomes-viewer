#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>

#include "quad.h"

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

    MainWindow mw;
    mw.show();
    int ret = a.exec();

    return ret;
}
