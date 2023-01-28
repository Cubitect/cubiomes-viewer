#include "formsearchcontrol.h"
#include "ui_formsearchcontrol.h"

#include "mainwindow.h"
#include "search.h"
#include "rangedialog.h"

#include "cubiomes/util.h"

#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QFontMetrics>


QVariant SeedTableModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (index.column() == COL_SEED)
            return seeds[index.row()].txtSeed;
        if (index.column() == COL_HEX48)
            return seeds[index.row()].txtHex48;
        if (index.column() == COL_TOP16)
            return seeds[index.row()].txtTop16;
    }
    else if (role == Qt::UserRole)
    {
        if (index.column() == COL_HEX48)
            return seeds[index.row()].varHex48;
        if (index.column() == COL_TOP16)
            return seeds[index.row()].varTop16;
        return seeds[index.row()].varSeed;
    }
    return QVariant::Invalid;
}

QVariant SeedTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0)
        return QVariant::Invalid;
    if (role == Qt::InitialSortOrderRole)
        return QVariant::fromValue(Qt::DescendingOrder);
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if (section == COL_SEED)
            return QVariant::fromValue(tr("seed"));
        if (section == COL_TOP16)
            return QVariant::fromValue(tr("top 16"));
        if (section == COL_HEX48)
            return QVariant::fromValue(tr("lower 48 bit"));
    }
    if (role == Qt::DisplayRole && orientation == Qt::Vertical)
        return QVariant::fromValue(section + 1);
    return QVariant::Invalid;
}

int SeedTableModel::insertSeeds(QVector<uint64_t> newseeds)
{
    int row = seeds.size();
    beginInsertRows(QModelIndex(), row, row + newseeds.size()-1);
    for (uint64_t seed : qAsConst(newseeds))
    {
        Seed s;
        s.seed = seed;
        s.varSeed = QVariant::fromValue(seed);
        s.varTop16 = QVariant::fromValue((quint64)(seed>>48) & 0xFFFF);
        s.varHex48 = QVariant::fromValue((quint64)(seed & MASK48));
        s.txtSeed = QVariant::fromValue(QString::asprintf("%" PRId64, seed));
        s.txtTop16 = QVariant::fromValue(QString::asprintf("%04llx", (quint64)(seed>>48) & 0xFFFF));
        s.txtHex48 = QVariant::fromValue(QString::asprintf("%012llx", (quint64)(seed & MASK48)));
        seeds.append(s);
    }
    endInsertRows();
    return row;
}

void SeedTableModel::removeRow(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    seeds.removeAt(row);
    endRemoveRows();
}

void SeedTableModel::reset()
{
    beginRemoveRows(QModelIndex(), 0, seeds.size());
    seeds.clear();
    endRemoveRows();
}

FormSearchControl::FormSearchControl(MainWindow *parent)
    : QWidget(parent)
    , parent(parent)
    , ui(new Ui::FormSearchControl)
    , model(new SeedTableModel(this))
    , proxy(new SeedSortProxy(this))
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
    , qbuf()
    , nextupdate()
    , updt(20)
{
    ui->setupUi(this);
    protodialog = new ProtoBaseDialog(this);

    QFont mono = *gp_font_mono;
    ui->results->setFont(mono);
    ui->progressBar->setFont(mono);
    ui->labelStatus->setFont(mono);

    proxy->setSourceModel(model);
    ui->results->setModel(proxy);

    ui->results->horizontalHeader()->setFont(mono);
    ui->results->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->results->verticalHeader()->setDefaultSectionSize(QFontMetrics(mono).height());
    ui->results->setColumnWidth(SeedTableModel::COL_SEED, 200);
    ui->results->setColumnWidth(SeedTableModel::COL_TOP16, 60);
    ui->results->setColumnWidth(SeedTableModel::COL_HEX48, 120);

    connect(ui->results->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, &FormSearchControl::onSort);
    ui->results->sortByColumn(-1, Qt::AscendingOrder);

    connect(&sthread, &SearchMaster::searchFinish, this, &FormSearchControl::searchFinish, Qt::QueuedConnection);

    connect(&stimer, &QTimer::timeout, this, QOverload<>::of(&FormSearchControl::resultTimeout));

    connect(
        ui->results->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
        SLOT(onSeedSelectionChanged()));

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
    int n = proxy->rowCount();
    QVector<uint64_t> results = QVector<uint64_t>(n);
    for (int i = 0; i < n; i++)
    {
        results[i] = proxy->data(proxy->index(i, SeedTableModel::COL_SEED), Qt::UserRole).toULongLong();
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
        QByteArray ba = path.toLocal8Bit();
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
    model->reset();
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
            nextupdate = 0;
            updt = 20;
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

void FormSearchControl::onSeedSelectionChanged()
{
    int row = ui->results->currentIndex().row();
    if (row >= 0 && row < ui->results->model()->rowCount())
    {
        QModelIndex idx = ui->results->model()->index(row, SeedTableModel::COL_SEED);
        uint64_t s = ui->results->model()->data(idx, Qt::UserRole).toULongLong();
        emit selectedSeedChanged(s);
    }
}

void FormSearchControl::on_results_clicked(const QModelIndex &)
{
    onSeedSelectionChanged();
}

void FormSearchControl::on_results_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);

    // this is a contextual temporary menu so shortcuts are only indicated here,
    // but will not function - see keyReleaseEvent() for shortcut implementation

    QAction *actremove = menu.addAction(QIcon::fromTheme("list-remove"),
        tr("Remove selected seed"), this,
        &FormSearchControl::removeCurrent, QKeySequence::Delete);
    actremove->setEnabled(!ui->results->selectionModel()->hasSelection());

    QAction *actcopy = menu.addAction(QIcon::fromTheme("edit-copy"),
        tr("Copy list to clipboard"), this,
        &FormSearchControl::copyResults, QKeySequence::Copy);
    actcopy->setEnabled(ui->results->model()->rowCount() > 0);

    int n = pasteList(true);
    QAction *actpaste = menu.addAction(QIcon::fromTheme("edit-paste"),
        tr("Paste %n seed(s) from clipboard", "", n), this,
        &FormSearchControl::pasteResults, QKeySequence::Paste);
    actpaste->setEnabled(n > 0);
    menu.exec(ui->results->mapToGlobal(pos));
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

void FormSearchControl::onSort(int, Qt::SortOrder)
{
    // We want to achieve: none -> descending -> ascending -> none
    // The headerview flips the indicator with the logic:
    //  if (same_section)
    //      new_order = old_order == descending ? ascending : descending
    //  else
    //      new_order = InitialSortOrder (else ascending)
    QHeaderView *header = ui->results->horizontalHeader();

    if (proxy->order == Qt::AscendingOrder && proxy->column != -1)
    {
        header->setSortIndicatorShown(false);
        header->setSortIndicator(-1, Qt::DescendingOrder);
        proxy->column = -1;
    }
    else
    {
        header->setSortIndicatorShown(true);
    }
}

void FormSearchControl::onSearchResult(uint64_t seed)
{
    qbuf.push_back(seed);
    if (ui->checkStop->isChecked())
    {
        onBufferTimeout();
        return;
    }

    quint64 ns = elapsed.nsecsElapsed();
    if (ns > nextupdate)
    {
        nextupdate = ns + updt * 1e6;
        QTimer::singleShot(updt, this, &FormSearchControl::onBufferTimeout);
    }
}

void FormSearchControl::onBufferTimeout()
{
    uint64_t t = -elapsed.elapsed();

    searchResultsAdd(qbuf, false);
    qbuf.clear();

    QApplication::processEvents(); // force processing of events so we can time correctly

    t += elapsed.elapsed();
    if (8*t > updt)
        updt = 4*t;
    nextupdate = elapsed.nsecsElapsed() + 1e6 * updt;
}

int FormSearchControl::searchResultsAdd(QVector<uint64_t> seeds, bool countonly)
{
    const Config& config = parent->config;
    int ns = model->seeds.size();
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
        current.insert(model->seeds[i].seed);

    QVector<uint64_t> newseeds;
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
        newseeds.append(s);
        n++;
    }
    if (!newseeds.empty())
    {
        ui->results->setSortingEnabled(false);
        model->insertSeeds(newseeds);
        ui->results->setSortingEnabled(true);
    }
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
        double remain = ((double)end - prog) / (avg + 1e-6);
        QString eta;
        if (remain >= 3600*24*1000)
            eta = "years";
        else
        {
            int s = (int) remain;
            if (s > 86400)
                eta = QString("%1d:%2").arg(s / 86400).arg((s % 86400) / 3600, 2, 10, QLatin1Char('0'));
            else if (s > 3600)
                eta = QString("%1h:%2").arg(s / 3600).arg((s % 3600) / 60, 2, 10, QLatin1Char('0'));
            else
                eta = QString("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
        }
        status = tr("seeds/sec: %1 min: %2 max: %3 isize: %4 eta: %5")
            .arg(getAbbrNum(avg), -8)
            .arg(getAbbrNum(min), -8)
            .arg(getAbbrNum(max), -8)
            .arg(sthread.itemsize, -3)
            .arg(eta);
    }
    ui->labelStatus->setText(status);

    update();
}

void FormSearchControl::removeCurrent()
{
    QModelIndex index = ui->results->currentIndex();
    int row = proxy->mapToSource(index).row();
    if (row >= 0)
    {
        model->removeRow(row);
    }
}

void FormSearchControl::copyResults()
{
    QString text;
    int n = ui->results->model()->rowCount();
    for (int i = 0; i < n; i++)
    {
        QModelIndex idx = ui->results->model()->index(i, SeedTableModel::COL_SEED);
        uint64_t seed = ui->results->model()->data(idx, Qt::UserRole).toULongLong();
        text += QString::asprintf("%" PRId64 "\n", seed);
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
}

void FormSearchControl::keyReleaseEvent(QKeyEvent *event)
{
    if (ui->results->hasFocus())
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
