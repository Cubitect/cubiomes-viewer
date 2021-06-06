#include "searchitem.h"
#include "seedtables.h"

#include <QMessageBox>
#include <QStandardPaths>
#include <QApplication>

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif


SearchItem::~SearchItem()
{
    if (searchtype >= 0)
        emit canceled(itemid);
}

void SearchItem::run()
{
    LayerStack g;
    setupGenerator(&g, mc);
    StructPos spos[100] = {};
    QVector<int64_t> matches;

    if (searchtype == SEARCH_LIST)
    {   // seed = slist[..]
        int64_t ie = idx+scnt < len ? idx+scnt : len;
        for (int64_t i = idx; i < ie; i++)
        {
            seed = slist[i];
            if (testSeed(spos, seed, &g, true))
                matches.push_back(seed);
        }
        isdone = (ie == len);
    }

    if (searchtype == SEARCH_INC)
    {
        if (slist)
        {   // seed = (high << 48) | slist[..]
            int64_t high = (sstart >> 48) & 0xffff;
            int64_t lowidx = idx;

            for (int i = 0; i < scnt; i++)
            {
                seed = (high << 48) | slist[lowidx];

                if (testSeed(spos, seed, &g, true))
                    matches.push_back(seed);

                if (++lowidx >= len)
                {
                    lowidx = 0;
                    if (++high >= 0x10000)
                    {
                        isdone = true;
                        break;
                    }
                }
            }
        }
        else
        {   // seed++
            seed = sstart;
            for (int i = 0; i < scnt; i++)
            {
                if (testSeed(spos, seed, &g, true))
                    matches.push_back(seed);

                if (seed == ~(int64_t)0)
                {
                    isdone = true;
                    break;
                }
                seed++;
            }
        }
    }

    if (searchtype == SEARCH_BLOCKS)
    {   // seed = ([..] << 48) | low
        do
        {
            if (slist && idx >= len)
            {
                isdone = true;
                break;
            }

            int64_t high = (sstart >> 48) & 0xffff;
            int64_t low;
            if (slist)
                low = slist[idx];
            else
                low = sstart & MASK48;

            if (!testSeed(spos, low, NULL, true))
            {
                // warning: block search should only cover candidates
                break;
            }

            for (int i = 0; i < scnt; i++)
            {
                seed = (high << 48) | low;

                if (testSeed(spos, seed, &g, false))
                    matches.push_back(seed);

                if (++high >= 0x10000)
                    break;
            }
        }
        while (0);
    }

    if (!matches.empty())
    {
        emit results(matches, false);
    }
    emit itemDone(itemid, seed, isdone);
    searchtype = -1;
}


void SearchItemGenerator::init(
    QObject *mainwin, int mc, const Condition *cond, int ccnt,
    Gen48Settings gen48, const std::vector<int64_t>& seedlist,
    int itemsize, int searchtype, int64_t sstart)
{
    this->mainwin = mainwin;
    this->searchtype = searchtype;
    this->mc = mc;
    this->cond = cond;
    this->ccnt = ccnt;
    this->itemid = 0;
    this->itemsiz = itemsize;
    this->slist = seedlist;
    this->gen48 = gen48;
    this->idx = 0;
    this->scnt = ~(uint64_t)0;
    this->seed = sstart;
    this->isdone = false;
}


static int check(int64_t s48, void *data)
{
    (void) data;
    const StructureConfig sconf = {};
    return isQuadBaseFeature24(sconf, s48, 7+1, 7+1, 9+1) != 0;
}

static void genQHBases(QObject *qtobj, int qual, int64_t salt, std::vector<int64_t>& list48)
{
    const char *lbstr = NULL;
    const int64_t *lbset = NULL;
    int64_t lbcnt = 0;
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (path.isEmpty())
        path = "protobases";

    switch (qual)
    {
    case IDEAL_SALTED:
    case IDEAL:
        lbstr = "ideal";
        lbset = low20QuadIdeal;
        lbcnt = sizeof(low20QuadIdeal) / sizeof(int64_t);
        break;
    case CLASSIC:
        lbstr = "cassic";
        lbset = low20QuadClassic;
        lbcnt = sizeof(low20QuadClassic) / sizeof(int64_t);
        break;
    case NORMAL:
        lbstr = "normal";
        lbset = low20QuadHutNormal;
        lbcnt = sizeof(low20QuadHutNormal) / sizeof(int64_t);
        break;
    case BARELY:
        lbstr = "barely";
        lbset = low20QuadHutBarely;
        lbcnt = sizeof(low20QuadHutBarely) / sizeof(int64_t);
        break;
    default:
        return;
    }

    path += QString("/quad_") + lbstr + ".txt";
    QByteArray fnam = path.toLatin1();
    int64_t *qb = NULL;
    int64_t qn = 0;

    if ((qb = loadSavedSeeds(fnam.data(), &qn)) == NULL)
    {
        printf("Writing quad-protobases to: %s\n", fnam.data());
        fflush(stdout);

        QMetaObject::invokeMethod(qtobj, "openProtobaseMsg", Qt::QueuedConnection, Q_ARG(QString, path));

        int threads = QThread::idealThreadCount();
        int err = searchAll48(&qb, &qn, fnam.data(), threads, lbset, lbcnt, 20, check, NULL);

        if (err)
        {
            QMetaObject::invokeMethod(
                    qtobj, "warning", Qt::BlockingQueuedConnection,
                    Q_ARG(QString, QString("Warning")),
                    Q_ARG(QString, QString("Failed to generate protobases.")));
            return;
        }
        else
        {
            QMetaObject::invokeMethod(qtobj, "closeProtobaseMsg", Qt::BlockingQueuedConnection);
        }
    }
    else
    {
        //printf("Loaded quad-protobases from: %s\n", fnam.data());
        //fflush(stdout);
    }

    if (qb)
    {
        // convert protobases to proper bases by subtracting the salt
        list48.resize(qn);
        for (int64_t i = 0; i < qn; i++)
            list48[i] = qb[i] - salt;
        free(qb);
    }
}

// Produces a list of seed bases from precomputed lists, provided all candidates fit into a buffer.
bool getQuadCandidates(std::vector<int64_t>& list48, QObject *qtobj, Gen48Settings gen48, int mc, int64_t bufmax)
{
    std::vector<int64_t> qlist;
    list48.clear();

    if (gen48.mode == GEN48_QH)
    {
        int64_t salt = 0;
        if (gen48.qual == IDEAL_SALTED)
            salt = gen48.salt;
        else
            salt = (mc <= MC_1_12 ? SWAMP_HUT_CONFIG_112.salt : SWAMP_HUT_CONFIG.salt);
        genQHBases(qtobj, gen48.qual, salt, qlist);
    }
    else if (gen48.mode == GEN48_QM)
    {
        const int64_t *qb = g_qm_90;
        int64_t qn = sizeof(g_qm_90) / sizeof(int64_t);
        qlist.reserve(qn);
        for (int64_t i = 0; i < qn; i++)
            if (qmonumentQual(qb[i]) >= gen48.qmarea)
                qlist.push_back(qb[i]);
    }

    if (qlist.empty())
        return false;

    int x = gen48.x1;
    int z = gen48.z1;
    int w = gen48.x2 - x + 1;
    int h = gen48.z2 - z + 1;

    // does the set of candidates for this condition fit in memory?
    if ((int64_t)qlist.size() * (int64_t)sizeof(int64_t) * w*h >= bufmax)
        return false;

    try {
        list48.resize(qlist.size() * w*h);
    } catch (...) {
        return false;
    }

    int64_t *p = list48.data();
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            for (int64_t b : qlist)
                *p++ = moveStructure(b, x+i, z+j);

    std::sort(list48.begin(), list48.end());
    auto last = std::unique(list48.begin(), list48.end());
    list48.erase(last, list48.end());

    return !list48.empty();
}


void SearchItemGenerator::presearch()
{
    int64_t sstart = seed;

    if (slist.empty() && searchtype != SEARCH_LIST)
        getQuadCandidates(slist, mainwin, gen48, mc, PRECOMPUTE48_BUFSIZ);

    if (searchtype == SEARCH_LIST && !slist.empty())
    {
        scnt = slist.size();
        for (idx = 0; idx < scnt; idx++)
            if (slist[idx] == sstart)
                break;
        if (idx == scnt)
            idx = 0;
        seed = slist[idx];
    }

    if (searchtype == SEARCH_INC)
    {
        if (!slist.empty())
        {
            scnt = 0x10000 * slist.size();
            int64_t high = (sstart >> 48) & 0xffff;
            for (idx = 0; idx < slist.size(); idx++)
                if (slist[idx] >= (sstart & MASK48))
                    break;
            if (idx == slist.size())
            {
                if (++high >= 0x10000)
                    isdone = true;
                idx = 0;
            }
            seed = (high << 48) | slist[idx];
        }
        else
        {
            scnt = ~(uint64_t)0;
            seed = sstart;
        }
    }

    if (searchtype == SEARCH_BLOCKS)
    {
        if (!slist.empty())
        {
            scnt = 0x10000 * slist.size();
            for (idx = 0; idx < slist.size(); idx++)
                if (slist[idx] >= (sstart & MASK48))
                    break;
            if (idx == slist.size())
                isdone = true;
            else
                seed = (sstart & ~MASK48) | slist[idx];
        }
        else
        {
            scnt = ~(uint64_t)0;
            seed = sstart;
        }
    }
}

void SearchItemGenerator::getProgress(uint64_t *prog, uint64_t *end)
{
    if (searchtype == SEARCH_LIST)
    {
        *prog = idx;
    }

    if (searchtype == SEARCH_INC)
    {
        if (!slist.empty())
            *prog = ((seed >> 48) & 0xffff) * slist.size() + idx;
        else
            *prog = (uint64_t) seed;
    }

    if (searchtype == SEARCH_BLOCKS)
    {
        if (!slist.empty())
            *prog = idx * 0x10000;
        else
            *prog = (uint64_t)(seed & MASK48) * 0x10000;
    }

    *end = scnt;
}


// does the 48-bit seed meet the conditions c..ce?
static bool isCandidate(int64_t s48, int mc, const Condition *c, const Condition *ce, std::atomic_bool *abort)
{
    StructPos spos[100] = {};
    for (; c != ce; c++)
        if (!testCond(spos, s48, c, mc, NULL, abort))
            return false;
    return true;
}

SearchItem *SearchItemGenerator::requestItem()
{
    if (isdone)
        return NULL;

    SearchItem *item = new SearchItem();

    item->searchtype = searchtype;
    item->mc        = mc;
    item->cond      = cond;
    item->ccnt      = ccnt;
    item->itemid    = itemid++;
    item->slist     = slist.empty() ? NULL : slist.data();
    item->len       = slist.size();
    item->idx       = idx;
    item->sstart    = seed;
    item->scnt      = itemsiz;
    item->seed      = seed;
    item->isdone    = isdone;
    item->abort     = abort;

    if (searchtype == SEARCH_LIST)
    {
        if (idx + itemsiz > scnt)
            item->scnt = scnt - idx;
        idx += itemsiz;
    }

    if (searchtype == SEARCH_INC)
    {
        if (!slist.empty())
        {
            int64_t high = (seed >> 48) & 0xffff;
            idx += itemsiz;
            high += idx / slist.size();
            idx %= slist.size();
            seed = (high << 48) | slist[idx];
            if (high >= 0x10000)
                isdone = true;
        }
        else
        {
            unsigned long long int s;
            if (__builtin_uaddll_overflow(seed, itemsiz, &s))
                isdone = true;
            seed = (int64_t)s;
        }
    }

    if (searchtype == SEARCH_BLOCKS)
    {
        if (!slist.empty())
        {
            int64_t high = (seed >> 48) & 0xffff;
            high += itemsiz;
            if (high >= 0x10000)
            {
                high = 0;
                idx++;
            }
            if (idx >= slist.size())
                isdone = true;
            else
                seed = (high << 48) | slist[idx];
        }
        else
        {
            int64_t high = (seed >> 48) & 0xffff;
            int64_t low = seed & MASK48;
            high += itemsiz;
            if (high >= 0x10000)
            {
                item->scnt -= 0x10000 - high;
                high = 0;
                low++;

                unsigned long long ts = __rdtsc() + (1ULL << 27);

                /// === search for next candidate ===
                for (; low <= MASK48; low++)
                {
                    if (isCandidate(low, mc, cond, cond+ccnt, abort))
                        break;
                    if (__rdtsc() > ts)
                    {
                        ts = __rdtsc() + (1ULL << 27);
                        high = 0xffff;
                        break;
                    }
                }
                if (low > MASK48)
                    isdone = true;
            }
            seed = (high << 48) | low;
        }
    }

    return item;
}

