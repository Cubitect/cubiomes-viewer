#include "searchitem.h"
#include "seedtables.h"

#include <QMessageBox>
#include <QStandardPaths>
#include <QApplication>
#include <QElapsedTimer>

SearchItem::~SearchItem()
{
    if (searchtype >= 0)
        emit canceled(itemid);
}


void SearchItem::run()
{
    QVector<uint64_t> matches;
    WorldGen gen;
    Pos cpos[100];
    Pos origin = {0,0};
    gen.init(mc, large);

    if (searchtype == SEARCH_LIST)
    {   // seed = slist[..]
        uint64_t ie = idx+scnt < len ? idx+scnt : len;
        for (uint64_t i = idx; i < ie; i++)
        {
            seed = slist[i];
            gen.setSeed(seed);
            if (testSeedAt(origin, cpos, pcvec, PASS_FULL_64, &gen, abort)
                == COND_OK
            )
            {
                matches.push_back(seed);
            }
        }
        isdone = (ie == len);
    }

    if (searchtype == SEARCH_INC)
    {
        if (slist)
        {   // seed = (high << 48) | slist[..]
            uint64_t high = (sstart >> 48) & 0xffff;
            uint64_t lowidx = idx;

            for (int i = 0; i < scnt; i++)
            {
                seed = (high << 48) | slist[lowidx];

                gen.setSeed(seed);
                if (testSeedAt(origin, cpos, pcvec, PASS_FULL_64, &gen, abort)
                    == COND_OK
                )
                {
                    matches.push_back(seed);
                }

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
                gen.setSeed(seed);
                if (testSeedAt(origin, cpos, pcvec, PASS_FULL_64, &gen, abort)
                    == COND_OK
                )
                {
                    matches.push_back(seed);
                }

                if (seed == ~(uint64_t)0)
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

            uint64_t high = (sstart >> 48) & 0xffff;
            uint64_t low;
            if (slist)
                low = slist[idx];
            else
                low = sstart & MASK48;

            gen.setSeed(low);
            if (testSeedAt(origin, cpos, pcvec, PASS_FULL_48, &gen, abort)
                == COND_FAILED
            )
            {
                break;
            }

            for (int i = 0; i < scnt; i++)
            {
                seed = (high << 48) | low;

                gen.setSeed(seed);
                if (testSeedAt(origin, cpos, pcvec, PASS_FULL_64, &gen, abort)
                    == COND_OK
                )
                {
                    matches.push_back(seed);
                }

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
    QObject *mainwin, WorldInfo wi,
    const SearchConfig& sc, const Gen48Settings& gen48, const Config& config,
    const std::vector<uint64_t>& slist, const QVector<Condition>& cv)
{
    this->mainwin = mainwin;
    this->searchtype = sc.searchtype;
    this->mc = wi.mc;
    this->large = wi.large;
    this->condvec = cv;
    this->itemid = 0;
    this->itemsiz = config.seedsPerItem;
    this->slist = slist;
    this->gen48 = gen48;
    this->idx = 0;
    this->scnt = ~(uint64_t)0;
    this->seed = sc.startseed;
    this->idxmin = 0;
    this->smin = sc.smin;
    this->smax = sc.smax;
    this->isdone = false;
}


static int check(uint64_t s48, void *data)
{
    (void) data;
    const StructureConfig sconf = {};
    return isQuadBaseFeature24(sconf, s48, 7+1, 7+1, 9+1) != 0;
}

static void genQHBases(QObject *qtobj, int qual, uint64_t salt, std::vector<uint64_t>& list48)
{
    const char *lbstr = NULL;
    const uint64_t *lbset = NULL;
    uint64_t lbcnt = 0;
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (path.isEmpty())
        path = "protobases";

    switch (qual)
    {
    case IDEAL_SALTED:
    case IDEAL:
        lbstr = "ideal";
        lbset = low20QuadIdeal;
        lbcnt = sizeof(low20QuadIdeal) / sizeof(uint64_t);
        break;
    case CLASSIC:
        lbstr = "cassic";
        lbset = low20QuadClassic;
        lbcnt = sizeof(low20QuadClassic) / sizeof(uint64_t);
        break;
    case NORMAL:
        lbstr = "normal";
        lbset = low20QuadHutNormal;
        lbcnt = sizeof(low20QuadHutNormal) / sizeof(uint64_t);
        break;
    case BARELY:
        lbstr = "barely";
        lbset = low20QuadHutBarely;
        lbcnt = sizeof(low20QuadHutBarely) / sizeof(uint64_t);
        break;
    default:
        return;
    }

    path += QString("/quad_") + lbstr + ".txt";
    QByteArray fnam = path.toLatin1();
    uint64_t *qb = NULL;
    uint64_t qn = 0;

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
        for (uint64_t i = 0; i < qn; i++)
            list48[i] = qb[i] - salt;
        free(qb);
    }
}


bool applyTranspose(std::vector<uint64_t>& slist,
                    const Gen48Settings& gen48, uint64_t bufmax)
{
    std::vector<uint64_t> list48;

    int x = gen48.x1;
    int z = gen48.z1;
    int w = gen48.x2 - x + 1;
    int h = gen48.z2 - z + 1;

    // does the set of candidates for this condition fit in memory?
    if ((uint64_t)slist.size() * sizeof(int64_t) * w*h >= bufmax)
    {
        slist.clear();
        return false;
    }

    try {
        list48.resize(slist.size() * w*h);
    } catch (...) {
        slist.clear();
        return false;
    }

    uint64_t *p = list48.data();
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            for (uint64_t b : slist)
                *p++ = moveStructure(b, x+i, z+j);

    std::sort(list48.begin(), list48.end());
    auto last = std::unique(list48.begin(), list48.end());
    list48.erase(last, list48.end());
    slist.swap(list48);
    return !slist.empty();
}

void SearchItemGenerator::presearch()
{
    uint64_t sstart = seed;

    if (searchtype != SEARCH_LIST)
    {
        if (gen48.mode == GEN48_QH)
        {
            uint64_t salt = 0;
            if (gen48.qual == IDEAL_SALTED)
                salt = gen48.salt;
            else
            {
                StructureConfig sconf;
                getStructureConfig_override(Swamp_Hut, mc, &sconf);
                salt = sconf.salt;
            }
            slist.clear();
            genQHBases(mainwin, gen48.qual, salt, slist);
        }
        else if (gen48.mode == GEN48_QM)
        {
            StructureConfig sconf;
            getStructureConfig_override(Monument, mc, &sconf);
            const uint64_t *qb = g_qm_90;
            uint64_t qn = sizeof(g_qm_90) / sizeof(uint64_t);
            slist.clear();
            slist.reserve(qn);
            for (uint64_t i = 0; i < qn; i++)
                if (qmonumentQual(qb[i]) >= gen48.qmarea)
                    slist.push_back((qb[i] - sconf.salt) & MASK48);
        }
        else if (gen48.mode == GEN48_LIST)
        {
            if (gen48.listsalt)
            {
                for (uint64_t& rs : slist)
                    rs += gen48.listsalt;
            }
        }

        if (!slist.empty())
            applyTranspose(slist, gen48, PRECOMPUTE48_BUFSIZ);
    }

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
            seed = sstart;
            if (seed < smin)
                seed = smin;
            scnt = 0x10000 * slist.size();
            uint64_t high = (seed >> 48) & 0xffff;
            for (idx = 0; idx < slist.size(); idx++)
                if (slist[idx] >= (seed & MASK48))
                    break;
            if (idx == slist.size())
            {
                if (high++ >= (smax >> 48))
                    isdone = true;
                idx = 0;
            }
            seed = (high << 48) | slist[idx];

            for (idxmin = 0; idxmin < slist.size(); idxmin++)
                if (slist[idxmin] >= (smin & MASK48))
                    break;
            for (scnt = 0; scnt < slist.size(); scnt++)
                if (slist[scnt] >= (smax & MASK48))
                    break;
            high = ((smax >> 48) - (smin >> 48)) & 0xffff;
            scnt += high * slist.size() - idxmin;
        }
        else
        {
            scnt = smax - smin;
            seed = sstart;
            if (seed < smin)
                seed = smin;
        }
        if (seed > smax)
            isdone = true;
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
        {
            uint64_t h = ((seed >> 48) - (smin >> 48)) & 0xffff;
            *prog = h * slist.size() + idx - idxmin;
        }
        else
        {
            *prog = seed - smin;
        }
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

SearchItem *SearchItemGenerator::requestItem()
{
    if (isdone)
        return NULL;

    SearchItem *item = new SearchItem();

    item->searchtype = searchtype;
    item->mc        = mc;
    item->large     = large;
    item->pcvec     = &condvec;
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
            uint64_t high = (seed >> 48) & 0xffff;
            idx += itemsiz;
            high += idx / slist.size();
            idx %= slist.size();
            seed = (high << 48) | slist[idx];
            if (high > (smax >> 48))
                isdone = true;
        }
        else
        {
            // seed += itemsize; with overflow detection
            uint64_t s = seed + itemsiz;
            if (s < seed)
                isdone = true; // overflow
            seed = s;
        }
        if (seed > smax)
            isdone = true;
    }

    if (searchtype == SEARCH_BLOCKS)
    {
        if (!slist.empty())
        {
            uint64_t high = (seed >> 48) & 0xffff;
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
            WorldGen gen;
            Pos cpos[100];
            Pos origin = {0,0};
            gen.init(mc, large);

            QElapsedTimer timer;

            uint64_t high = (seed >> 48) & 0xffff;
            uint64_t low = seed & MASK48;
            high += itemsiz;
            if (high >= 0x10000)
            {
                item->scnt -= 0x10000 - high;
                high = 0;
                low++;

                timer.start();

                for (; low <= MASK48; low++)
                {
                    gen.setSeed(low);
                    if (testSeedAt(origin, cpos, &condvec, PASS_FAST_48, &gen,
                        abort) != COND_FAILED)
                    {
                        break;
                    }

                    if (timer.elapsed() > 500)
                    {
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

