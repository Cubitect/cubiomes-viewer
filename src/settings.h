#ifndef SETTINGS_H
#define SETTINGS_H

#include <QThread>

enum { STYLE_SYSTEM, STYLE_DARK };

struct Config
{
    bool smoothMotion;
    bool restoreSession;
    int autosaveCycle;
    int uistyle;
    int seedsPerItem;
    int queueSize;
    int maxMatching;

    Config() { reset(); }

    void reset()
    {
        smoothMotion = true;
        restoreSession = true;
        autosaveCycle = 10;
        uistyle = STYLE_DARK;
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
    uint64_t salt;
    uint64_t listsalt;
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
        listsalt = 0;
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
    int searchtype;
    QString slist64path;
    int threads;
    uint64_t startseed;
    bool stoponres;
    uint64_t smin;
    uint64_t smax;

    SearchConfig() { reset(); }

    void reset()
    {
        searchtype = SEARCH_INC;
        slist64path = "";
        threads = QThread::idealThreadCount();
        startseed = 0;
        stoponres = true;
        smin = 0;
        smax = ~(uint64_t)0;
    }
};


#endif // SETTINGS_H
