#ifndef SETTINGS_H
#define SETTINGS_H

#include <QThread>

struct Config
{
    bool smoothMotion;
    bool restoreSession;
    int autosaveCycle;
    int seedsPerItem;
    int queueSize;
    int maxMatching;

    Config() { reset(); }

    void reset()
    {
        smoothMotion = true;
        restoreSession = true;
        autosaveCycle = 10;
        seedsPerItem = 256;
        queueSize = QThread::idealThreadCount();
        maxMatching = 65536;
    }
};

enum { GEN48_AUTO, GEN48_QH, GEN48_QM, GEN48_LIST, GEN48_NONE };
enum { IDEAL, CLASSIC, NORMAL, BARELY, IDEAL_SALTED };

struct Gen48Settings
{
    int mode;
    QString slist48path;
    int64_t salt;
    int qual;
    int qmarea;
    bool manualarea;
    int x1, z1, x2, z2;

    Gen48Settings() { reset(); }

    void reset()
    {
        mode = GEN48_AUTO;
        slist48path = "";
        salt = 0;
        qual = IDEAL;
        qmarea = 13028;
        manualarea = false;
        x1 = z1 = x2 = z2 = 0;
    }
};

// search type options from combobox
enum { SEARCH_INC = 0, SEARCH_BLOCKS = 1, SEARCH_LIST = 2 };

struct SearchConfig
{
    int searchmode;
    QString slist64path;
    int threads;
    int64_t startseed;
    bool stoponres;

    SearchConfig() { reset(); }

    void reset()
    {
        searchmode = SEARCH_INC;
        slist64path = "";
        threads = QThread::idealThreadCount();
        startseed = 0;
        stoponres = true;
    }
};


#endif // SETTINGS_H
