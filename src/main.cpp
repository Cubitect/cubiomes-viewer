#include "mainwindow.h"
#include "headless.h"

#include "aboutdialog.h"
#include "world.h"

#include <QApplication>
#include <QFontDatabase>
#include <QStandardPaths>
#include <QDir>

#include "cubiomes/generator.h"
#include "cubiomes/util.h"

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
    initBiomeColors(g_biomeColors);
    initBiomeTypeColors(g_tempsColors);

    QCoreApplication::setApplicationName(APP_STRING);

    bool version = false;
    bool nogui = false;
    bool reset = false;
    bool usage = false;
    QString sessionpath;
    QString resultspath;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--version") == 0)
            version = true;
        else if (strcmp(argv[i], "--nogui") == 0)
            nogui = true;
        else if (strcmp(argv[i], "--reset-all") == 0)
            reset = true;
        else if (strncmp(argv[i], "--session=", 10) == 0)
            sessionpath = argv[i] + 10;
        else if (strncmp(argv[i], "--session", 9) == 0 && i+1 < argc)
            sessionpath = argv[++i];
        else if (strncmp(argv[i], "--out=", 6) == 0)
            resultspath = argv[i] + 6;
        else if (strncmp(argv[i], "--out", 5) == 0 && i+1 < argc)
            resultspath = argv[++i];
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
            usage = true;
    }

    if (usage)
    {
        const char *msg =
                "Usage: cubiomes-viewer [options]\n"
                "Options:\n"
                "      --help                 Display this help and exit.\n"
                "      --version              Output version information and exit.\n"
                "      --nogui                Run in headless search mode.\n"
                "      --reset-all            Clear settings and remove all session data.\n"
                "      --session=file         Open this session file.\n"
                "      --out=file             Write matching seeds to this file while searching.\n"
                "\n";
        printf("%s", msg);
        exit(0);
    }
    if (version)
    {
        printf("%s %s\n", APP_STRING, getVersStr().toLocal8Bit().data());
        exit(0);
    }

    if (reset)
    {
        QSettings settings(APP_STRING, APP_STRING);
        settings.clear();

        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir dir(path);
        if (dir.exists() && path.contains(APP_STRING))
        {
            dir.removeRecursively();
        }
    }

    if (sessionpath.isEmpty())
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir dir(path);
        if (!dir.exists())
            dir.mkpath(".");
        sessionpath = path + "/session.save";
    }

    if (nogui)
    {
        QCoreApplication app(argc, argv);
        Headless headless(sessionpath, resultspath, &app);

        QObject::connect(&headless, SIGNAL(finished()), &app, SLOT(quit()));
        QTimer::singleShot(0, &headless, SLOT(run()));

        return app.exec();
    }
    else
    {
        QApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, false);

        QApplication app(argc, argv);

        MainWindow mw(sessionpath, resultspath);
        mw.show();
        return app.exec();
    }
}
