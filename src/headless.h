#ifndef HEADLESS_H
#define HEADLESS_H

#include "searchthread.h"
#include <QTimer>
#include <QFile>

class Headless : public QObject
{
    Q_OBJECT

public:
    Headless(QString sessionpath, QString resultspath);
    virtual ~Headless();

    bool loadSession(QString sessionpath);

public slots:
    void start();
    void searchResult(uint64_t seed);
    void searchFinish(bool done);
    void progressTimeout();

public:
    SearchMaster sthread;
    QString sessionpath;
    Session session;
    std::vector<uint64_t> results;
    QFile resultfile;
    QTextStream resultstream;
    QTimer timer;
};

#endif // HEADLESS_H
