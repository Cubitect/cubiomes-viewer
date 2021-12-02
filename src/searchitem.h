#ifndef SEARCHITEM_H
#define SEARCHITEM_H

#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QVector>
#include <QElapsedTimer>

#include "settings.h"
#include "search.h"


struct SearchItem : public QObject, QRunnable
{
    Q_OBJECT
public:

    SearchItem() : QObject(),QRunnable()
    {
        setAutoDelete(true);
    }
    virtual ~SearchItem();

    virtual void run();

signals:
    int results(QVector<uint64_t> seeds, bool countonly);
    void itemDone(uint64_t itemid, uint64_t seed, bool isdone);
    void canceled(uint64_t itemid);

public:
    int                 searchtype;
    int                 mc;
    int                 large;
    QVector<Condition>* pcvec;
    uint64_t            itemid;     // item identifier
    const uint64_t    * slist;      // candidate list
    uint64_t            len;        // number of candidates
    uint64_t            idx;        // current index in candidate buffer
    uint64_t            sstart;     // starting seed
    int                 scnt;       // number of seeds to process in this item
    uint64_t            seed;       // (out) current seed while processing
    bool                isdone;     // (out) has the final seed been reached
    std::atomic_bool  * abort;

    // the end seed is highest unsigned seed value in the search space
    // (or the last entry in the seed list)
};


struct SearchItemGenerator
{
    void init(
        QObject *mainwin, WorldInfo wi,
        const SearchConfig& sc, const Gen48Settings& gen48, const Config& config,
        const std::vector<uint64_t>& slist, const QVector<Condition>& cv);

    void presearch();

    SearchItem *requestItem();
    void getProgress(uint64_t *prog, uint64_t *end);

    QObject               * mainwin;
    int                     searchtype;
    int                     mc;
    int                     large;
    QVector<Condition>      condvec;
    uint64_t                itemid;     // item incrementor
    int                     itemsiz;    // number of seeds per search item
    Gen48Settings           gen48;      // 48-bit generator settings
    std::vector<uint64_t>   slist;      // candidate list
    uint64_t                idx;        // index within candidate list
    uint64_t                scnt;       // size of search space
    uint64_t                seed;       // current seed (next to be processed)
    uint64_t                idxmin;     // idx of smin
    uint64_t                smin;
    uint64_t                smax;
    bool                    isdone;
    bool                    isstart;
    std::atomic_bool      * abort;
};



#endif // SEARCHITEM_H
