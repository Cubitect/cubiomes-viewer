#include "searchitem.h"


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
    int mc, const Condition *cond, int ccnt,
    const std::vector<int64_t>& seedlist,
    int itemsize, int searchtype, int64_t sstart)
{
    this->searchtype = searchtype;
    this->mc = mc;
    this->cond = cond;
    this->ccnt = ccnt;
    this->itemid = 0;
    this->itemsiz = itemsize;
    this->slist = seedlist;
    this->idx = 0;
    this->scnt = ~(uint64_t)0;
    this->seed = sstart;
    this->isdone = false;

    if (slist.empty() && searchtype != SEARCH_LIST)
        getCandidates(slist, mc, cond, ccnt, PRECOMPUTE48_BUFSIZ);

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

                /// === search for next candidate ===
                for (; low <= MASK48; low++)
                {
                    if (isCandidate(low, mc, cond, cond+ccnt, abort))
                        break;
                }
                if (low > MASK48)
                    isdone = true;
            }
            seed = (high << 48) | low;
        }
    }

    return item;
}

