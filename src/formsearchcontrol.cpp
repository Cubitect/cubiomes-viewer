#include "formsearchcontrol.h"
#include "ui_formsearchcontrol.h"

#include "mainwindow.h"
#include "search.h"

#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>


FormSearchControl::FormSearchControl(MainWindow *parent)
    : QWidget(parent)
    , parent(parent)
    , ui(new Ui::FormSearchControl)
    , sthread(this)
    , stimer()
    , slist64path()
    , slist64()
{
    ui->setupUi(this);

    QFont mono = QFont("Monospace", 9);
    mono.setStyleHint(QFont::TypeWriter);
    ui->listResults->setFont(mono);
    ui->progressBar->setFont(mono);

    //connect(&sthread, &SearchThread::results, this, &MainWindow::searchResultsAdd, Qt::BlockingQueuedConnection);
    connect(&sthread, &SearchThread::progress, this, &FormSearchControl::searchProgress, Qt::QueuedConnection);
    connect(&sthread, &SearchThread::searchFinish, this, &FormSearchControl::searchFinish, Qt::QueuedConnection);

    connect(&stimer, &QTimer::timeout, this, QOverload<>::of(&FormSearchControl::resultTimeout));
    stimer.start(500);

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

QVector<int64_t> FormSearchControl::getResults()
{
    int n = ui->listResults->rowCount();
    QVector<int64_t> results = QVector<int64_t>(n);
    for (int i = 0; i < n; i++)
    {
        results[i] = ui->listResults->item(i, 0)->data(Qt::UserRole).toLongLong();
    }
    return results;
}

SearchConfig FormSearchControl::getSearchConfig()
{
    SearchConfig s;
    s.searchmode = ui->comboSearchType->currentIndex();
    s.threads = ui->spinThreads->value();
    s.slist64path = slist64path;
    s.startseed = ui->lineStart->text().toLongLong();
    s.stoponres = ui->checkStop->isChecked();
    return s;
}

bool FormSearchControl::setSearchConfig(SearchConfig s, bool quiet)
{
    bool ok = true;
    if (s.searchmode >= SEARCH_INC && s.searchmode <= SEARCH_LIST)
        ui->comboSearchType->setCurrentIndex(s.searchmode);
    else
        ok = false;

    ui->spinThreads->setValue(s.threads);
    ui->lineStart->setText(QString::asprintf("%" PRId64, s.startseed));
    ui->checkStop->setChecked(s.stoponres);

    return ok && setList64(s.slist64path, quiet);
}

bool FormSearchControl::isbusy()
{
    return ui->buttonStart->isChecked() || sthread.isRunning();
}

bool FormSearchControl::setList64(QString path, bool quiet)
{
    if (!path.isEmpty())
    {
        QFileInfo finfo(path);
        parent->prevdir = finfo.absolutePath();
        slist64path = finfo.fileName();
        int64_t *l = NULL;
        int64_t len;
        QByteArray ba = path.toLatin1();
        l = loadSavedSeeds(ba.data(), &len);
        if (l && len > 0)
        {
            slist64.assign(l, l+len);
            searchProgress(0, len, l[0]);
            free(l);
            return true;
        }
        else
        {
            if (!quiet)
                QMessageBox::warning(this, "Warning", "Failed to load seed list from file", QMessageBox::Ok);
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
    }
    else
    {
        ui->buttonStart->setText("Start search");
        ui->buttonStart->setIcon(QIcon(":/icons/search.png"));
        ui->buttonStart->setChecked(false);
        ui->buttonStart->setEnabled(true);
        ui->comboSearchType->setEnabled(true);
        ui->spinThreads->setEnabled(true);
    }
    emit searchStatusChanged(lock);
}

void FormSearchControl::setSearchMode(int mode)
{
    ui->comboSearchType->setCurrentIndex(mode);
    if (mode == SEARCH_LIST)
    {
        on_buttonLoadList_clicked();
    }
    else
    {
        slist64.clear();
        slist64path.clear();
    }
}


void FormSearchControl::on_buttonClear_clicked()
{
    ui->listResults->clearContents();
    ui->listResults->setRowCount(0);
    searchProgressReset();
}

void FormSearchControl::on_buttonStart_clicked()
{
    if (ui->buttonStart->isChecked())
    {
        int mc = MC_1_16;
        parent->getSeed(&mc, NULL);
        const Config& config = parent->config;
        const QVector<Condition>& condvec = parent->formCond->getConditions();
        int64_t sstart = (int64_t) ui->lineStart->text().toLongLong();
        int searchtype = ui->comboSearchType->currentIndex();
        int threads = ui->spinThreads->value();
        int ok = true;

        if (condvec.empty())
        {
            QMessageBox::warning(this, "Warning", "Please define some constraints using the \"Add\" button.", QMessageBox::Ok);
            ok = false;
        }
        if (searchtype == SEARCH_LIST && slist64.empty())
        {
            QMessageBox::warning(this, "Warning", "No seed list file selected.", QMessageBox::Ok);
            ok = false;
        }
        if (sthread.isRunning())
        {
            QMessageBox::warning(this, "Warning", "Search is still running.", QMessageBox::Ok);
            ok = false;
        }

        if (ok)
        {
            Gen48Settings gen48 = parent->formGen48->getSettings(true);
            // the search can either use a full list or a 48-bit list
            if (searchtype == SEARCH_LIST)
                slist = slist64;
            else if (gen48.mode == GEN48_LIST)
                slist = parent->formGen48->getList48();
            else
                slist.clear();

            ok = sthread.set(parent, searchtype, threads, gen48, slist, sstart, mc, condvec, config.seedsPerItem, config.queueSize);
        }

        if (ok)
        {
            ui->lineStart->setText(QString::asprintf("%" PRId64, sstart));
            ui->buttonStart->setText("Abort search");
            ui->buttonStart->setIcon(QIcon(":/icons/cancel.png"));
            sthread.start();
            searchLockUi(true);
        }
        else
        {
            ui->buttonStart->setChecked(false);
        }
    }
    else
    {
        sthread.stop(); // tell search to stop at next convenience
        //sthread.quit(); // tell the event loop to exit
        //sthread.wait(); // wait for search to finish
        ui->buttonStart->setText("Start search");
        ui->buttonStart->setIcon(QIcon(":/icons/search.png"));
        ui->buttonStart->setChecked(false);

        // disable until finish
        ui->buttonStart->setEnabled(false);
    }

    update();
}

void FormSearchControl::on_buttonLoadList_clicked()
{
    QString fnam = QFileDialog::getOpenFileName(this, "Load seed list", parent->prevdir, "Text files (*.txt);;Any files (*)");
    setList64(fnam, false);
}

void FormSearchControl::on_listResults_itemSelectionChanged()
{
    int row = ui->listResults->currentRow();
    if (row >= 0 && row < ui->listResults->rowCount())
    {
        int64_t s = ui->listResults->item(row, 0)->data(Qt::UserRole).toLongLong();
        emit selectedSeedChanged(s);
    }
}

void FormSearchControl::on_listResults_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);

    QAction *actremove = menu.addAction(QIcon::fromTheme("list-remove"), "Remove selected seed", this, &FormSearchControl::removeCurrent);
    actremove->setEnabled(!ui->listResults->selectedItems().empty());

    QAction *actcopy = menu.addAction(QIcon::fromTheme("edit-copy"), "Copy list to clipboard", this, &FormSearchControl::copyResults);
    actcopy->setEnabled(ui->listResults->rowCount() > 0);

    int n = pasteList(true);
    QAction *actpaste = menu.addAction(QIcon::fromTheme("edit-paste"), QString::asprintf("Paste %d seeds from clipboard", n), this, &FormSearchControl::pasteResults);
    actpaste->setEnabled(n > 0);
    menu.exec(ui->listResults->mapToGlobal(pos));
}

void FormSearchControl::on_buttonSearchHelp_clicked()
{
    const char* msg =
            "<html><head/><body><p>"
            "The <b>incremental</b> search checks seeds in numerical order, "
            "save for grouping into work items for parallelization. This type "
            "of search is best suited for a non-exhaustive search space and "
            "with strong biome dependencies."
            "</p><p>"
            "With <b>48-bit family blocks</b> the search looks for suitable "
            "48-bit seeds first and parallelizes the search through the upper "
            "16-bits. This type of search is best suited for exhaustive "
            "searches and for many types of structure restrictions."
            "</p><p>"
            "Load a <b>seed list from a file</b> to search through an "
            "existing set of seeds. The seeds should be in decimal ASCII text, "
            "separated by newline characters. You can browse for a file using "
            "the &quot;...&quot; button. (The seed generator is ignored with "
            "this option.)"
            "</p></body></html>"
            ;
    QMessageBox::information(this, "Help: search types", msg, QMessageBox::Ok);
}

void FormSearchControl::on_comboSearchType_currentIndexChanged(int index)
{
    ui->buttonLoadList->setEnabled(index == SEARCH_LIST);
}

void FormSearchControl::pasteResults()
{
    pasteList(false);
}

int FormSearchControl::pasteList(bool dummy)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QStringList slist = clipboard->text().split('\n');
    QVector<int64_t> seeds;

    for (QString s : slist)
    {
        s = s.trimmed();
        if (s.isEmpty())
            continue;
        bool ok = true;
        int64_t seed = s.toLongLong(&ok);
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


int FormSearchControl::searchResultsAdd(QVector<int64_t> seeds, bool countonly)
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

    QSet<int64_t> current;
    current.reserve(n + seeds.size());
    for (int i = 0; i < n; i++)
    {
        int64_t seed = ui->listResults->item(i, 0)->data(Qt::UserRole).toLongLong();
        current.insert(seed);
    }

    ui->listResults->setSortingEnabled(false);
    for (int64_t s : seeds)
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
        seeditem->setData(Qt::DisplayRole, QVariant::fromValue(s));
        ui->listResults->insertRow(n);
        ui->listResults->setItem(n, 0, s48item);
        ui->listResults->setItem(n, 1, seeditem);
        n++;
    }
    ui->listResults->setSortingEnabled(true);

    if (countonly == false && n >= config.maxMatching)
    {
        sthread.stop();
        QString msg = QString::asprintf("Maximum number of results reached (%d).", config.maxMatching);
        QMessageBox::warning(this, "Warning", msg, QMessageBox::Ok);
    }

    int addcnt = n - ns;
    if (ui->checkStop->isChecked() && addcnt)
    {
        sthread.reqstop = true;
        sthread.pool.clear();
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
    QString fmt = QString::asprintf("0 / %" PRIu64 " (0.00%%)", cnt);
    if (!slist64path.isEmpty() && ui->comboSearchType->currentIndex() == SEARCH_LIST)
        fmt = slist64path + ": " + fmt;

    ui->lineStart->setText("0");
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat(fmt);
}

void FormSearchControl::searchProgress(uint64_t last, uint64_t end, int64_t seed)
{
//    if (sthread.itemgen.searchtype == SEARCH_BLOCKS)
//        seed &= MASK48;
    ui->lineStart->setText(QString::asprintf("%" PRId64, seed));

    if (end)
    {
        int v = (int) floor(10000 * (double)last / end);
        ui->progressBar->setValue(v);
        QString fmt = QString::asprintf(
                    "%" PRIu64 " / %" PRIu64 " (%d.%02d%%)", last, end, v / 100, v % 100);
        if (!slist64path.isEmpty() && ui->comboSearchType->currentIndex() == SEARCH_LIST)
            fmt = slist64path + ": " + fmt;
        ui->progressBar->setFormat(fmt);
    }
}

void FormSearchControl::searchFinish()
{
    if (!sthread.abort)
    {
        searchProgress(0, 0, sthread.itemgen.seed);
    }
    if (sthread.itemgen.isdone)
    {
        ui->progressBar->setValue(10000);
        ui->progressBar->setFormat(QString::asprintf("Done"));
    }
    searchLockUi(false);
}

void FormSearchControl::resultTimeout()
{
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
        int64_t seed = ui->listResults->item(i, 0)->data(Qt::UserRole).toLongLong();
        text += QString::asprintf("%" PRId64 "\n", seed);
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
}
