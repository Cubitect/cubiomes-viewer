#ifndef HEADLESS_H
#define HEADLESS_H

#include "searchthread.h"
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QFile>

class Headless : public QThread
{
    Q_OBJECT

public:
    Headless(QString sessionpath, QString resultspath, QObject *parent = 0);
    virtual ~Headless();

    bool loadSession(QString sessionpath);

public slots:
    void run();
    void searchResult(uint64_t seed);
    void searchFinish(bool done);
    void progressTimeout();

signals:
    void finished();

public:
    SearchMaster sthread;
    QString sessionpath;
    Session session;
    std::vector<uint64_t> results;
    QFile resultfile;
    QTextStream resultstream;
    QTimer timer;
    QElapsedTimer elapsed;
};

#endif // HEADLESS_H
