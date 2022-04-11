#ifndef SEARCHTHREAD_H
#define SEARCHTHREAD_H

#include "search.h"
#include "settings.h"

#include <QThread>
#include <QMutex>
#include <QVector>
#include <QElapsedTimer>


class FormSearchControl;
struct SearchWorker;

struct SearchMaster : QThread
{
    Q_OBJECT
public:
    SearchMaster(FormSearchControl *parent);
    virtual ~SearchMaster();

    bool set(WorldInfo wi, const SearchConfig& sc, const Gen48Settings& gen48, const Config& config,
            std::vector<uint64_t>& slist, const QVector<Condition>& cv);

    void presearch();

    virtual void run() override;
    void stop();

    // Determines the lowest seed/prog that is about to be processed.
    // (This is the point at which you would want to reload a previous search.)
    bool getProgress(uint64_t *prog, uint64_t *end, uint64_t *seed);

    bool requestItem(SearchWorker *item);

public slots:
    void onWorkerFinished();

signals:
    void searchFinish(bool done);

public:
    FormSearchControl     * parent;
    QVector<SearchWorker*>  workers;

    QMutex                  mutex;
    std::atomic_bool        abort;

    QElapsedTimer           timer;
    uint64_t                count;

    int                     searchtype;
    int                     mc;
    int                     large;
    ConditionTree           condtree;
    int                     itemsize;   // number of seeds per search item
    int                     threadcnt;  // numbr of worker threads
    Gen48Settings           gen48;      // 48-bit generator settings
    std::vector<uint64_t>   slist;      // candidate list
    uint64_t                idx;        // index within candidate list
    uint64_t                scnt;       // search space size
    uint64_t                prog;       // search space progress tracker
    uint64_t                seed;       // current seed (next to be processed)
    uint64_t                smin;
    uint64_t                smax;
    bool                    isdone;
};


struct SearchWorker : QThread
{
    Q_OBJECT
public:
    SearchWorker(SearchMaster *master);

    virtual void run() override;

signals:
    int results(QVector<uint64_t> seeds, bool countonly);

public:
    SearchMaster      * master;

    int                 searchtype;
    int                 mc;
    int                 large;
    ConditionTree     * pctree;
    const uint64_t    * slist;      // candidate list
    uint64_t            len;        // number of candidates
    std::atomic_bool  * abort;

    /// current work item
    uint64_t            prog;       // search space progress
    uint64_t            idx;        // current index in candidate buffer
    uint64_t            sstart;     // starting seed
    int                 scnt;       // number of seeds to process in this item
    uint64_t            seed;       // (out) current seed while processing
    // the end seed is highest unsigned seed value in the search space
    // (or the last entry in the seed list)
};





#endif // SEARCHTHREAD_H
