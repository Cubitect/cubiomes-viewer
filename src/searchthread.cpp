#include "searchthread.h"
#include "formsearchcontrol.h"
#include "util.h"
#include "seedtables.h"
#include "message.h"
#include "aboutdialog.h"

#include <QEventLoop>
#include <QApplication>
#include <QStandardPaths>
#include <QElapsedTimer>
#include <QMutex>
#include <QVector>
#include <QDateTime>


void Session::writeHeader(QTextStream& stream)
{
    stream << "#Version:  " << VERS_MAJOR << "." << VERS_MINOR << "." << VERS_PATCH << "\n";
    stream << "#Time:     " << QDateTime::currentDateTime().toString() << "\n";
    // MC version of the session should take priority over the one in the settings
    wi.write(stream);

    sc.write(stream);
    gen48.write(stream);

    for (Condition &c : cv)
        stream << "#Cond: " << c.toHex() << "\n";
}

bool Session::save(QWidget *widget, QString fnam, bool quiet)
{
    QFile file(fnam);

    if (!file.open(QIODevice::WriteOnly))
    {
        if (!quiet)
            warn(widget, QApplication::tr("Failed to open file:\n\"%1\"").arg(fnam));
        return false;
    }

    QTextStream stream(&file);
    writeHeader(stream);

    for (uint64_t s : slist)
        stream << QString::asprintf("%" PRId64 "\n", (int64_t)s);

    return true;
}

bool Session::load(QWidget *widget, QString fnam, bool quiet)
{
    QFile file(fnam);

    if (!file.open(QIODevice::ReadOnly))
    {
        if (!quiet)
            warn(widget, QApplication::tr("Failed to open session file:\n\"%1\"").arg(fnam));
        return false;
    }

    int major = 0, minor = 0, patch = 0;
    QTextStream stream(&file);
    QString line;
    line = stream.readLine();
    int lno = 1;

    if (sscanf(line.toLocal8Bit().data(), "#Version: %d.%d.%d", &major, &minor, &patch) != 3)
    {
        if (quiet)
            return false;
        QString msg = QApplication::tr("File does not look like a session file.\n"
                                       "Progress may be incomplete or broken.\n\n"
                                       "Continue anyway?");
        int button = warn(widget, msg, QMessageBox::Abort|QMessageBox::Yes);
        if (button == QMessageBox::Abort)
            return false;
    }
    else if (cmpVers(major, minor, patch) > 0)
    {
        if (quiet)
            return false;
        QString msg = QApplication::tr("Session file was created with a newer version.\n"
                                       "Progress may be incomplete or broken.\n\n"
                                       "Continue loading progress anyway?");
        int button = warn(widget, msg, QMessageBox::Abort|QMessageBox::Yes);
        if (button == QMessageBox::Abort)
            return false;
    }

    while (stream.status() == QTextStream::Ok && !stream.atEnd())
    {
        lno++;
        line = stream.readLine();

        if (line.isEmpty()) continue;
        if (line.startsWith("#Time:")) continue;
        if (line.startsWith("#Title:")) continue;
        if (line.startsWith("#Desc:")) continue;
        if (sc.read(line)) continue;
        if (gen48.read(line)) continue;
        if (wi.read(line)) continue;

        if (line.startsWith("#Cond:"))
        {   // Conditions
            Condition c;
            if (c.readHex(line.mid(6).trimmed()))
            {
                cv.push_back(c);
            }
            else
            {
                if (quiet)
                    return false;
                QString msg = QApplication::tr("Condition [%1] at line %2 is not supported.\n\n"
                                               "Continue anyway?");
                int button = warn(widget, msg.arg(c.save).arg(lno), QMessageBox::Abort|QMessageBox::Yes);
                if (button == QMessageBox::Abort)
                    return false;
            }
        }
        else
        {   // Seeds
            QByteArray ba = line.toLocal8Bit();
            const char *p = ba.data();
            uint64_t s;
            if (sscanf(p, "%" PRId64, (int64_t*)&s) == 1)
            {
                slist.push_back(s);
            }
            else
            {
                if (quiet)
                    return false;
                QString msg = QApplication::tr("Failed to parse line %1 of file:\n%2\n\n"
                                               "Continue anyway?");
                int button = warn(widget, msg.arg(lno).arg(line), QMessageBox::Abort|QMessageBox::Yes);
                if (button == QMessageBox::Abort)
                    return false;
            }
        }
    }
    return true;
}


SearchMaster::SearchMaster(QWidget *parent)
    : QThread(parent)
    , mutex()
    , abort()
    , proghist()
    , progtimer()
    , itemtimer()
    , count()
    , env()
    , searchtype()
    , mc()
    , large()
    , condtree()
    , itemsize()
    , threadcnt()
    , gen48()
    , slist()
    , idx()
    , scnt()
    , prog()
    , seed()
    , smin()
    , smax()
    , isdone()
{
}

SearchMaster::~SearchMaster()
{
    stop();
}

bool SearchMaster::set(QWidget *widget, const Session& s)
{
    char refbuf[100] = {};
    char disabled[100] = {};

    for (const Condition& c : s.cv)
        if (c.meta & Condition::DISABLED)
            disabled[c.save] = 1;

    for (const Condition& c : s.cv)
    {
        char cid[8];
        snprintf(cid, sizeof(cid), "[%02d]", c.save);
        if (c.save < 1 || c.save > 99)
        {
            warn(widget, tr("Condition with invalid ID %1.").arg(cid));
            return false;
        }
        if (c.type < 0 || c.type >= FILTER_MAX)
        {
            warn(widget, tr("Encountered invalid filter type %1 in condition ID %2.").arg(c.type).arg(cid));
            return false;
        }
        if (disabled[c.save])
            continue;

        const FilterInfo& finfo = g_filterinfo.list[c.type];

        if (c.relative && refbuf[c.relative] == 0)
        {
            warn(widget, tr("Condition with ID %1 has a broken reference position:\n"
                            "condition missing or out of order.").arg(cid));
            return false;
        }
        if (++refbuf[c.save] > 1)
        {
            warn(widget, tr("More than one condition with ID %1.").arg(cid));
            return false;
        }
        if (c.relative && disabled[c.relative])
        {
            int button = info(widget, tr("Condition %1 has been indirectly disabled by reference.").arg(cid),
                              QMessageBox::Abort|QMessageBox::Ignore);
            if (button == QMessageBox::Abort)
                return false;
        }
        if (s.wi.mc < finfo.mcmin)
        {
            const char *mcs = mc2str(finfo.mcmin);
            warn(widget, tr("Condition %1 requires a minimum Minecraft version of %2.").arg(cid, mcs));
            return false;
        }
        if (s.wi.mc > finfo.mcmax)
        {
            const char *mcs = mc2str(finfo.mcmax);
            warn(widget, tr("Condition %1 not available for Minecraft versions above %2.").arg(cid, mcs));
            return false;
        }
        if (finfo.cat == CAT_BIOMES &&
            c.type != F_BIOME_CENTER &&
            c.type != F_BIOME_CENTER_256 &&
            c.type != F_TEMPS &&
            c.type != F_CLIMATE_NOISE &&
            c.type != F_CLIMATE_MINMAX)
        {
            uint64_t b = c.biomeToFind;
            uint64_t m = c.biomeToFindM;
            if ((c.biomeToExcl & b) || (c.biomeToExclM & m))
            {
                warn(widget, tr("Biome condition with ID %1 has contradicting flags for include and exclude.").arg(cid));
                return false;
            }
            if ((b | m | c.biomeToExcl | c.biomeToExclM) == 0)
            {
                int button = info(widget, tr("Biome condition with ID %1 specifies no biomes.").arg(cid),
                                  QMessageBox::Abort|QMessageBox::Ignore);
                if (button == QMessageBox::Abort)
                    return false;
            }

            int layerId = finfo.layer;
            if (layerId == 0 && s.wi.mc <= MC_1_17)
            {
                Generator tmp;
                setupGenerator(&tmp, s.wi.mc, 0);
                const Layer *l = getLayerForScale(&tmp, finfo.step);
                if (l)
                    layerId = l - tmp.ls.layers;
            }
            uint64_t ab, am;
            uint32_t flags = 0;
            getAvailableBiomes(&ab, &am, layerId, s.wi.mc, flags);
            b ^= (ab & b);
            m ^= (am & m);
            if (b || m)
            {
                int cnt = __builtin_popcountll(b) + __builtin_popcountll(m);
                QString msg = tr("Biome condition with ID %1 includes %n biome(s) "
                                 "that do not generate in MC %2.", "", cnt);
                warn(widget, msg.arg(cid, mc2str(s.wi.mc)));
                return false;
            }
        }
        if (c.type == F_TEMPS)
        {
            int w = c.x2 - c.x1 + 1;
            int h = c.z2 - c.z1 + 1;
            if (c.count > w * h)
            {
                QString msg = tr("Temperature category condition with ID %1 has too "
                                 "many restrictions (%2) for the area (%3 x %4).");
                warn(widget, msg.arg(cid).arg(c.count).arg(w).arg(h));
                return false;
            }
        }
        if (finfo.cat == CAT_STRUCT)
        {
            if (c.count >= 128)
            {
                warn(widget, tr("Structure condition %1 checks for too many instances (>= 128).").arg(cid));
                return false;
            }
        }
        if (c.skipref && c.rmax == 0 && c.x1 == 0 && c.x2 == 0 && c.z1 == 0 && c.z2 == 0)
        {
            warn(widget, tr("Condition %1 ignores its only location of size 1.").arg(cid));
            return false;
        }
    }

    QString err = condtree.set(s.cv, s.wi.mc);
    if (err.isEmpty())
    {
        err = env.init(s.wi.mc, s.wi.large, &condtree);
    }
    if (!err.isEmpty())
    {
        warn(widget, tr("Failed to setup search environment:\n%1").arg(err));
        return false;
    }

    this->searchtype = s.sc.searchtype;
    this->mc = s.wi.mc;
    this->large = s.wi.large;
    this->itemsize = 1;
    this->threadcnt = s.sc.threads;
    this->slist = s.slist;
    this->gen48 = s.gen48;
    this->idx = 0;
    this->scnt = ~(uint64_t)0;
    this->prog = 0;
    this->seed = s.sc.startseed;
    this->smin = s.sc.smin;
    this->smax = s.sc.smax;
    this->isdone = false;
    this->abort = false;
    return true;
}


static int check(uint64_t s48, void *data)
{
    (void) data;
    static const StructureConfig sconf = {0,0,0,0,0,0};
    return isQuadBaseFeature24(sconf, s48, 7+1, 7+1, 9+1) != 0;
}

static void genQHBases(QObject *qtobj, int qual, uint64_t salt, std::vector<uint64_t>& list48, std::atomic_bool *stop)
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
    QByteArray fnam = path.toLocal8Bit();
    uint64_t *qb = NULL;
    uint64_t qn = 0;

    if ((qb = loadSavedSeeds(fnam.data(), &qn)) == NULL)
    {
        printf("[INFO]: Writing quad-protobases to: %s\n", fnam.data());
        fflush(stdout);

        if (qtobj)
            QMetaObject::invokeMethod(qtobj, "openProtobaseMsg", Qt::QueuedConnection, Q_ARG(QString, path));

        int threads = QThread::idealThreadCount();
        int err = searchAll48(&qb, &qn, fnam.data(), threads, lbset, lbcnt, 20, check, NULL, (volatile char*) stop);

        if (err)
        {
            printf("[WARN]: Failed to generate protobases.\n");
            if (qtobj && !*stop)
            {
                QMetaObject::invokeMethod(
                        qtobj, "warning", Qt::BlockingQueuedConnection,
                        Q_ARG(QString, SearchMaster::tr("Failed to generate protobases.")));
            }
            return;
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

static bool applyTranspose(std::vector<uint64_t>& slist,
        const Gen48Config& gen48, uint64_t bufmax)
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

void SearchMaster::presearch(QObject *qtobj)
{
    uint64_t sstart = seed;

    if (gen48.mode == GEN48_AUTO)
    {   // resolve automatic mode
        for (const Condition& c : qAsConst(condtree.condvec))
        {
            if (c.type >= F_QH_IDEAL && c.type <= F_QH_BARELY)
            {
                gen48.mode = GEN48_QH;
                break;
            }
            else if (c.type >= F_QM_95 && c.type <= F_QM_90)
            {
                gen48.mode = GEN48_QM;
                break;
            }
        }
    }

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
            genQHBases(qtobj, gen48.qual, salt, slist, &abort);
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

    if (searchtype == SEARCH_LIST)
    {
        if (!slist.empty())
        {   // 64-bit seed list
            scnt = slist.size();
            for (idx = 0; idx < scnt; idx++)
                if (slist[idx] == sstart)
                    break;
            if (idx == scnt)
                idx = 0;
            seed = slist[idx];
            smax = slist.back();
            prog = idx;
        }
        else
        {   // slist should not be empty for a meaningful list search
            scnt = smax = ~(uint64_t)0;
            prog = seed = sstart;
            idx = 0;
        }
    }

    if (searchtype == SEARCH_48ONLY)
    {
        if (!slist.empty())
        {   // 48-bit seed list
            scnt = slist.size();
            for (idx = 0; idx < scnt; idx++)
                if (slist[idx] == sstart)
                    break;
            if (idx == scnt)
                idx = 0;
            seed = slist[idx];
            smax = slist.back();
            prog = idx;
        }
        else
        {
            prog = seed = sstart;
            scnt = smax = MASK48;
            if (seed > smax)
                isdone = true;
        }
    }

    if (searchtype == SEARCH_INC)
    {   // smin & smax are given by user
        if (!slist.empty())
        {   // incremental search with a 48-bit list (incl. quad-searches)
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
            // trim the search space to the range [smin, smax]
            uint64_t idxmin, idxmax;
            for (idxmin = 0; idxmin < slist.size(); idxmin++)
                if (slist[idxmin] >= (smin & MASK48))
                    break;
            for (idxmax = 0; idxmax < slist.size(); idxmax++)
                if (slist[idxmax] >= (smax & MASK48))
                    break;
            high = (high - (smin >> 48)) & 0xffff;
            prog = high * slist.size() + idx - idxmin;
            high = ((smax >> 48) - (smin >> 48)) & 0xffff;
            scnt = high * slist.size() + idxmax - idxmin;
        }
        else
        {   // simple incremental search
            seed = sstart;
            if (seed < smin)
                seed = smin;
            prog = seed - smin;
            scnt = smax - smin;
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
            {
                seed = (sstart & ~MASK48) | slist[idx];
                prog = 0x10000 * idx + (seed >> 48);
            }
            smax = slist.back() | (0xffffULL << 48);
        }
        else
        {
            scnt = smax = ~(uint64_t)0;
            seed = sstart;
            prog = (seed << 16) | (seed >> 48);
        }
    }
}

void SearchMaster::run()
{
    stop();
    abort = false;
    QObject *qtobj = parent();
    presearch(qtobj);
    if (qtobj)
        QMetaObject::invokeMethod(qtobj, "closeProtobaseMsg", Qt::BlockingQueuedConnection);
    if (abort)
    {
        emit searchFinish(false);
        return;
    }

    for (int i = 0; i < threadcnt; i++)
    {
        SearchWorker *worker = new SearchWorker(this);
        QObject::connect(
            worker, &SearchWorker::result,
            this, &SearchMaster::onWorkerResult,
            Qt::BlockingQueuedConnection);
        QObject::connect(
            worker, &SearchWorker::finished,
            this, &SearchMaster::onWorkerFinished,
            Qt::QueuedConnection);

        workers.push_back(worker);
    }

    QMutexLocker locker(&mutex);

    proghist.clear();
    progtimer.start();
    itemtimer.start();
    count = 0;

    for (SearchWorker *worker: workers)
    {
        worker->start();
    }
}

void SearchMaster::stop()
{
    abort = true;
    if (workers.empty())
        return;

    // clear event loop of currently running signals (such as results triggers)
    QApplication::processEvents();

    for (long stop_ms = 300; ; stop_ms *= 5)
    {
        QElapsedTimer timer;
        timer.start();

        int running = 0;
        for (SearchWorker *worker: workers)
        {
            long ms = stop_ms - timer.elapsed();
            if (mc < 1) ms = 1;
            running += !worker->wait(ms);
        }
        if (!running)
            break;
        if (parent() == nullptr)
            continue;
        int button = 0;
        Qt::ConnectionType connectiontype = Qt::BlockingQueuedConnection;
        if (QThread::currentThread() == QApplication::instance()->thread())
        {   // main thread would deadlock with a blocking connection
            connectiontype = Qt::DirectConnection;
        }
        QMetaObject::invokeMethod(
                parent(), "warning", connectiontype,
                Q_RETURN_ARG(int, button),
                Q_ARG(QString, tr("Failed to stop %n worker thread(s).\n"
                "Keep waiting for threads to stop?", "", running)),
                Q_ARG(QMessageBox::StandardButtons, QMessageBox::No|QMessageBox::Yes));
        if (button != QMessageBox::Yes)
            break;
    }

    for (SearchWorker *worker: workers)
    {
        if (worker->isRunning())
        {
            worker->disconnect(this);
            connect(worker, &SearchWorker::finished, worker, &QObject::deleteLater);
        }
        else
        {
            delete worker;
        }
    }
    workers.clear();
    emit searchFinish(false);
}


static QString getAbbrNum(double x)
{
    if (x >= 10e9)
        return QString::asprintf("%.1fG", x * 1e-9);
    if (x >= 10e6)
        return QString::asprintf("%.1fM", x * 1e-6);
    if (x >= 10e3)
        return QString::asprintf("%.1fK", x * 1e-3);
    return QString::asprintf("%.2f", x);
}

bool SearchMaster::getProgress(QString *status, uint64_t *prog, uint64_t *end, uint64_t *seed, qreal *min, qreal *avg, qreal *max)
{
    if (!mutex.tryLock(10))
    {
        if (searchtype == SEARCH_BLOCKS && slist.empty())
        {   // a block search with no list looks for candidates in the search
            // master and can therefore make progress outside of workers
            *prog = this->prog;
            *end  = this->scnt;
            *seed = this->seed;
            return true;
        }
        return false;
    }
    *prog = this->prog;
    *end  = this->scnt;
    *seed = this->seed;
    *min = *avg = *max = nan("");

    bool valid = false;
    for (SearchWorker *worker: workers)
    {
        if (worker->prog < *prog)
        {
            *prog = worker->prog;
            *seed = worker->seed;
            valid = true;
        }
    }

    if (isdone)
    {
        *prog = this->scnt;
    }

    mutex.unlock();

    // track the progress over a few seconds so we can estimate the search speed
    enum { SAMPLE_SEC = 20 };
    if (valid)
    {
        TProg tp = { (uint64_t) progtimer.nsecsElapsed(), *prog };
        proghist.push_front(tp);
        while (proghist.size() > 1 && proghist.back().ns < tp.ns - SAMPLE_SEC*1e9)
            proghist.pop_back();
    }

    if (proghist.size() > 1 && proghist.front().ns > proghist.back().ns)
    {
        std::vector<qreal> samples;
        samples.reserve(proghist.size());
        auto it_prev = proghist.begin();
        auto it = it_prev;
        while (++it != proghist.end())
        {
            qreal dp = it_prev->prog - it->prog;
            qreal dt = 1e-9 * (it_prev->ns - it->ns);
            if (dt > 0)
                samples.push_back(dp / dt);
            it_prev = it;
        }
        std::sort(samples.begin(), samples.end());
        qreal speedtot = 0;
        qreal weightot = 1e-6;
        int n = (int) samples.size();
        int r = (int) (n / SAMPLE_SEC);
        int has_zeros = 0;
        for (int i = -r; i <= r; i++)
        {
            int j = n/2 + i;
            if (j < 0 || j >= n)
                continue;
            has_zeros += samples[j] == 0;
            speedtot += samples[j];
            weightot += 1.0;
        }
        *min = samples[n*1/4]; // lower quartile
        *avg = speedtot / weightot; // median
        *max = samples[n*3/4]; // upper quartile
        if (*avg && has_zeros)
        {   // probably a slow sampling regime, use whole range for estimate
            speedtot = 0;
            for (qreal s : samples)
                speedtot += s;
            *avg = speedtot / n;
        }
    }

    qreal remain = ((qreal)*end - *prog) / (*avg + 1e-6);
    QString eta;
    if (remain >= 3600*24*1000)
        eta = "years";
    else if (remain > 0)
    {
        int s = (int) remain;
        if (s > 86400)
            eta = QString("%1d:%2").arg(s / 86400).arg((s % 86400) / 3600, 2, 10, QLatin1Char('0'));
        else if (s > 3600)
            eta = QString("%1h:%2").arg(s / 3600).arg((s % 3600) / 60, 2, 10, QLatin1Char('0'));
        else
            eta = QString("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
    }
    *status = QString("seeds/sec: %1 min: %2 max: %3 isize: %4 eta: %5")
        .arg(getAbbrNum(*avg), -8)
        .arg(getAbbrNum(*min), -8)
        .arg(getAbbrNum(*max), -8)
        .arg(itemsize, -3)
        .arg(eta);

    return valid;
}

bool SearchMaster::requestItem(SearchWorker *item)
{
    if (isdone)
        return false;

    QMutexLocker locker(&mutex);

    // check if we should adjust the item size
    uint64_t nsec = itemtimer.nsecsElapsed();
    count++;
    if (nsec > 0.1e9)
    {
        if (count < 1e2 && itemsize > 1)
            itemsize /= 2;
        if (count > 1e3 && itemsize < 0x10000)
            itemsize *= 2;
        itemtimer.start();
        count = 0;
    }

    item->prog      = prog;
    item->idx       = idx;
    item->sstart    = seed;
    item->scnt      = itemsize;
    item->seed      = seed;

    prog += itemsize;

    if (searchtype == SEARCH_LIST)
    {
        if (idx + itemsize > scnt)
            item->scnt = scnt - idx;
        idx += itemsize;
        if (idx >= scnt)
            isdone = true;
    }

    if (searchtype == SEARCH_48ONLY)
    {
        if (!slist.empty())
        {
            if (idx + itemsize > scnt)
                item->scnt = scnt - idx;
            idx += itemsize;
            if (idx >= scnt)
                isdone = true;
        }
        else
        {
            seed += itemsize;
            if (seed > MASK48)
                isdone = true;
        }
    }

    if (searchtype == SEARCH_INC)
    {
        if (!slist.empty())
        {
            uint64_t high = (seed >> 48) & 0xffff;
            idx += itemsize;
            high += idx / slist.size();
            idx %= slist.size();
            seed = (high << 48) | slist[idx];
            if (high > (smax >> 48))
                isdone = true;
        }
        else
        {
            // seed += itemsize; with overflow detection
            uint64_t s = seed + itemsize;
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
            high += itemsize;
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
            Pos origin = {0,0};
            uint64_t high = (seed >> 48) & 0xffff;
            uint64_t low = seed & MASK48;
            high += itemsize;
            if (high >= 0x10000)
            {
                item->scnt -= 0x10000 - high;
                high = 0;
                low++;

                for (; low <= MASK48 && !abort; low++)
                {
                    env.setSeed(low);
                    if (testTreeAt(origin, &env, PASS_FAST_48, &abort)
                        != COND_FAILED)
                    {
                        break;
                    }
                    // update progress for skipped block
                    seed = low;
                    prog += 0x10000;
                }
                if (low > MASK48)
                    isdone = true;
            }
            seed = (high << 48) | low;
        }
    }

    return true;
}

void SearchMaster::onWorkerResult(uint64_t seed)
{
    emit searchResult(seed);
}

void SearchMaster::onWorkerFinished()
{
    QMutexLocker locker(&mutex);
    if (workers.empty())
        return;
    for (SearchWorker *worker : workers)
        if (!worker->isFinished())
            return;
    for (SearchWorker *worker: workers)
        delete worker;
    workers.clear();
    emit searchFinish(isdone && !abort);
}


SearchWorker::SearchWorker(SearchMaster *master)
    : QThread(nullptr)
    , master(master)
{
    this->slist         = master->slist.empty() ? NULL : master->slist.data();
    this->len           = master->slist.size();
    this->abort         = &master->abort;

    this->prog          = master->prog;
    this->idx           = master->idx;
    this->sstart        = master->seed;
    this->scnt          = 0;
    this->seed          = master->seed;
}


void SearchWorker::run()
{
    Pos origin = {0,0};
    SearchThreadEnv env;
    env.init(master->mc, master->large, &master->condtree);

    switch (master->searchtype)
    {
    case SEARCH_LIST:
        while (!*abort && master->requestItem(this))
        {   // seed = slist[..]
            uint64_t ie = idx+scnt < len ? idx+scnt : len;
            for (uint64_t i = idx; i < ie; i++)
            {
                seed = slist[i];
                env.setSeed(seed);
                if (testTreeAt(origin, &env, PASS_FULL_64, abort) == COND_OK)
                {
                    if (!*abort)
                        emit result(seed);
                }
            }
            //if (ie == len) // done
            //   break;
        }
        break;

    case SEARCH_48ONLY:
        while (!*abort && master->requestItem(this))
        {
            if (slist)
            {
                uint64_t ie = idx+scnt < len ? idx+scnt : len;
                for (uint64_t i = idx; i < ie; i++)
                {
                    seed = slist[i];
                    env.setSeed(seed);
                    if (testTreeAt(origin, &env, PASS_FULL_48, abort) != COND_FAILED)
                    {
                        if (!*abort)
                            emit result(seed);
                    }
                }
            }
            else
            {
                seed = sstart;
                for (int i = 0; i < scnt; i++)
                {
                    env.setSeed(seed);
                    if (testTreeAt(origin, &env, PASS_FULL_48, abort) != COND_FAILED)
                    {
                        if (!*abort)
                            emit result(seed);
                    }

                    if (seed >= MASK48)
                    {   // done
                        break;
                    }
                    seed++;
                }
            }
        }
        break;

    case SEARCH_INC:
        while (!*abort && master->requestItem(this))
        {
            if (slist)
            {   // seed = (high << 48) | slist[..]
                uint64_t high = (sstart >> 48) & 0xffff;
                uint64_t lowidx = idx;

                for (int i = 0; i < scnt; i++)
                {
                    seed = (high << 48) | slist[lowidx];

                    env.setSeed(seed);
                    if (testTreeAt(origin, &env, PASS_FULL_64, abort) == COND_OK)
                    {
                        if (!*abort)
                            emit result(seed);
                    }

                    if (++lowidx >= len)
                    {
                        lowidx = 0;
                        if (++high >= 0x10000)
                        {   // done
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
                    env.setSeed(seed);
                    if (testTreeAt(origin, &env, PASS_FULL_64, abort) == COND_OK)
                    {
                        if (!*abort)
                            emit result(seed);
                    }

                    if (seed == ~(uint64_t)0)
                    {   // done
                        break;
                    }
                    seed++;
                }
            }
        }
        break;

    case SEARCH_BLOCKS:
        while (!*abort && master->requestItem(this))
        {   // seed = ([..] << 48) | low
            if (slist && idx >= len)
            {
                continue;
            }

            uint64_t high = (sstart >> 48) & 0xffff;
            uint64_t low;
            if (slist)
                low = slist[idx];
            else
                low = sstart & MASK48;

            env.setSeed(low);
            if (testTreeAt(origin, &env, PASS_FULL_48, abort) == COND_FAILED)
            {
                continue;
            }

            for (int i = 0; i < scnt; i++)
            {
                seed = (high << 48) | low;

                env.setSeed(seed);
                if (testTreeAt(origin, &env, PASS_FULL_64, abort) == COND_OK)
                {
                    if (!*abort)
                        emit result(seed);
                }

                if (++high >= 0x10000)
                    break; // done
            }
        }
        break;
    }
}



