#include "formsearchcontrol.h"
#include "ui_formsearchcontrol.h"

#include "mainwindow.h"
#include "search.h"
#include "rangedialog.h"

#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>


FormSearchControl::FormSearchControl(MainWindow *parent)
    : QWidget(parent)
    , parent(parent)
    , ui(new Ui::FormSearchControl)
    , protodialog()
    , sthread(this)
    , stimer()
    , elapsed()
    , proghist()
    , slist64path()
    , slist64fnam()
    , slist64()
    , smin(0)
    , smax(~(uint64_t)0)
{
    ui->setupUi(this);
    protodialog = new ProtoBaseDialog(this);

    QFont mono = QFont("Monospace", 9);
    mono.setStyleHint(QFont::TypeWriter);
    ui->listResults->setFont(mono);
    ui->progressBar->setFont(mono);
    ui->labelStatus->setFont(mono);

    ui->listResults->horizontalHeader()->setFont(mono);

    connect(&sthread, &SearchMaster::searchFinish, this, &FormSearchControl::searchFinish, Qt::QueuedConnection);

    connect(&stimer, &QTimer::timeout, this, QOverload<>::of(&FormSearchControl::resultTimeout));

    searchProgressReset();
    ui->spinThreads->setMaximum(QThread::idealThreadCount());
    ui->spinThreads->setValue(QThread::idealThreadCount());

    searchLockUi(false);
}

FormSearchControl::~FormSearchControl()
{
    stimer.stop();
    sthread.stop(); // tell search to stop at next convenience
    sthread.quit(); // tell the event loop to exit
    sthread.wait(); // wait for search to finish
    delete ui;
}

QVector<uint64_t> FormSearchControl::getResults()
{
    int n = ui->listResults->rowCount();
    QVector<uint64_t> results = QVector<uint64_t>(n);
    for (int i = 0; i < n; i++)
    {
        results[i] = ui->listResults->item(i, 0)->data(Qt::UserRole).toULongLong();
    }
    return results;
}

SearchConfig FormSearchControl::getSearchConfig()
{
    SearchConfig s;
    s.searchtype = ui->comboSearchType->currentIndex();
    s.threads = ui->spinThreads->value();
    s.slist64path = slist64path;
    s.startseed = ui->lineStart->text().toLongLong();
    s.stoponres = ui->checkStop->isChecked();
    s.smin = smin;
    s.smax = smax;
    return s;
}

bool FormSearchControl::setSearchConfig(SearchConfig s, bool quiet)
{
    bool ok = true;
    if (s.searchtype >= SEARCH_INC && s.searchtype <= SEARCH_LIST)
    {
        ui->comboSearchType->setCurrentIndex(s.searchtype);
        on_comboSearchType_currentIndexChanged(s.searchtype);
    }
    else
    {
        ok = false;
    }

    ui->spinThreads->setValue(s.threads);
    ui->checkStop->setChecked(s.stoponres);
    smin = s.smin;
    smax = s.smax;

    if (ok)
        ok &= setList64(s.slist64path, quiet);

    ui->lineStart->setText(QString::asprintf("%" PRId64, (int64_t)s.startseed));

    return ok;
}

void FormSearchControl::stopSearch()
{
    sthread.stop();
    //sthread.quit(); // tell the event loop to exit
    //sthread.wait(); // wait for search to finish
}

bool FormSearchControl::setList64(QString path, bool quiet)
{
    if (!path.isEmpty())
    {
        QFileInfo finfo(path);
        parent->prevdir = finfo.absolutePath();
        slist64fnam = finfo.fileName();
        slist64path = path;
        uint64_t *l = NULL;
        uint64_t len;
        QByteArray ba = path.toLatin1();
        l = loadSavedSeeds(ba.data(), &len);
        if (l != NULL)
        {
            slist64.assign(l, l+len);
            searchProgress(0, len, l[0]);
            free(l);
            return true;
        }
        else if (!quiet)
        {
            int button = QMessageBox::warning(
                this, tr("Warning"),
                tr("Failed to load 64-bit seed list from file:\n\"%1\"").arg(path),
                QMessageBox::Reset, QMessageBox::Ignore);
            if (button == QMessageBox::Reset)
            {
                slist64fnam.clear();
                slist64path.clear();
                slist64.clear();
            }
        }
    }
    return false;
}

void FormSearchControl::searchLockUi(bool lock)
{
    if (lock)
    {
        ui->comboSearchType->setEnabled(false);
        ui->spinThreads->setEnabled(false);
        ui->buttonMore->setEnabled(false);
    }
    else
    {
        ui->buttonStart->setText(tr("Start search"));
        ui->buttonStart->setIcon(QIcon(":/icons/search.png"));
        ui->buttonStart->setChecked(false);
        ui->buttonStart->setEnabled(true);
        ui->comboSearchType->setEnabled(true);
        ui->spinThreads->setEnabled(true);
        int st = ui->comboSearchType->currentIndex();
        ui->buttonMore->setEnabled(st == SEARCH_INC || st == SEARCH_LIST);
    }
    emit searchStatusChanged(lock);
}

void FormSearchControl::setSearchMode(int mode)
{
    ui->comboSearchType->setCurrentIndex(mode);
    if (mode == SEARCH_LIST)
    {
        on_buttonMore_clicked();
    }
    else
    {
        slist64.clear();
        slist64path.clear();
        slist64fnam.clear();
    }
}


int FormSearchControl::warning(QString text, QMessageBox::StandardButtons buttons)
{
    return QMessageBox::warning(this, tr("Warning"), text, buttons);
}

void FormSearchControl::openProtobaseMsg(QString path)
{
    protodialog->setPath(path);
    protodialog->show();
}

void FormSearchControl::closeProtobaseMsg()
{
    if (protodialog->closeOnDone())
        protodialog->close();
}

void FormSearchControl::on_buttonClear_clicked()
{
    ui->listResults->clearContents();
    ui->listResults->setRowCount(0);
    searchProgressReset();
    ui->lineStart->setText("0");
}

void FormSearchControl::on_buttonStart_clicked()
{
    if (ui->buttonStart->isChecked())
    {
        WorldInfo wi;
        parent->getSeed(&wi);
        const Config& config = parent->config;
        QVector<Condition> condvec = parent->formCond->getConditions();
        SearchConfig sc = getSearchConfig();
        int ok = true;

        if (condvec.empty())
        {
            QMessageBox::warning(this, tr("Warning"), tr("Please define some constraints using the \"Add\" button."), QMessageBox::Ok);
            ok = false;
        }
        if (sc.searchtype == SEARCH_LIST && slist64.empty())
        {
            QMessageBox::warning(this, tr("Warning"), tr("No seed list file selected."), QMessageBox::Ok);
            ok = false;
        }
        if (sthread.isRunning())
        {
            QMessageBox::warning(this, tr("Warning"), tr("Search is still running."), QMessageBox::Ok);
            ok = false;
        }

        if (ok)
        {
            for (Condition& c : condvec)
                c.apply(wi);

            Gen48Settings gen48 = parent->formGen48->getSettings(true);
            // the search can either use a full list or a 48-bit list
            if (sc.searchtype == SEARCH_LIST)
                slist = slist64;
            else if (gen48.mode == GEN48_LIST)
                slist = parent->formGen48->getList48();
            else
                slist.clear();

            ok = sthread.set(wi, sc, gen48, config, slist, condvec);
        }

        if (ok)
        {
            ui->lineStart->setText(QString::asprintf("%" PRId64, (int64_t)sc.startseed));
            ui->buttonStart->setText(tr("Abort search"));
            ui->buttonStart->setIcon(QIcon(":/icons/cancel.png"));
            searchLockUi(true);
            sthread.start();
            elapsed.start();
            stimer.start(250);
        }
        else
        {
            ui->buttonStart->setChecked(false);
        }
    }
    else
    {
        ui->buttonStart->setText(tr("Start search"));
        ui->buttonStart->setIcon(QIcon(":/icons/search.png"));
        ui->buttonStart->setChecked(false);

        // disable until finish
        ui->buttonStart->setEnabled(false);
        stopSearch();
    }

    update();
}

void FormSearchControl::on_buttonMore_clicked()
{
    int type = ui->comboSearchType->currentIndex();
    if (type == SEARCH_LIST)
    {
        QString fnam = QFileDialog::getOpenFileName(
            this, tr("Load seed list"), parent->prevdir, tr("Text files (*.txt);;Any files (*)"));
        setList64(fnam, false);
    }
    else if (type == SEARCH_INC)
    {
        RangeDialog *dialog = new RangeDialog(this, smin, smax);
        int status = dialog->exec();
        if (status == QDialog::Accepted)
        {
            dialog->getBounds(&smin, &smax);
            searchProgressReset();
        }
    }
}

void FormSearchControl::on_listResults_itemSelectionChanged()
{
    int row = ui->listResults->currentRow();
    if (row >= 0 && row < ui->listResults->rowCount())
    {
        uint64_t s = ui->listResults->item(row, 0)->data(Qt::UserRole).toULongLong();
        emit selectedSeedChanged(s);
    }
}

void FormSearchControl::on_listResults_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);

    // this is a contextual temporary menu so shortcuts are only indicated here,
    // but will not function - see keyReleaseEvent() for shortcut implementation

    QAction *actremove = menu.addAction(QIcon::fromTheme("list-remove"),
        tr("Remove selected seed"), this,
        &FormSearchControl::removeCurrent, QKeySequence::Delete);
    actremove->setEnabled(!ui->listResults->selectedItems().empty());

    QAction *actcopy = menu.addAction(QIcon::fromTheme("edit-copy"),
        tr("Copy list to clipboard"), this,
        &FormSearchControl::copyResults, QKeySequence::Copy);
    actcopy->setEnabled(ui->listResults->rowCount() > 0);

    int n = pasteList(true);
    QAction *actpaste = menu.addAction(QIcon::fromTheme("edit-paste"),
        tr("Paste %n seed(s) from clipboard", "", n), this,
        &FormSearchControl::pasteResults, QKeySequence::Paste);
    actpaste->setEnabled(n > 0);
    menu.exec(ui->listResults->mapToGlobal(pos));
}

void FormSearchControl::on_buttonSearchHelp_clicked()
{
    QMessageBox mb(this);
    mb.setIcon(QMessageBox::Information);
    mb.setWindowTitle(tr("Help: search types"));
    mb.setText(tr(
        "<html><head/><body><p>"
        "The <b>incremental</b> search checks seeds in numerical order, "
        "save for grouping into work items for parallelization. This type "
        "of search is best suited for a non-exhaustive search space and "
        "with strong biome dependencies. You can restrict this type of "
        "search to a value range using the &quot;...&quot; button."
        "</p><p>"
        "With <b>48-bit family blocks</b> the search looks for suitable "
        "48-bit seeds first and parallelizes the search through the upper "
        "16-bits. This search type is best suited for exhaustive searches and "
        "those with very restrictive structure requirements."
        "</p><p>"
        "Load a <b>seed list from a file</b> to search through an "
        "existing set of seeds. The seeds should be in decimal ASCII text, "
        "separated by newline characters. You can browse for a file using "
        "the &quot;...&quot; button. (The seed generator is ignored with "
        "this option.)"
        "</p></body></html>"
        ));
    mb.exec();
}

void FormSearchControl::on_comboSearchType_currentIndexChanged(int index)
{
    ui->buttonMore->setEnabled(index == SEARCH_INC || index == SEARCH_LIST);
    searchProgressReset();
}

void FormSearchControl::pasteResults()
{
    pasteList(false);
}

int FormSearchControl::pasteList(bool dummy)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QStringList slist = clipboard->text().split('\n');
    QVector<uint64_t> seeds;

    for (QString s : slist)
    {
        s = s.trimmed();
        if (s.isEmpty())
            continue;
        bool ok = true;
        uint64_t seed = (uint64_t) s.toLongLong(&ok);
        if (!ok)
            return 0;
        seeds.push_back(seed);
    }

    if (!seeds.empty())
    {
        return searchResultsAdd(seeds, dummy);
    }
    return 0;
}


int FormSearchControl::searchResultsAdd(QVector<uint64_t> seeds, bool countonly)
{
    const Config& config = parent->config;
    int ns = ui->listResults->rowCount();
    int n = ns;
    if (n >= config.maxMatching)
        return 0;
    if (seeds.size() + n > config.maxMatching)
        seeds.resize(config.maxMatching - n);
    if (seeds.empty())
        return 0;

    QSet<uint64_t> current;
    current.reserve(n + seeds.size());
    for (int i = 0; i < n; i++)
    {
        uint64_t seed = ui->listResults->item(i, 0)->data(Qt::UserRole).toULongLong();
        current.insert(seed);
    }

    ui->listResults->setSortingEnabled(false);
    for (uint64_t s : seeds)
    {
        if (current.contains(s))
            continue;
        if (countonly)
        {
            n++;
            continue;
        }
        current.insert(s);
        QTableWidgetItem* s48item = new QTableWidgetItem();
        QTableWidgetItem* seeditem = new QTableWidgetItem();
        s48item->setData(Qt::UserRole, QVariant::fromValue(s));
        s48item->setText(QString::asprintf("%012llx|%04x",
                (qulonglong)(s & MASK48), (uint)(s >> 48) & 0xffff));
        seeditem->setData(Qt::DisplayRole, QVariant::fromValue((int64_t)s));
        ui->listResults->insertRow(n);
        ui->listResults->setItem(n, 0, s48item);
        ui->listResults->setItem(n, 1, seeditem);
        n++;
    }
    ui->listResults->setSortingEnabled(true);

    if (countonly == false && n >= config.maxMatching)
    {
        sthread.stop();
        QString msg = tr("Maximum number of results reached (%1).").arg(config.maxMatching);
        QMessageBox::warning(this, tr("Warning"), msg, QMessageBox::Ok);
    }

    int addcnt = n - ns;
    if (ui->checkStop->isChecked() && addcnt)
        sthread.abort = true;

    if (addcnt)
        emit resultsAdded(addcnt);

    return addcnt;
}

void FormSearchControl::searchProgressReset()
{
    uint64_t cnt;
    cnt = parent->formGen48->estimateSeedCnt();
    if (cnt > MASK48)
        cnt = ~(uint64_t)0;
    else
        cnt <<= 16;

    QString fmt;
    int searchtype = ui->comboSearchType->currentIndex();
    if (searchtype == SEARCH_LIST)
    {
        if (!slist64fnam.isEmpty())
        {
            fmt = slist64fnam + ": ";
            cnt = slist64.size();
        }
    }
    if (searchtype == SEARCH_INC)
    {
        if (smin != 0 || smax != ~(uint64_t)0)
        {
            fmt = QString::asprintf(" [%" PRIu64 " - %" PRIu64 "]: ", smin, smax);
            cnt = 0;
        }
    }

    if (cnt)
        fmt += QString::asprintf("0 / %" PRIu64 " (0.00%%)", cnt);
    else
        fmt += "0 / ? (0.00%%)";

    ui->progressBar->setValue(0);
    ui->progressBar->setFormat(fmt);
}

void FormSearchControl::searchProgress(uint64_t prog, uint64_t end, int64_t seed)
{
    ui->lineStart->setText(QString::asprintf("%" PRId64, seed));

    if (end)
    {
        int v = (int) floor(10000 * (double)prog / end);
        ui->progressBar->setValue(v);
        QString fmt = QString::asprintf(
                    "%" PRIu64 " / %" PRIu64 " (%d.%02d%%)",
                    prog, end, v / 100, v % 100
                    );
        int searchtype = ui->comboSearchType->currentIndex();
        if (searchtype == SEARCH_LIST)
        {
            if (!slist64fnam.isEmpty())
            {
                fmt = slist64fnam + ": " + fmt;
            }
        }
        if (searchtype == SEARCH_INC)
        {
            if (smin != 0 || smax != ~(uint64_t)0)
            {
                fmt = QString::asprintf("[%" PRIu64 " - %" PRIu64 "]: ", smin, smax) + fmt;
            }
        }
        ui->progressBar->setFormat(fmt);
    }
}

void FormSearchControl::searchFinish(bool done)
{
    stimer.stop();
    resultTimeout();
    if (done)
    {
        ui->lineStart->setText(QString::asprintf("%" PRId64, sthread.smax));
        ui->progressBar->setValue(10000);
        ui->progressBar->setFormat(tr("Done", "Progressbar when finished"));
    }
    ui->labelStatus->setText("Idle");
    proghist.clear();
    searchLockUi(false);
}

#define SAMPLE_SEC 20

static void estmateSpeed(const std::deque<FormSearchControl::TProg>& hist,
    double *min, double *avg, double *max)
{   // We will try to get a decent estimate for the search speed.
    std::vector<double> samples;
    samples.reserve(hist.size());
    auto it_last = hist.begin();
    auto it = it_last;
    while (++it != hist.end())
    {
        double dp = it_last->prog - it->prog;
        double dt = 1e-9 * (it_last->ns - it->ns);
        if (dt > 0)
            samples.push_back(dp / dt);
        it_last = it;
    }
    std::sort(samples.begin(), samples.end());
    double speedtot = 0;
    double weightot = 1e-6;
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
    if (min) *min = samples[n*1/4]; // lower quartile
    if (avg) *avg = speedtot / weightot; // median
    if (max) *max = samples[n*3/4]; // upper quartile
    if (avg && has_zeros)
    {   // probably a slow sampling regime, use whole range for estimate
        speedtot = 0;
        for (double s : samples)
            speedtot += s;
        *avg = speedtot / n;
    }
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

void FormSearchControl::resultTimeout()
{
    uint64_t prog, end, seed;
    if (!sthread.getProgress(&prog, &end, &seed))
        return;
    searchProgress(prog, end, seed);

    // track the progress over a few seconds so we can estimate the search speed
    TProg tp = { (uint64_t) elapsed.nsecsElapsed(), prog };
    proghist.push_front(tp);
    while (proghist.size() > 1 && proghist.back().ns < tp.ns - SAMPLE_SEC*1e9)
        proghist.pop_back();

    QString status = tr("Running...");
    if (proghist.size() > 1 && proghist.front().ns > proghist.back().ns)
    {
        double min, avg, max;
        estmateSpeed(proghist, &min, &avg, &max);
        status = tr("seeds/sec: %1 min: %2 max: %3 isize: %4")
            .arg(getAbbrNum(avg), -8)
            .arg(getAbbrNum(min), -8)
            .arg(getAbbrNum(max), -8)
            .arg(sthread.itemsize);
    }
    ui->labelStatus->setText(status);

    update();
}

void FormSearchControl::removeCurrent()
{
    int row = ui->listResults->currentRow();
    if (row >= 0)
        ui->listResults->removeRow(row);
}

void FormSearchControl::copyResults()
{
    QString text;
    int n = ui->listResults->rowCount();
    for (int i = 0; i < n; i++)
    {
        uint64_t seed = ui->listResults->item(i, 0)->data(Qt::UserRole).toULongLong();
        text += QString::asprintf("%" PRId64 "\n", seed);
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
}

void FormSearchControl::keyReleaseEvent(QKeyEvent *event)
{
    if (ui->listResults->hasFocus())
    {
        if (event->matches(QKeySequence::Delete))
            removeCurrent();
        else if (event->matches(QKeySequence::Copy))
            copyResults();
        else if (event->matches(QKeySequence::Paste))
            pasteResults();
    }
    QWidget::keyReleaseEvent(event);
}


