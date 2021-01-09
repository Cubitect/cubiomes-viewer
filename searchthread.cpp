#include "searchthread.h"
#include <QMessageBox>

class FamilyBlock: public QRunnable
{
    // This class is a threadpool item for a range of upper 16-bit values to
    // look through, following a full 64-bit seed.
public:
    SearchThread *master;   // master thread for results
    int64_t sstart;         // starting seed
    int scnt;               // number of upper 16-bit combinations to check
    int mc;                 // mincraft version
    const Condition* cond;  // conditions to be met
    int ccnt;               // number of conditions

    FamilyBlock(SearchThread *t, int64_t sstart, int scnt, int mc,
                const Condition* cond, int ccnt)
        : master(t),sstart(sstart),scnt(scnt),mc(mc),cond(cond),ccnt(ccnt)
    {
        setAutoDelete(true);
    }

    void run()
    {
        LayerStack g;
        setupGenerator(&g, mc);
        StructPos spos[100] = {};
        int64_t seedbuf[scnt];

        int n = searchFamily(seedbuf, sstart, scnt, mc, &g, cond, ccnt, spos, &master->abortsearch);
        if (n && !master->abortsearch)
        {
            master->mutex.lock();
            for (int i = 0; i < n; i++)
            {
                master->seeds.push_back(seedbuf[i]);
                //printf("%ld\n", seedbuf[i]); fflush(stdout);
            }
            master->mutex.unlock();
        }
    }
};

// called from main GUI thread
bool SearchThread::set(int type, int64_t start48, int mc, const QVector<Condition>& cv)
{
    this->searchtype = type;
    this->sstart = start48;
    this->mc = mc;
    this->condvec = cv;
    char refbuf[100] = {};

    for (const Condition& c : cv)
    {
        if (c.save < 1 || c.save > 99)
        {
            QMessageBox::warning(NULL, "Warning", QString::asprintf("Condition with invalid ID [%02d].", c.save));
            return false;
        }
        if (c.relative && refbuf[c.relative] == 0)
        {
            QMessageBox::warning(NULL, "Warning", QString::asprintf(
                    "Condition with ID [%02d] has a broken reference position:\n"
                    "condition missing or out of order.", c.save));
            return false;
        }
        if (++refbuf[c.save] > 1)
        {
            QMessageBox::warning(NULL, "Warning", QString::asprintf("More than one condition with ID [%02d].", c.save));
            return false;
        }
        if (c.type != F_TEMP)
        {
            int w = c.x2 - c.x1;
            int h = c.z2 - c.z1;
            if (w < 0 || h < 0)
            {
                const FilterInfo& ft = g_filterinfo.list[c.type];
                QMessageBox::warning(NULL, "Warning", QString::asprintf(
                        "Condition with ID [%02d] does not specify a valid area:\nPosition: [%d, %d] Size: (%d, %d) Scale: x%d",
                        c.save, c.x1, c.z1, w, h, ft.step));
                return false;
            }
        }
        if (c.type >= F_BIOME && c.type <= F_BIOME_256_BIOME)
        {
            if ((c.exclb & (c.bfilter.riverToFind | c.bfilter.oceanToFind)) ||
                (c.exclm & c.bfilter.riverToFindM))
            {
                QMessageBox::warning(NULL, "Warning", QString::asprintf("Biome filter condition with ID [%02d] has contradicting flags for include and exclude.", c.save));
                return false;
            }
            if (c.count == 0)
            {
                QMessageBox::information(NULL, "Info", QString::asprintf("Biome filter condition with ID [%02d] specifies no biomes.", c.save));
            }
        }
    }

    return true;
}

// does the 48-bit seed meet the conditions c..ce?
static bool isCandidate(int64_t s48, int mc, const Condition *c, const Condition *ce, volatile bool *abort)
{
    StructPos spos[100] = {};
    for (; c != ce; c++)
        if (!testCond(spos, s48, c, mc, NULL, abort))
            return false;
    return true;
}


void SearchThread::run()
{
    abortsearch = false;
    elapsed.start();

    const Condition *cond = condvec.data();
    int64_t ccnt = condvec.size();

    CandidateList cl = getCandidates(mc, cond, ccnt, PRECOMPUTE48_BUFSIZ);
    int64_t ci;
    char *sp;
    int64_t s48 = sstart;

    if (cl.mem)
    {
        // a pre-computed list of candidates exists, skip forwards to starting point
        for (ci = 0, sp = cl.mem; ci < cl.bcnt; ci++, sp += cl.isiz)
            if ((s48 = *(int64_t*)sp) >= sstart)
                break;

        for (; ci < cl.bcnt && !abortsearch; ci++, sp += cl.isiz)
        {
            s48 = *(int64_t*)sp;
            if (isCandidate(s48, mc, cond, cond+ccnt, &abortsearch))
            {
                if (abortsearch)
                    break;
                if (runSearch48(s48, cond, ccnt) && stoponres)
                    break;
            }
        }
        if (ci == cl.bcnt)
            s48 = MASK48+1;

        free(cl.mem);
    }
    else
    {
        // go through all 48-bit seeds
        for (; s48 <= MASK48 && !abortsearch; s48++)
        {
            if (isCandidate(s48, mc, cond, cond+ccnt, &abortsearch))
            {
                if (abortsearch)
                    break;
                if (runSearch48(s48, cond, ccnt) && stoponres)
                    break;
            }
        }
    }

    emit finish(s48);
}

// search type options from combobox
enum { SEARCH_ALL64 = 0, SEARCH_INC48 = 1, SEARCH_CANDIT = 2 };

bool SearchThread::runSearch48(int64_t s48, const Condition* cond, int ccnt)
{
    // found a 48-bit seed candidate
    seeds.clear();

    if (searchtype == SEARCH_CANDIT)
    {
        seeds.push_back(s48);
    }
    else if (searchtype == SEARCH_INC48)
    {
        LayerStack g;
        setupGenerator(&g, mc);
        StructPos spos[100] = {};
        int64_t seedbuf[1];
        if (searchFamily(seedbuf, s48, 1, mc, &g, cond, ccnt, spos, &abortsearch))
            seeds.push_back(seedbuf[0]);
    }
    else
    {
        // distribute the search for the upper 16-bits onto a thread pool
        const int blocksize = 0x200;
        const int blockcnt = 0x10000 / blocksize;
        for (int i = 0; i < blockcnt; i++)
        {
            pool.start(new FamilyBlock(this, s48, blocksize, mc, cond, ccnt));
            s48 += (int64_t)blocksize << 48;
        }
        pool.waitForDone();
    }

    if (!abortsearch)
    {
        if (elapsed.elapsed() > 10)
        {
            emit baseDone(s48);
            elapsed.start();
        }
    }
    if (!seeds.empty())
    {
        emit results(seeds, false);
        return true;
    }
    return false;
}
