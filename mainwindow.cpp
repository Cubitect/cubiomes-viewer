#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filterdialog.h"
#include "gotodialog.h"
#include "quadlistdialog.h"
#include "aboutdialog.h"
#include "quad.h"
#include "cutil.h"

#include <QIntValidator>
#include <QMetaType>
#include <QMessageBox>
#include <QtDebug>
#include <QDataStream>
#include <QMenu>
#include <QClipboard>
#include <QFont>
#include <QFileDialog>
#include <QTextStream>

#include <stdlib.h>

#define MAXRESULTS 65535


QDataStream& operator<<(QDataStream& out, const Condition& v)
{
    out.writeRawData((const char*)&v, sizeof(Condition));
    return out;
}

QDataStream& operator>>(QDataStream& in, Condition& v)
{
    in.readRawData((char*)&v, sizeof(Condition));
    return in;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sthread(this),
    stimer(this)
{
    ui->setupUi(this);
    ui->frameMap->layout()->addWidget(ui->toolBar);
    ui->toolBar->setContentsMargins(0, 0, 0, 0);

    ui->splitterMap->setSizes(QList<int>({8000, 10000}));
    ui->splitterSearch->setSizes(QList<int>({1000, 2000}));
    ui->splitterConditions->setSizes(QList<int>({1000, 3000}));

    QFont mono = QFont("Monospace", 9);
    mono.setStyleHint(QFont::TypeWriter);
    ui->listConditions48->setFont(mono);
    ui->listConditionsFull->setFont(mono);
    ui->listResults->setFont(mono);

    qRegisterMetaType< int64_t >("int64_t");
    qRegisterMetaType< QVector<int64_t> >("QVector<int64_t>");
    qRegisterMetaType< Condition >("Condition");
    qRegisterMetaTypeStreamOperators< Condition >("Condition");

    protodialog = new ProtoBaseDialog(this);

    connect(&sthread, &SearchThread::results, this, &MainWindow::searchResultsAdd, Qt::BlockingQueuedConnection);
    connect(&sthread, &SearchThread::baseDone, this, &MainWindow::searchBaseDone, Qt::BlockingQueuedConnection);
    connect(&sthread, &SearchThread::finish, this, &MainWindow::searchFinish);
    connect(ui->checkStop, &QAbstractButton::toggled, &sthread, &SearchThread::setStopOnResult, Qt::DirectConnection);
    sthread.setStopOnResult(ui->checkStop->isChecked());

    connect(&stimer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::resultTimeout));
    stimer.start(500);

    updateSensitivity();

    prevdir = ".";
}

MainWindow::~MainWindow()
{
    stimer.stop();
    sthread.stop(); // tell search to stop at next convenience
    sthread.quit(); // tell the event loop to exit
    sthread.wait(); // wait for search to finish
    delete ui;
}

QVector<Condition> MainWindow::getConditions() const
{
    QVector<Condition> conds;

    for (int i = 0, ie = ui->listConditions48->count(); i < ie; i++)
        conds.push_back(qvariant_cast<Condition>(ui->listConditions48->item(i)->data(Qt::UserRole)));

    for (int i = 0, ie = ui->listConditionsFull->count(); i < ie; i++)
        conds.push_back(qvariant_cast<Condition>(ui->listConditionsFull->item(i)->data(Qt::UserRole)));

    return conds;
}

MapView* MainWindow::getMapView()
{
    return ui->mapView;
}

bool MainWindow::getSeed(int *mc, int64_t *seed, bool applyrand)
{
    bool ok = true;
    if (mc)
    {
        const std::string& mcs = ui->comboBoxMC->currentText().toStdString();
        *mc = str2mc(mcs.c_str());
        if (*mc < 0)
        {
            *mc = MC_1_16;
            qDebug() << "Unknown MC version: " << *mc;
            ok = false;
        }
    }

    if (seed)
    {
        const QByteArray& ba = ui->seedEdit->text().toLocal8Bit();
        int v = str2seed(ba.data(), seed);
        if (applyrand && v == S_RANDOM)
            ui->seedEdit->setText(QString::asprintf("%" PRId64, *seed));
    }

    return ok;
}

bool MainWindow::setSeed(int mc, int64_t seed)
{
    const char *mcstr = mc2str(mc);
    if (!mcstr)
    {
        qDebug() << "Unknown MC version: " << mc;
        return false;
    }

    ui->comboBoxMC->setCurrentText(mcstr);
    ui->seedEdit->setText(QString::asprintf("%" PRId64, seed));
    ui->mapView->setSeed(mc, seed);
    return true;
}

// [ID] Condition                   Cnt Rel  Area
void MainWindow::setItemCondition(QListWidgetItem *item, Condition cond) const
{
    item->setData(Qt::UserRole, QVariant::fromValue(cond));

    const FilterInfo& ft = g_filterinfo.list[cond.type];
    QString s = QString::asprintf("[%02d] %-28sx%-3d", cond.save, ft.name, cond.count);

    if (cond.relative)
        s += QString::asprintf("[%02d]+", cond.relative);
    else
        s += "     ";

    if (ft.coord)
        s += QString::asprintf("(%d,%d)", cond.x1*ft.step, cond.z1*ft.step);
    if (ft.area)
        s += QString::asprintf(",(%d,%d)", (cond.x2+1)*ft.step-1, (cond.z2+1)*ft.step-1);

    if (ft.cat == CAT_48)
        item->setBackground(QColor(Qt::yellow));
    else if (ft.cat == CAT_FULL)
        item->setBackground(QColor(Qt::green));

    item->setText(s);
}

void MainWindow::editCondition(QListWidgetItem *item)
{
    FilterDialog *dialog = new FilterDialog(this, item, (Condition*)item->data(Qt::UserRole).data());
    QObject::connect(dialog, SIGNAL(setCond(QListWidgetItem*,Condition)), this, SLOT(addItemCondition(QListWidgetItem*,Condition)), Qt::QueuedConnection);
    dialog->exec();
}

void MainWindow::updateMapSeed()
{
    int mc;
    int64_t seed;
    if (getSeed(&mc, &seed))
        ui->mapView->setSeed(mc, seed);
}

void MainWindow::updateSensitivity()
{
    int selectcnt = 0;
    selectcnt += ui->listConditions48->selectedItems().size();
    selectcnt += ui->listConditionsFull->selectedItems().size();

    if (selectcnt == 0)
    {
        ui->buttonRemove->setEnabled(false);
        ui->buttonEdit->setEnabled(false);
    }
    else if (selectcnt == 1)
    {
        ui->buttonRemove->setEnabled(true);
        ui->buttonEdit->setEnabled(true);
    }
    else
    {
        ui->buttonRemove->setEnabled(true);
        ui->buttonEdit->setEnabled(false);
    }
}

int MainWindow::getIndex(int idx) const
{
    const QVector<Condition> condvec = getConditions();
    int cnt[100] = {};
    for (const Condition& c : condvec)
        if (c.save > 0 || c.save < 100)
            cnt[c.save]++;
        else return 0;
    if (idx <= 0)
        idx = 1;
    if (cnt[idx] == 0)
        return idx;
    for (int i = 1; i < 100; i++)
        if (cnt[i] == 0)
            return i;
    return 0;
}


void MainWindow::warning(QString title, QString text)
{
    QMessageBox::warning(this, title, text, QMessageBox::Ok);
}

void MainWindow::mapGoto(qreal x, qreal z)
{
    ui->mapView->setView(x, z);
}

void MainWindow::openProtobaseMsg(QString path)
{
    protodialog->setPath(path);
    protodialog->show();
}

void MainWindow::closeProtobaseMsg()
{
    if (protodialog->closeOnDone())
        protodialog->close();
}

void MainWindow::on_comboBoxMC_currentIndexChanged(int)
{
    updateMapSeed();
    update();
}

void MainWindow::on_seedEdit_editingFinished()
{
    updateMapSeed();
    update();
}

void MainWindow::on_seedEdit_textChanged(const QString &a)
{
    int64_t s;
    int v = str2seed(a.toLocal8Bit().data(), &s);
    switch (v)
    {
        case 0: ui->labelSeedType->setText("(text)"); break;
        case 1: ui->labelSeedType->setText("(numeric)"); break;
        case 2: ui->labelSeedType->setText("(random)"); break;
    }
}

void MainWindow::on_actionDesert_toggled(bool show)
{ ui->mapView->setShow(D_DESERT, show); }

void MainWindow::on_actionJungle_toggled(bool show)
{ ui->mapView->setShow(D_JUNGLE, show); }

void MainWindow::on_actionIgloo_toggled(bool show)
{ ui->mapView->setShow(D_IGLOO, show); }

void MainWindow::on_actionHut_toggled(bool show)
{ ui->mapView->setShow(D_HUT, show); }

void MainWindow::on_actionMonument_toggled(bool show)
{ ui->mapView->setShow(D_MONUMENT, show); }

void MainWindow::on_actionVillage_toggled(bool show)
{ ui->mapView->setShow(D_VILLAGE, show); }

void MainWindow::on_actionRuin_toggled(bool show)
{ ui->mapView->setShow(D_RUINS, show); }

void MainWindow::on_actionShipwreck_toggled(bool show)
{ ui->mapView->setShow(D_SHIPWRECK, show); }

void MainWindow::on_actionMansion_toggled(bool show)
{ ui->mapView->setShow(D_MANSION, show); }

void MainWindow::on_actionOutpost_toggled(bool show)
{ ui->mapView->setShow(D_OUTPOST, show); }

void MainWindow::on_actionPortal_toggled(bool show)
{ ui->mapView->setShow(D_PORTAL, show); }

void MainWindow::on_actionSpawn_toggled(bool show)
{ ui->mapView->setShow(D_SPAWN, show); }

void MainWindow::on_actionStronghold_toggled(bool show)
{ ui->mapView->setShow(D_STRONGHOLD, show); }


static void remove_selected(QListWidget *list)
{
    QList<QListWidgetItem*> selected = list->selectedItems();
    for (QListWidgetItem *item : selected)
    {
        list->takeItem(list->row(item));
        delete item;
    }
}

void MainWindow::on_buttonRemoveAll_clicked()
{
    ui->listConditions48->clear();
    ui->listConditionsFull->clear();
}

void MainWindow::on_buttonRemove_clicked()
{
    remove_selected(ui->listConditions48);
    remove_selected(ui->listConditionsFull);
}

void MainWindow::on_buttonEdit_clicked()
{
    QListWidget *list = 0;
    QListWidgetItem* item = 0;
    QList<QListWidgetItem*> selected;

    list = ui->listConditions48;
    selected = list->selectedItems();
    if (!selected.empty())
    {
        item = selected.first();
        list->takeItem(list->row(item));
    }
    else
    {
        list = ui->listConditionsFull;
        selected = list->selectedItems();
        if (!selected.empty())
        {
            item = selected.first();
            list->takeItem(list->row(item));
        }
    }

    if (item)
        editCondition(item);
}

void MainWindow::on_buttonAddFilter_clicked()
{
    FilterDialog *dialog = new FilterDialog(this);
    QObject::connect(dialog, SIGNAL(setCond(QListWidgetItem*,Condition)), this, SLOT(addItemCondition(QListWidgetItem*,Condition)), Qt::QueuedConnection);
    dialog->show();
}


void MainWindow::on_listConditions48_itemDoubleClicked(QListWidgetItem *item)
{
    ui->listConditions48->takeItem(ui->listConditions48->row(item));
    editCondition(item);
}

void MainWindow::on_listConditionsFull_itemDoubleClicked(QListWidgetItem *item)
{
    ui->listConditionsFull->takeItem(ui->listConditionsFull->row(item));
    editCondition(item);
}

void MainWindow::on_listConditions48_itemSelectionChanged()
{
    updateSensitivity();
}

void MainWindow::on_listConditionsFull_itemSelectionChanged()
{
    updateSensitivity();
}

void MainWindow::on_buttonClear_clicked()
{
    ui->listResults->clearContents();
    ui->listResults->setRowCount(0);
    ui->lineStart48->setText("0");
    ui->progressBar->setValue(0);
    ui->progressBar->setFormat("0.00%");
}

void MainWindow::on_buttonStart_clicked()
{
    if (ui->buttonStart->isChecked())
    {
        int mc = MC_1_16;
        getSeed(&mc, NULL);
        QVector<Condition> condvec = getConditions();
        int64_t sstart = (int64_t)ui->lineStart48->text().toLongLong() & MASK48;
        int searchtype = ui->comboSearchType->currentIndex();
        int ok = true;

        if (condvec.empty())
        {
            warning("Warning", "Please define some constraints using the \"Add\" button.");
            ok = false;
        }
        if (sthread.isRunning())
        {
            warning("Warning", "Search is still running.");
            ok = false;
        }

        if (ok)
            ok = sthread.set(searchtype, sstart, mc, condvec);

        if (ok)
        {
            ui->lineStart48->setText(QString::asprintf("%" PRId64, sstart));
            ui->comboSearchType->setEnabled(false);
            ui->buttonStart->setText("Abort search");
            ui->buttonStart->setIcon(QIcon::fromTheme("process-stop"));
            sthread.start();
        }
        else
        {
            ui->buttonStart->setChecked(false);
        }
    }
    else
    {
        sthread.stop(); // tell search to stop at next convenience
        sthread.quit(); // tell the event loop to exit
        //sthread.wait(); // wait for search to finish
        ui->buttonStart->setEnabled(true);
        ui->buttonStart->setText("Start search");
        ui->buttonStart->setIcon(QIcon::fromTheme("system-search"));
        ui->comboSearchType->setEnabled(true);
    }

    update();
}


void MainWindow::on_listResults_itemSelectionChanged()
{
    int row = ui->listResults->currentRow();
    if (row >= 0 && row < ui->listResults->rowCount())
    {
        int64_t s = ui->listResults->item(row, 0)->data(Qt::UserRole).toLongLong();
        ui->seedEdit->setText(QString::asprintf("%" PRId64, s));
        on_seedEdit_editingFinished();
    }
}

void MainWindow::on_listResults_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);

    QAction *actremove = menu.addAction(QIcon::fromTheme("list-remove"), "Remove selected seed", this, &MainWindow::removeCurrent);
    actremove->setEnabled(!ui->listResults->selectedItems().empty());

    QAction *actcopy = menu.addAction(QIcon::fromTheme("edit-copy"), "Copy list to clipboard", this, &MainWindow::copyResults);
    actcopy->setEnabled(ui->listResults->rowCount() > 0);

    int n = pasteList(true);
    QAction *actpaste = menu.addAction(QIcon::fromTheme("edit-paste"), QString::asprintf("Paste %d seeds from clipboard", n), this, &MainWindow::pasteResults);
    actpaste->setEnabled(n > 0);
    menu.exec(ui->listResults->mapToGlobal(pos));
}

void MainWindow::on_buttonInfo_clicked()
{
    const char* msg =
            "The constraints are separated into two sections. "
            "The first (top) list is used to select a 48-bit generator, "
            "while the second (bottom) list contains conditions that have a biome dependency."
            "\n\n"
            "Conditions can reference each other for relative positions "
            "(indicated with the ID in square brackets [XY]). "
            "The conditions will be checked in the same order they are listed, "
            "so make sure that references are not broken."
            "\n\n"
            "You can edit existing conditions by double-clicking, and use drag to reorder them. "
    ;
    QMessageBox::information(this, "Help: search conditions", msg, QMessageBox::Ok);
}


void MainWindow::on_actionSave_triggered()
{
    QString fnam = QFileDialog::getSaveFileName(this, "Save progress", prevdir, "Text files (*.txt);;Any files (*)");
    if (!fnam.isEmpty())
    {
        QFileInfo finfo(fnam);
        QFile file(fnam);
        prevdir = finfo.absolutePath();

        if (!file.open(QIODevice::WriteOnly))
        {
            warning("Warning", "Failed to open file.");
            return;
        }

        QTextStream stream(&file);
        stream << "#Version:  " << VERS_MAJOR << "." << VERS_MINOR << "." << VERS_PATCH << "\n";
        stream << "#Search:   " << ui->comboSearchType->currentIndex() << "\n";
        stream << "#Progress: " << ui->lineStart48->text().toLongLong() << "\n";
        QVector<Condition> condvec = getConditions();
        for (Condition &c : condvec)
            stream << "#Cond: " << QByteArray((const char*) &c, sizeof(Condition)).toHex() << "\n";

        int n = ui->listResults->rowCount();
        for (int i = 0; i < n; i++)
        {
            int64_t seed = ui->listResults->item(i, 0)->data(Qt::UserRole).toLongLong();
            stream << QString::asprintf("%" PRId64 "\n", seed);
        }
    }
}

void MainWindow::on_actionLoad_triggered()
{
    if (ui->buttonStart->isChecked() || sthread.isRunning())
    {
        warning("Warning", "Cannot load progress: search is still active.");
        return;
    }

    QString fnam = QFileDialog::getOpenFileName(this, "Load progress", prevdir, "Text files (*.txt);;Any files (*)");
    if (!fnam.isEmpty())
    {
        QFileInfo finfo(fnam);
        QFile file(fnam);
        prevdir = finfo.absolutePath();

        if (!file.open(QIODevice::ReadOnly))
        {
            warning("Warning", "Failed to open file.");
            return;
        }

        int major = 0, minor = 0, patch = 0;
        int searchtype = 0;
        int64_t s48 = 0;
        QVector<Condition> condvec;
        QVector<int64_t> seeds;

        QTextStream stream(&file);
        QString line;
        line = stream.readLine();
        if (sscanf(line.toLatin1().data(), "#Version: %d.%d.%d", &major, &minor, &patch) != 3)
            goto L_read_failed;
        if (cmpVers(major, minor, patch) > 0)
            warning("Warning", "Progress file was created with a newer version.");

        line = stream.readLine();
        if (sscanf(line.toLatin1().data(), "#Search: %d", &searchtype) != 1)
            goto L_read_failed;

        line = stream.readLine();
        if (sscanf(line.toLatin1().data(), "#Progress: %" PRId64, &s48) != 1)
            goto L_read_failed;

        while (stream.status() == QTextStream::Ok)
        {
            line = stream.readLine();
            if (line.isEmpty())
                break;
            if (line.startsWith("#Cond:"))
            {
                QString hex = line.mid(6).trimmed();
                QByteArray ba = QByteArray::fromHex(QByteArray(hex.toLatin1().data()));
                if (ba.size() == sizeof(Condition))
                {
                    Condition c = *(Condition*) ba.data();
                    condvec.push_back(c);
                }
                else goto L_read_failed;
            }
            else
            {
                int64_t seed;
                if (sscanf(line.toLatin1().data(), "%" PRId64, &seed) == 1)
                    seeds.push_back(seed);
                else goto L_read_failed;
            }
        }

        on_buttonRemoveAll_clicked();
        on_buttonClear_clicked();

        ui->comboSearchType->setCurrentIndex(searchtype);
        ui->lineStart48->setText(QString::asprintf("%" PRId64, s48));

        for (Condition &c : condvec)
        {
            QListWidgetItem *item = new QListWidgetItem();
            addItemCondition(item, c);
        }

        searchResultsAdd(seeds, false);

        return;
L_read_failed:
        warning("Warning", "Failed to parse progress file.");
    }
}

void MainWindow::on_actionGo_to_triggered()
{
    GotoDialog *dialog = new GotoDialog(this, ui->mapView->getX(), ui->mapView->getZ());
    dialog->show();
}

void MainWindow::on_actionScan_seed_for_Quad_Huts_triggered()
{
    QuadListDialog *dialog = new QuadListDialog(this);
    dialog->show();
}

void MainWindow::on_actionOpen_shadow_seed_triggered()
{
    int mc;
    int64_t seed;
    if (getSeed(&mc, &seed))
    {
        setSeed(mc, getShadow(seed));
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog *dialog = new AboutDialog(this);
    dialog->show();
}

void MainWindow::on_mapView_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction("Copy coordinates", this, &MainWindow::copyCoord);
    menu.addAction("Go to coordinates...", this, &MainWindow::on_actionGo_to_triggered);
    menu.exec(ui->mapView->mapToGlobal(pos));
}

void MainWindow::addItemCondition(QListWidgetItem *item, Condition cond)
{
    const FilterInfo& ft = g_filterinfo.list[cond.type];
    cond.save = getIndex(cond.save);

    if (ft.cat == CAT_FULL)
    {
        if (!item)
            item = new QListWidgetItem();
        setItemCondition(item, cond);
        ui->listConditionsFull->addItem(item);
    }
    else if (ft.cat == CAT_48)
    {
        if (item)
        {
            setItemCondition(item, cond);
            ui->listConditions48->addItem(item);
            return;
        }

        item = new QListWidgetItem();
        setItemCondition(item, cond);
        ui->listConditions48->addItem(item);

        int idx = getIndex(cond.save+1);

        if (cond.type >= F_QH_IDEAL && cond.type <= F_QH_BARELY)
        {
            Condition cq = cond;
            cq.type = F_HUT;
            cq.x1 = -128; cq.z1 = -128;
            cq.x2 = +128; cq.z2 = +128;
            cq.relative = cond.save;
            cq.save = idx;
            cq.count = 4;
            QListWidgetItem *item = new QListWidgetItem(ui->listConditionsFull, QListWidgetItem::UserType);
            setItemCondition(item, cq);
            ui->listConditionsFull->addItem(item);
        }
        else if (cond.type == F_QM_90 || cond.type == F_QM_95)
        {
            Condition cq = cond;
            cq.type = F_MONUMENT;
            cq.x1 = -160; cq.z1 = -160;
            cq.x2 = +160; cq.z2 = +160;
            cq.relative = cond.save;
            cq.save = idx;
            cq.count = 4;
            QListWidgetItem *item = new QListWidgetItem(ui->listConditionsFull, QListWidgetItem::UserType);
            setItemCondition(item, cq);
            ui->listConditionsFull->addItem(item);
        }
    }
}

int MainWindow::searchResultsAdd(QVector<int64_t> seeds, bool countonly)
{
    int ns = ui->listResults->rowCount();
    int n = ns;
    if (n >= MAXRESULTS)
        return 0;
    if (seeds.size() + n > MAXRESULTS)
        seeds.resize(MAXRESULTS - n);
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
                (qulonglong)(s & MASK48), (uint)(s >> 48) & ((1 << 16) - 1)));
        seeditem->setData(Qt::DisplayRole, QVariant::fromValue(s));
        ui->listResults->insertRow(n);
        ui->listResults->setItem(n, 0, s48item);
        ui->listResults->setItem(n, 1, seeditem);
        n++;
    }
    ui->listResults->setSortingEnabled(true);

    if (countonly == false && n >= MAXRESULTS)
    {
        sthread.stop();
        warning("Warning", QString::asprintf("Maximum number of results reached (%d).", MAXRESULTS));
    }

    if (ui->checkStop->isChecked())
        sthread.stop();

    return n - ns;
}

void MainWindow::searchBaseDone(int64_t s48)
{
    ui->lineStart48->setText(QString::asprintf("%" PRId64, s48 + 1));
    int v = (s48 * 10000) >> 48;
    if (ui->progressBar->value() != v)
    {
        ui->progressBar->setValue(v);
        ui->progressBar->setFormat(QString::asprintf("%d.%02d%%", v / 100, v % 100));
    }
}

void MainWindow::searchFinish(int64_t s48)
{
    if (s48 >= MASK48)
    {
        ui->lineStart48->setText(QString::asprintf("%" PRId64, MASK48));
        ui->progressBar->setValue(10000);
        ui->progressBar->setFormat(QString::asprintf("Done"));
    }
    ui->buttonStart->setChecked(false);
    on_buttonStart_clicked();
}

void MainWindow::resultTimeout()
{
    update();
}

void MainWindow::removeCurrent()
{
    int row = ui->listResults->currentRow();
    if (row >= 0)
        ui->listResults->removeRow(row);
}

void MainWindow::copyResults()
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

void MainWindow::pasteResults()
{
    pasteList(false);
}

int MainWindow::pasteList(bool dummy)
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

void MainWindow::copyCoord()
{
    Pos p = ui->mapView->getActivePos();
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(QString::asprintf("%d, %d", p.x, p.z));
}


