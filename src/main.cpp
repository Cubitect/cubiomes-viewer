#include "mainwindow.h"
#include <QApplication>

#include "quad.h"

#include "cubiomes/generator.h"
#include "cubiomes/util.h"

MainWindow *gMainWindowInstance;
unsigned char biomeColors[256][3];
unsigned char tempsColors[256][3];

int main(int argc, char *argv[])
{
    initBiomes();
    initBiomeColours(biomeColors);
    initBiomeTypeColours(tempsColors);

    QApplication a(argc, argv);
    MainWindow mw;
    gMainWindowInstance = &mw;
    mw.show();
    int ret = a.exec();
    gMainWindowInstance = NULL;

    return ret;
}
