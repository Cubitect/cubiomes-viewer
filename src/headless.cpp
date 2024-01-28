#include "headless.h"
#include "message.h"
#include "aboutdialog.h"

#include <QStandardPaths>
#include <QApplication>
#include <QDateTime>

#include "cubiomes/util.h"


#if defined(_WIN32)
#include <windows.h>
short get_term_width()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
        return 80;
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}
#else
#include <sys/ioctl.h>
short get_term_width()
{
    struct winsize ws;
    if (ioctl(0, TIOCGWINSZ, &ws) < 0)
        return 80;
    return ws.ws_col;
}
#endif


static QTextStream& qOut()
{
    static QTextStream out (stdout);
    return out;
}

Headless::Headless(QString sessionpath, QString resultspath, QObject *parent)
    : QThread(parent)
    , sthread(nullptr)
    , sessionpath(sessionpath)
    , resultfile(resultspath)
    , resultstream(stdout)
{
    sthread.isdone = true;

    QSettings settings(APP_STRING, APP_STRING);
    g_extgen.load(settings);

    if (!loadSession(sessionpath))
        return;

    if (!sthread.set(nullptr, session))
        return;

    connect(&sthread, &SearchMaster::searchResult, this, &Headless::searchResult, Qt::QueuedConnection);
    connect(&sthread, &SearchMaster::searchFinish, this, &Headless::searchFinish, Qt::QueuedConnection);
    connect(&timer, &QTimer::timeout, this, QOverload<>::of(&Headless::progressTimeout));

    if (!resultfile.fileName().isEmpty())
    {
        if (resultfile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
            resultstream.setDevice(&resultfile);
        else
            warn(nullptr, "Output file for results coult not be created - using stdout instead.");
    }
}

Headless::~Headless()
{
}

static bool load_seeds(std::vector<uint64_t>& seeds, QString path)
{
    QByteArray ba = path.toLocal8Bit();
    uint64_t len;
    if (uint64_t *l = loadSavedSeeds(ba.data(), &len))
    {
        seeds.assign(l, l+len);
        free(l);
        return true;
    }
    return false;
}

bool Headless::loadSession(QString sessionpath)
{
    qOut() << "Loading session: \"" << sessionpath << "\"\n";
    qOut().flush();

    if (!session.load(nullptr, sessionpath, false))
    {
        return false;
    }

    if (session.cv.empty())
    {
        warn(nullptr, "Session defines no search constraints.");
        return false;
    }
    if (session.sc.searchtype == SEARCH_LIST)
    {
        if (!load_seeds(session.slist, session.sc.slist64path))
        {
            warn(nullptr, QString("Failed to load 64-bit seed list:\n\"%1\"").arg(session.sc.slist64path));
            return false;
        }
    }
    else if (session.gen48.mode == GEN48_LIST)
    {
        if (!load_seeds(session.slist, session.gen48.slist48path))
        {
            warn(nullptr, QString("Failed to load 48-bit seed list:\n\"%1\"").arg(session.gen48.slist48path));
            return false;
        }
    }
    else
    {
        session.slist.clear();
    }

    return true;
}

void Headless::run()
{
    qOut() << "Condition summary:\n";
    for (const Condition& cond : qAsConst(session.cv))
        qOut() << cond.summary(false) << "\n";

    if (sthread.isdone)
    {
        qOut() << "Search parameters invalid or incomplete.\n";
        qOut().flush();
        searchFinish(false);
        return;
    }

    qOut() << "\nSearching for seeds...\n\n";
    qOut().flush();

    session.writeHeader(resultstream);
    resultstream.flush();

    sthread.start();
    elapsed.start();

    if (resultfile.isOpen())
    {
        qOut() << "\n\n\n\n\n\n\n";
        qOut().flush();
        timer.start(250);
    }
}

void Headless::searchResult(uint64_t seed)
{
    results.push_back(seed);
    resultstream << (int64_t) seed << "\n";
    resultstream.flush();
}

void Headless::searchFinish(bool done)
{
    if (timer.isActive())
    {
        timer.stop();
        progressTimeout();
    }
    if (done)
        qOut() << "Search done!\n";
    qOut() << "Stopping event loop.\n";
    qOut().flush();
    emit finished();
}

void Headless::progressTimeout()
{
    QString status;
    uint64_t prog, end, seed;
    qreal min, avg, max;
    sthread.getProgress(&status, &prog, &end, &seed, &min, &avg, &max);

    short width = get_term_width();
    if (width <= 24)
        return;

    qreal perc = (qreal) prog / end;

    int cols = floor(perc * (width - 4) + 1e-6);
    qint64 sec = elapsed.elapsed() / 1000;

    QStringList l;
    l += QString(" Found matching seeds:%1 ").arg(results.size(), width-23);
    l += QString(" Scheduled seed:%1 ").arg((int64_t)seed, width-17);
    l += QString(" Progress:%1 ").arg(QString("%1 / %2 : %3%").arg(prog).arg(end).arg(100*perc, 5, 'f', 2), width-11);
    l += QString(" [%1%2] ").arg("", cols, '#').arg("", width-cols-4, '-');
    l += QString(" %1").arg(status, 1-width);
    l += QString::asprintf(" %d:%02d:%02d", (int)(sec / 3600), (int)(sec / 60) % 60, (int)(sec % 60));
    l += "";

    qOut() << "\e[999D\e[" << l.size() << "A";
    for (QString& s : l)
    {
        s.truncate(width);
        qOut() << s << "\n";
    }
    qOut().flush();
}


