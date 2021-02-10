#include "searchthread.h"
#include "mainwindow.h"

#include <QMessageBox>
#include <QEventLoop>

#include <x86intrin.h>

#define TSC_INTERRUPT_CNT ((uint64_t)1 << 30)

#define ITEM_SIZE 1024


extern MainWindow *gMainWindowInstance;


SearchThread::SearchThread(MainWindow *parent)
    : QThread()
    , parent(parent)
    , condvec()
    , itemgen()
    , pool()
    , activecnt()
    , abort()
    , reqstop()
    , recieved()
    , lastid()
{
    itemgen.abort = &abort;
}

bool SearchThread::set(int type, int threads, std::vector<int64_t>& slist64, int64_t sstart, int mc, const QVector<Condition>& cv)
{
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
        if (c.type >= F_BIOME && c.type <= F_BIOME_256_OTEMP)
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
        if (c.type == F_TEMPS)
        {
            int w = c.x2 - c.x1 + 1;
            int h = c.z2 - c.z1 + 1;
            if (w * h < c.count)
            {
                QMessageBox::warning(NULL, "Warning", QString::asprintf(
                        "Temperature category condition with ID [%02d] has too many restrictions (%d) for the area (%d x %d).",
                        c.save, c.count, w, h));
                return false;
            }
            if (c.count == 0)
            {
                QMessageBox::information(NULL, "Info", QString::asprintf("Temperature category condition with ID [%02d] specifies no restrictions.", c.save));
            }
        }
    }

    int itemsize = 1024;
    condvec = cv;
    itemgen.init(mc, condvec.data(), condvec.size(), slist64, itemsize, type, sstart);
    pool.setMaxThreadCount(threads);
    recieved.resize(2 * threads);
    lastid = itemgen.itemid;
    reqstop = false;
    abort = false;
    return true;
}


void SearchThread::run()
{
    pool.waitForDone();
    for (int idx = 0; idx < recieved.size(); idx++)
    {
        recieved[idx].valid = false;
        startNextItem();
    }
}

void SearchThread::debug()
{
    printf("lastid: %-2lu reci:", lastid);
    for (int i = 0; i < recieved.size(); i++)
        printf(" %d", (int)recieved[i].valid);
    printf("  (");
    for (int i = 0; i < recieved.size(); i++)
        printf(" %lu", i + lastid);
    printf(" )\n");
    fflush(stdout);
}

SearchItem *SearchThread::startNextItem()
{
    SearchItem *item = itemgen.requestItem();
    if (!item)
        return NULL;
    // call back here when done
    QObject::connect(item, &SearchItem::itemDone, this, &SearchThread::onItemDone, Qt::BlockingQueuedConnection);
    QObject::connect(item, &SearchItem::canceled, this, &SearchThread::onItemCanceled, Qt::QueuedConnection);
    // redirect results to mainwindow
    QObject::connect(item, &SearchItem::results, parent, &MainWindow::searchResultsAdd, Qt::BlockingQueuedConnection);
    ++activecnt;
    pool.start(item);
    return item;
}


void SearchThread::onItemDone(uint64_t itemid, int64_t seed, bool isdone)
{
    --activecnt;

    itemgen.isdone |= isdone;
    if (!itemgen.isdone && !reqstop && !abort)
    {
        if (itemid == lastid)
        {
            int64_t len = recieved.size();
            int idx;
            for (idx = 1; idx < len; idx++)
            {
                if (!recieved[idx].valid)
                    break;
            }

            lastid += idx;

            for (int i = idx; i < len; i++)
                recieved[i-idx] = recieved[i];
            for (int i = len-idx; i < len; i++)
                recieved[i].valid = false;

            for (int i = 0; i < idx; i++)
                startNextItem();

            uint64_t prog, end;
            itemgen.getProgress(&prog, &end);
            emit progress(prog, end, seed);
        }
        else
        {
            int idx = itemid - lastid;
            recieved[idx].valid = true;
            recieved[idx].seed = seed;
        }
    }

    if (activecnt == 0)
        emit searchFinish();
}

void SearchThread::onItemCanceled(uint64_t itemid)
{
    (void) itemid;
    --activecnt;
    if (activecnt == 0)
        emit searchFinish();
}

