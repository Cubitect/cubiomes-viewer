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
            QObject *mainwin, int mc, const Condition *cond, int ccnt,
            Gen48Settings gen48, const std::vector<int64_t>& seedlist,
            int itemsize, int searchtype, int64_t sstart);

    void presearch();

    SearchItem *requestItem();
    void getProgress(uint64_t *prog, uint64_t *end);

    QObject               * mainwin;
    int                     searchtype;
    int                     mc;
    const Condition       * cond;
    int                     ccnt;
    uint64_t                itemid;     // item incrementor
    int                     itemsiz;    // number of seeds per search item
    Gen48Settings           gen48;      // 48-bit generator settings
    std::vector<int64_t>    slist;      // candidate list
    uint64_t                idx;        // index within candidate list
    uint64_t                scnt;       // size of search space
    int64_t                 seed;       // current seed (next to be processed)
    bool                    isdone;
    std::atomic_bool      * abort;
};



#endif // SEARCHITEM_H
