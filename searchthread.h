#ifndef SEARCHTHREAD_H
#define SEARCHTHREAD_H

#include <QThread>
#include <QThreadPool>
#include <QMutex>
#include <QVector>
#include <QElapsedTimer>

#include "searchitem.h"

#define PRECOMPUTE48_BUFSIZ ((int64_t)1 << 30)


class MainWindow;

struct SearchThread : QThread
{
    Q_OBJECT
public:
    struct CheckedSeed
    {
        uint8_t valid;
        int64_t seed;
    };

    SearchThread(MainWindow *parent);

    bool set(int type, int threads, std::vector<int64_t>& slist64, int64_t sstart, int mc,
             const QVector<Condition>& cv, int itemsize, int queuesize);

    virtual void run() override;

    void stop() { abort = true; pool.clear(); }
    SearchItem *startNextItem();
    void debug();

signals:
    void progress(uint64_t last, uint64_t end, int64_t seed);
    void searchFinish();

public slots:
    void onItemDone(uint64_t itemid, int64_t seed, bool isdone);
    void onItemCanceled(uint64_t itemid);

public:
    MainWindow            * parent;

    QVector<Condition>      condvec;
    SearchItemGenerator     itemgen;
    QThreadPool             pool;
    QAtomicInt              activecnt;  // running + queued items
    std::atomic_bool        abort;
    std::atomic_bool        reqstop;

    QVector<CheckedSeed>    recieved;
    uint64_t                lastid;     // last item id
};

#endif // SEARCHTHREAD_H
