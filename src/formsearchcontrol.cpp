#include "formsearchcontrol.h"
#include "ui_formsearchcontrol.h"

#include "mainwindow.h"
#include "message.h"
#include "rangedialog.h"
#include "search.h"
#include "util.h"

#include <QAction>
#include <QClipboard>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMenu>


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
    else if (role == Qt::TextAlignmentRole)
    {
        static QVariant align = QVariant::fromValue((int)Qt::AlignRight | Qt::AlignVCenter);
        if (index.column() != COL_HEX48)
            return align;
    }
    return QVariant();
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
    return QVariant();
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
    if (seeds.empty())
        return;
    beginRemoveRows(QModelIndex(), 0, seeds.size());
    seeds.clear();
    endRemoveRows();
}

FormSearchControl::FormSearchControl(MainWindow *parent)
    : QWidget(parent)
    , parent(parent)
    , ui(new Ui::FormSearchControl)
    , model()
    , proxy()
    , sthread(this)
    , elapsed()
    , stimer()
    , resultfile()
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

    ui->comboSearchType->addItem(tr("incremental"), SEARCH_INC);
    ui->comboSearchType->addItem(tr("48-bit only"), SEARCH_48ONLY);
    ui->comboSearchType->addItem(tr("48-bit family blocks"), SEARCH_BLOCKS);
    ui->comboSearchType->addItem(tr("text seeds"), SEARCH_TEXT);
    ui->comboSearchType->addItem(tr("random seeds"), SEARCH_RANDOM);
    ui->comboSearchType->addItem(tr("seed list from file..."), SEARCH_LIST);

    model = new SeedTableModel(ui->results);
    proxy = new SeedSortProxy(ui->results);

    proxy->setSourceModel(model);
    ui->results->setModel(proxy);

    ui->results->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    connect(ui->results->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, &FormSearchControl::onSort);
    ui->results->sortByColumn(-1, Qt::AscendingOrder);

    connect(&sthread, &SearchMaster::searchResult, this, &FormSearchControl::searchResult, Qt::QueuedConnection);
    connect(&sthread, &SearchMaster::searchFinish, this, &FormSearchControl::searchFinish, Qt::QueuedConnection);

    connect(&stimer, &QTimer::timeout, this, QOverload<>::of(&FormSearchControl::progressTimeout));

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
    sthread.stopSearch();
    delete ui;
}

bool FormSearchControl::event(QEvent *e)
{
    if (e->type() == QEvent::LayoutRequest)
    {
        QFontMetrics fm = QFontMetrics(ui->results->font());
        ui->results->setColumnWidth(SeedTableModel::COL_SEED, txtWidth(fm, QString(24, '#')));
        ui->results->setColumnWidth(SeedTableModel::COL_TOP16, txtWidth(fm, QString(12, '#')));
        ui->results->verticalHeader()->setDefaultSectionSize(fm.height());
    }
    return QWidget::event(e);
}

std::vector<uint64_t> FormSearchControl::getResults()
{
    int n = proxy->rowCount();
    std::vector<uint64_t> results (n);
    for (int i = 0; i < n; i++)
    {
        results[i] = proxy->data(proxy->index(i, SeedTableModel::COL_SEED), Qt::UserRole).toULongLong();
    }
    return results;
}

SearchConfig FormSearchControl::getSearchConfig()
{
    SearchConfig s;
    s.searchtype = ui->comboSearchType->currentData().toInt();
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
    if (s.searchtype >= SEARCH_INC && s.searchtype <= SEARCH_48ONLY)
    {
        ui->comboSearchType->setCurrentIndex(ui->comboSearchType->findData(s.searchtype));
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

#if WASM
    (void) quiet;
#else
    if (ok)
        ok &= setList64(s.slist64path, quiet);
#endif

    ui->lineStart->setText(QString::asprintf("%" PRId64, (int64_t)s.startseed));

    return ok;
}

void FormSearchControl::stopSearch()
{
    sthread.stopSearch();
    onBufferTimeout();
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
            updateSearchProgress(0, len, l[0]);
            free(l);
            return true;
        }
        else if (!quiet)
        {
            int button = warn(this, tr("Warning"),
                tr("Failed to load 64-bit seed list from file:\n\"%1\"").arg(path),
                tr("Reset list path?"), QMessageBox::Reset | QMessageBox::Ignore);
            if (button != QMessageBox::Ignore)
            {
                slist64fnam.clear();
                slist64path.clear();
                slist64.clear();
            }
        }
    }
    return false;
}

bool FormSearchControl::setList64(QTextStream& stream)
{
    slist64fnam.clear();
    slist64path.clear();
    slist64.clear();

    while (!stream.atEnd())
    {
        QByteArray line = stream.readLine().toLocal8Bit();
        uint64_t s = 0;
        if (sscanf(line.data(), "%" PRId64, (int64_t*)&s) == 1)
            slist64.push_back(s);
    }

    if (!slist64.empty())
        updateSearchProgress(0, slist64.size(), slist64[0]);

    return true;
}

void FormSearchControl::setResultsPath(QString path)
{
    resultfile.close();
    resultfile.setFileName(path);
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
        int type = ui->comboSearchType->currentData().toInt();
        ui->buttonMore->setEnabled(type == SEARCH_INC || type == SEARCH_LIST);
    }
    emit searchStatusChanged(lock);
}

void FormSearchControl::setSearchMode(int mode)
{
    ui->comboSearchType->setCurrentIndex(ui->comboSearchType->findData(mode));
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

void FormSearchControl::setSearchRange(uint64_t smin, uint64_t smax)
{
    this->smin = smin;
    this->smax = smax;
    searchProgressReset();
}

bool FormSearchControl::getSeed(int row, uint64_t *seed)
{
    QAbstractItemModel *model = ui->results->model();
    if (row < 0 || row >= model->rowCount())
        return false;
    QModelIndex idx = model->index(row, SeedTableModel::COL_SEED);
    *seed = model->data(idx, Qt::UserRole).toULongLong();
    return true;
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
        Session session;
        parent->getSeed(&session.wi);
        session.cv = parent->formCond->getConditions();
        session.sc = getSearchConfig();
        int ok = true;

        if (session.cv.empty())
        {
            warn(this, tr("Please define some constraints using the \"Add\" button."));
            ok = false;
        }
        if (session.sc.searchtype == SEARCH_LIST && slist64.empty())
        {
            warn(this, tr("No seed list file selected."));
            ok = false;
        }
        if (ok)
        {
            session.gen48 = parent->formGen48->getConfig(true);
            // the search can either use a full list or a 48-bit list
            if (session.sc.searchtype == SEARCH_LIST)
                session.slist = slist64;
            else if (session.gen48.mode == GEN48_LIST)
                session.slist = parent->formGen48->getList48();
            else
                session.slist.clear();

            ok = sthread.set(parent, session);
        }

        if (ok)
        {
            if (!resultfile.fileName().isEmpty())
            {
                resultfile.close();
                resultfile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);
                QTextStream stream(&resultfile);
                session.writeHeader(stream);
            }
            ui->lineStart->setText(QString::asprintf("%" PRId64, (int64_t)session.sc.startseed));
            ui->buttonStart->setText(tr("Abort search"));
            ui->buttonStart->setIcon(QIcon(":/icons/cancel.png"));
            searchLockUi(true);
            nextupdate = 0;
            updt = 20;
            sthread.startSearch();
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
    int type = ui->comboSearchType->currentData().toInt();
    if (type == SEARCH_LIST)
    {
        QString filter = tr("Text files (*.txt);;Any files (*)");
#if WASM
        auto fileOpenCompleted = [=](const QString &fnam, const QByteArray &content) {
            if (!fnam.isEmpty()) {
                QTextStream stream(content);
                setList64(stream);
            }
        };
        QFileDialog::getOpenFileContent(filter, fileOpenCompleted);
#else
        QString fnam = QFileDialog::getOpenFileName(this, tr("Load seed list"), parent->prevdir, filter);
        setList64(fnam, false);
#endif
    }
    else if (type == SEARCH_INC)
    {
        RangeDialog *dialog = new RangeDialog(this, smin, smax);
        connect(dialog, &RangeDialog::applyBounds, this, &FormSearchControl::setSearchRange);
        dialog->show();
    }
}

void FormSearchControl::onSeedSelectionChanged()
{
    uint64_t s;
    if (getSeed(ui->results->currentIndex().row(), &s))
        emit selectedSeedChanged(s);
}

void FormSearchControl::on_results_clicked(const QModelIndex &)
{
    onSeedSelectionChanged();
}

void FormSearchControl::on_results_customContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = new QMenu(this);

    // this is a contextual temporary menu so shortcuts are only indicated here,
    // but will not function - see keyReleaseEvent() for shortcut implementation

    QAction *actremove = menu->addAction(QIcon::fromTheme("list-remove"),
        tr("Remove selected seed"), this,
        &FormSearchControl::removeCurrent, QKeySequence::Delete);
    actremove->setEnabled(ui->results->selectionModel()->hasSelection());

    QAction *actcopyseed = menu->addAction(QIcon::fromTheme("edit-copy"),
        tr("Copy selected seed"), this,
        &FormSearchControl::copySeed, QKeySequence::Copy);
    actcopyseed->setEnabled(ui->results->selectionModel()->hasSelection());

    QAction *actcopylist = menu->addAction(QIcon::fromTheme("edit-copy"),
        tr("Copy seed list"), this,
        &FormSearchControl::copyResults, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    actcopylist->setEnabled(ui->results->model()->rowCount() > 0);

    int n = pasteList(true);
    QAction *actpaste = menu->addAction(QIcon::fromTheme("edit-paste"),
        tr("Paste %n seed(s) from clipboard", "", n), this,
        &FormSearchControl::pasteResults, QKeySequence::Paste);
    actpaste->setEnabled(n > 0);
    menu->popup(ui->results->mapToGlobal(pos));
}

void FormSearchControl::on_buttonSearchHelp_clicked()
{
    QMessageBox *mb = new QMessageBox(this);
    mb->setIcon(QMessageBox::Information);
    mb->setWindowTitle(tr("Help: search types"));
    mb->setText(tr(
        "<html><head/><body><p>"
        "The <b>incremental</b> search checks seeds in numerical order, "
        "except for grouping seeds into work items for parallelization. "
        "This is the recommended option for general searches. You can "
        "restrict this type of search to a value range using the "
        "&quot;...&quot; button."
        "</p><p>"
        "When using <b>48-bit only</b>, the search checks partial seeds and "
        "will not test the full conditions. Instead it yields seed bases "
        "that may satify the conditions without knowing the upper 16-bit of "
        "the seed. A session file saved from this search is suitable to be used "
        "later with the 48-bit generator to look for matching seeds."
        "</p><p>"
        "With <b>48-bit family blocks</b> the search looks for suitable "
        "48-bit seeds first and parallelizes the search through the upper "
        "16-bits. This search type can be a better match for exhaustive searches "
        "and those with very restrictive structure requirements."
        "</p><p>"
        "Load a <b>seed list from a file</b> to search through an "
        "existing set of seeds. The seeds should be in decimal ASCII text, "
        "separated by newline characters. You can browse for a file using "
        "the &quot;...&quot; button. (The seed generator is ignored with "
        "this option.)"
        "</p></body></html>"
        ));
    mb->show();
}

void FormSearchControl::on_comboSearchType_currentIndexChanged(int)
{
    int type = ui->comboSearchType->currentData().toInt();
    ui->buttonMore->setEnabled(type == SEARCH_INC || type == SEARCH_LIST);
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
    std::vector<uint64_t> seeds;

    for (QString s : qAsConst(slist))
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

void FormSearchControl::searchResult(uint64_t seed)
{
    if (resultfile.isOpen())
    {
        char s[32];
        snprintf(s, sizeof(s), "%" PRId64 "\n", seed);
        resultfile.write(s);
        resultfile.flush();
    }

    qbuf.push_back(seed);
    if (ui->checkStop->isChecked())
    {
        searchResultsAdd(qbuf, false);
        qbuf.clear();
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

int FormSearchControl::searchResultsAdd(std::vector<uint64_t> seeds, bool countonly)
{
    if (seeds.empty())
        return 0;
    const Config& config = parent->config;
    int n = model->seeds.size();
    int nold = n;
    bool discarded = false;

    if (n >= config.maxMatching)
    {
        sthread.stopSearch();
        discarded = true;
    }
    if (n + (ssize_t)seeds.size() > config.maxMatching)
    {
        sthread.stopSearch();
        discarded = true;
        seeds.resize(config.maxMatching - n);
    }

    QSet<uint64_t> current;
    current.reserve(n + seeds.size());
    for (int i = 0; i < n; i++)
        current.insert(model->seeds[i].seed);

    QVector<uint64_t> newseeds;
    for (uint64_t s : seeds)
    {
        if (current.contains(s))
            continue;
        if (!countonly)
        {
            current.insert(s);
            newseeds.append(s);
        }
        n++;
    }
    if (!newseeds.empty())
    {
        ui->results->setSortingEnabled(false);
        model->insertSeeds(newseeds);
        ui->results->setSortingEnabled(true);
    }

    int addcnt = n - nold;
    if (ui->checkStop->isChecked() && addcnt)
        sthread.stopSearch();

    if (!countonly && discarded)
    {
        // guard against recursive calls, which can occur when the
        // eventloop is continued in the warning message dialog
        thread_local bool warn_active = false;
        if (!warn_active)
        {
            warn_active = true;
            warn(this, tr("Maximum number of results reached (%1).").arg(config.maxMatching));
            warn_active = false;
        }
    }

    if (addcnt)
        emit resultsAdded(addcnt);

    return addcnt;
}

void FormSearchControl::searchProgressReset()
{
    uint64_t cnt = parent->formGen48->estimateSeedCnt();
    if (cnt > MASK48)
        cnt = ~(uint64_t)0;
    else
        cnt <<= 16;

    QString fmt;
    int searchtype = ui->comboSearchType->currentData().toInt();
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

    if (parent)
        parent->setProgressIndication();
}

void FormSearchControl::updateSearchProgress(uint64_t prog, uint64_t end, int64_t seed)
{
    ui->lineStart->setText(QString::asprintf("%" PRId64, seed));

    if (!end)
        return;

    double value = (double)prog / end;
    int v = (int) floor(10000 * value);
    ui->progressBar->setValue(v);
    QString fmt = QString::asprintf(
                "%" PRIu64 " / %" PRIu64 " (%d.%02d%%)",
                prog, end, v / 100, v % 100
                );
    int searchtype = ui->comboSearchType->currentData().toInt();
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

    if (parent)
        parent->setProgressIndication(value);
}

void FormSearchControl::searchFinish(bool done)
{
    stimer.stop();
    onBufferTimeout();
    progressTimeout();
    if (done)
    {
        ui->lineStart->setText(QString::asprintf("%" PRId64, sthread.smax));
        ui->progressBar->setValue(10000);
        ui->progressBar->setFormat(tr("Done", "Progressbar"));
    }
    ui->labelStatus->setText(tr("Idle", "Progressbar"));
    searchLockUi(false);

    if (parent)
        parent->setProgressIndication();
}

void FormSearchControl::progressTimeout()
{
    uint64_t prog, end, seed;
    qreal min, avg, max;
    QString status = tr("Running...", "Progressbar");
    if (!sthread.getProgress(&status, &prog, &end, &seed, &min, &avg, &max))
        return;

    updateSearchProgress(prog, end, seed);

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

void FormSearchControl::copySeed()
{
    uint64_t seed;
    if (getSeed(ui->results->currentIndex().row(), &seed))
    {
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(QString::asprintf("%" PRId64, seed));
    }
}

void FormSearchControl::copyResults()
{
    QString text;
    int n = ui->results->model()->rowCount();
    for (int i = 0; i < n; i++)
    {
        uint64_t seed;
        if (getSeed(i, &seed))
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
            copySeed();
        else if ((event->modifiers() & Qt::CTRL) && (event->modifiers() & Qt::SHIFT) && event->key() == Qt::Key_C)
            copyResults();
        else if (event->matches(QKeySequence::Paste))
            pasteResults();
    }
    QWidget::keyReleaseEvent(event);
}
