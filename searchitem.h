#ifndef SEARCHITEM_H
#define SEARCHITEM_H

#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QVector>
#include <QElapsedTimer>

#include "search.h"

#define PRECOMPUTE48_BUFSIZ ((int64_t)1 << 30)

typedef unsigned __int128 uint128_t;

// search type options from combobox
enum { SEARCH_INC = 0, SEARCH_BLOCKS = 1, SEARCH_LIST = 2 };

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

    inline bool testSeed(StructPos *spos, int64_t seed, LayerStack *g, bool s48check)
    {
        const Condition *c, *ce = cond + ccnt;
        if (s48check)
        {
            for (c = cond; c != ce; c++)
                if (!testCond(spos, seed, c, mc, NULL, abort))
                    return false;
        }
        for (c = cond; c != ce; c++)
        {
            if (g_filterinfo.list[c->type].cat == CAT_48)
                continue;
            if (!testCond(spos, seed, c, mc, g, abort))
               return false;
        }
        return true;
    }

signals:
    int results(QVector<int64_t> seeds, bool countonly);
    void itemDone(uint64_t itemid, int64_t seed, bool isdone);
    void canceled(uint64_t itemid);

public:
    int                 searchtype;
    int                 mc;
    const Condition   * cond;
    int                 ccnt;
    uint64_t            itemid;     // item identifier
    const int64_t     * slist;      // candidate list
    int64_t             len;        // number of candidates
    int64_t             idx;        // current index in candidate buffer
    int64_t             sstart;     // starting seed
    int                 scnt;       // number of seeds to process in this item
    int64_t             seed;       // (out) current seed while processing
    bool                isdone;     // (out) has the final seed been reached
    std::atomic_bool  * abort;

    // the end seed is highest unsigned seed value in the search space
    // (or the last entry in the seed list)
};

struct SearchItemGenerator
{
    void init(
        int mc, const Condition *cond, int ccnt,
        const std::vector<int64_t>& slist,
        int itemsize, int searchtype, int64_t sstart);


    SearchItem *requestItem();
    void getProgress(uint64_t *prog, uint64_t *end);

    int                     searchtype;
    int                     mc;
    const Condition       * cond;
    int                     ccnt;
    uint64_t                itemid;     // item incrementor
    int                     itemsiz;    // number of seeds per search item
    std::vector<int64_t>    slist;      // candidate list
    uint64_t                idx;        // index within candidate list
    uint64_t                scnt;       // size of search space
    int64_t                 seed;       // current seed (next to be processed)
    bool                    isdone;
    std::atomic_bool      * abort;
};



#endif // SEARCHITEM_H
