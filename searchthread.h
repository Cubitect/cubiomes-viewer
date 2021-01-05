#ifndef SEARCHTHREAD_H
#define SEARCHTHREAD_H

#include <QThread>
#include <QThreadPool>
#include <QMutex>
#include <QVector>
#include <QElapsedTimer>

#include "search.h"

#define PRECOMPUTE48_BUFSIZ ((int64_t)1 << 30)


class SearchThread : public QThread
{
    Q_OBJECT

public:
    SearchThread(QObject *parent) :
        QThread(parent),mc(),sstart(),condvec(),pool(this),stoponres(),seeds(),mutex(),elapsed()
    {
    }

    bool set(int type, int64_t start48, int mc, const QVector<Condition>& cv);

    void stop() { abortsearch = true; }

    void run() override;
    bool runSearch48(int64_t s48, const Condition* cond, int ccnt);

signals:
    int results(QVector<int64_t> seeds, bool countonly);
    void baseDone(int64_t s48);
    void finish(int64_t s48);

public slots:
    void setStopOnResult(bool a) { stoponres = a; }

protected:
    int mc;
    int64_t sstart;
    QVector<Condition> condvec;
    QThreadPool pool;
    bool stoponres;
    int searchtype;

public:
    QVector<int64_t> seeds;
    QMutex mutex;
    volatile bool abortsearch;

    QElapsedTimer elapsed;
};

#endif // SEARCHTHREAD_H
