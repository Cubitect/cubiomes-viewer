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
#include <QSettings>


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
    //ui->frameMap->layout()->addWidget(ui->toolBar);
    //ui->toolBar->setContentsMargins(0, 0, 0, 0);

    QAction *toorigin = new QAction(QIcon(":/icons/origin.png"), "Goto origin", this);
    toorigin->connect(toorigin, &QAction::triggered, [=](){ this->mapGoto(0,0,16); });
    ui->toolBar->addAction(toorigin);
    ui->toolBar->addSeparator();

    saction.resize(STRUCT_NUM);
    addMapAction(D_DESERT, "desert", "Show desert pyramid");
    addMapAction(D_JUNGLE, "jungle", "Show jungle temples");
    addMapAction(D_IGLOO, "igloo", "Show igloos");
    addMapAction(D_HUT, "hut", "Show swamp huts");
    addMapAction(D_VILLAGE, "village", "Show villages");
    addMapAction(D_MANSION, "mansion", "Show woodland mansions");
    addMapAction(D_MONUMENT, "monument", "Show ocean monuments");
    addMapAction(D_RUINS, "ruins", "Show ocean ruins");
    addMapAction(D_SHIPWRECK, "shipwreck", "Show shipwrecks");
    addMapAction(D_OUTPOST, "outpost", "Show illager outposts");
    addMapAction(D_PORTAL, "portal", "Show ruined portals");
    addMapAction(D_SPAWN, "spawn", "Show world spawn");
    addMapAction(D_STRONGHOLD, "stronghold", "Show strongholds");

    ui->splitterMap->setSizes(QList<int>({8000, 10000}));
    ui->splitterSearch->setSizes(QList<int>({1000, 2000}));
    ui->splitterConditions->setSizes(QList<int>({1000, 3000}));

    QFont mono = QFont("Monospace", 9);
    mono.setStyleHint(QFont::TypeWriter);
    ui->listConditions48->setFont(mono);
    ui->listConditionsFull->setFont(mono);
    ui->listResults->setFont(mono);
    ui->progressBar->setFont(mono);

    qRegisterMetaType< int64_t >("int64_t");
    qRegisterMetaType< uint64_t >("uint64_t");
    qRegisterMetaType< QVector<int64_t> >("QVector<int64_t>");
    qRegisterMetaType< Config >("Config");
    qRegisterMetaType< Condition >("Condition");
    qRegisterMetaTypeStreamOperators< Condition >("Condition");

    protodialog = new ProtoBaseDialog(this);

    //connect(&sthread, &SearchThread::results, this, &MainWindow::searchResultsAdd, Qt::BlockingQueuedConnection);
    connect(&sthread, &SearchThread::progress, this, &MainWindow::searchProgress, Qt::QueuedConnection);
    connect(&sthread, &SearchThread::searchFinish, this, &MainWindow::searchFinish, Qt::QueuedConnection);

    connect(&stimer, &QTimer::timeout, this, QOverload<>::of(&MainWindow::resultTimeout));
    stimer.start(500);

    searchProgress(0, ~(int64_t)0, 0);
    ui->spinThreads->setMaximum(QThread::idealThreadCount());
    ui->spinThreads->setValue(QThread::idealThreadCount());

    updateSensitivity();

    prevdir = ".";
    config.reset();
    loadSettings();
}

MainWindow::~MainWindow()
{
    stimer.stop();
    sthread.stop(); // tell search to stop at next convenience
    sthread.quit(); // tell the event loop to exit
    sthread.wait(); // wait for search to finish
    saveSettings();
    delete ui;
}

QAction *MainWindow::addMapAction(int stype, const char *iconpath, const char *tip)
{
    QIcon icon;
    QString inam = QString(":icons/") + iconpath;
    icon.addPixmap(QPixmap(inam + ".png"), QIcon::Normal, QIcon::On);
    icon.addPixmap(QPixmap(inam + "_d.png"), QIcon::Normal, QIcon::Off);
    QAction *action = new QAction(icon, tip, this);
    action->setCheckable(true);
    action->connect(action, &QAction::toggled, [=](bool state){ this->onActionMapToggled(stype, state); });
    ui->toolBar->addAction(action);
    saction[stype] = action;
    return action;
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
        int v = str2seed(ui->seedEdit->text(), seed);
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

void MainWindow::saveSettings()
{
    QSettings settings("Cubitect", "Cubiomes-Viewer");
    settings.setValue("mainwindow/size", size());
    settings.setValue("mainwindow/pos", pos());
    settings.setValue("config/restoreSession", config.restoreSession);
    settings.setValue("config/smoothMotion", config.smoothMotion);
    settings.setValue("config/seedsPerItem", config.seedsPerItem);
    settings.setValue("config/queueSize", config.queueSize);
    settings.setValue("config/maxMatching", config.maxMatching);
    if (config.restoreSession)
    {
        int mc = MC_1_16;
        int64_t seed = 0;
        getSeed(&mc, &seed, false);
        settings.setValue("map/mc", mc);
        settings.setValue("map/seed", (qlonglong)seed);
        settings.setValue("map/x", ui->mapView->getX());
        settings.setValue("map/z", ui->mapView->getZ());
        settings.setValue("map/scale", ui->mapView->getScale());
        for (int stype = 0; stype < STRUCT_NUM; stype++)
            settings.setValue("map/show" + QString::number(stype), ui->mapView->getShow(stype));
        saveProgress("session.save", true);
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("Cubitect", "Cubiomes-Viewer");
    resize(settings.value("mainwindow/size", size()).toSize());
    move(settings.value("mainwindow/pos", pos()).toPoint());
    config.restoreSession = settings.value("config/restoreSession", config.restoreSession).toBool();
    config.smoothMotion = settings.value("config/smoothMotion", config.smoothMotion).toBool();
    config.seedsPerItem = settings.value("config/seedsPerItem", config.seedsPerItem).toInt();
    config.queueSize = settings.value("config/queueSize", config.queueSize).toInt();
    config.maxMatching = settings.value("config/maxMatching", config.maxMatching).toInt();
    ui->mapView->hasinertia = config.smoothMotion;
    if (config.restoreSession)
    {
        int mc = settings.value("map/mc", MC_1_16).toInt();
        int64_t seed = settings.value("map/seed", 0).toLongLong();
        setSeed(mc, seed);
        qreal x = settings.value("map/x", 0).toDouble();
        qreal z = settings.value("map/z", 0).toDouble();
        qreal scale = settings.value("map/scale", 16).toDouble();
        mapGoto(x, z, scale);
        for (int stype = 0; stype < STRUCT_NUM; stype++)
        {
            bool s = settings.value("map/show" + QString::number(stype), false).toBool();
            saction[stype]->setChecked(s);
            ui->mapView->setShow(stype, s);
        }
        loadProgress("session.save", true);
    }
}

bool MainWindow::saveProgress(QString fnam, bool quiet)
{
    QFileInfo finfo(fnam);
    QFile file(fnam);
    prevdir = finfo.absolutePath();

    if (!file.open(QIODevice::WriteOnly))
    {
        if (!quiet)
            warning("Warning", "Failed to open file.");
        return false;
    }

    QTextStream stream(&file);
    stream << "#Version:  " << VERS_MAJOR << "." << VERS_MINOR << "." << VERS_PATCH << "\n";
    stream << "#Search:   " << ui->comboSearchType->currentIndex() << "\n";
    stream << "#Progress: " << ui->lineStart->text().toLongLong() << "\n";
    QVector<Condition> condvec = getConditions();
    for (Condition &c : condvec)
        stream << "#Cond: " << QByteArray((const char*) &c, sizeof(Condition)).toHex() << "\n";

    int n = ui->listResults->rowCount();
    for (int i = 0; i < n; i++)
    {
        int64_t seed = ui->listResults->item(i, 0)->data(Qt::UserRole).toLongLong();
        stream << QString::asprintf("%" PRId64 "\n", seed);
    }
    return true;
}

bool MainWindow::loadProgress(QString fnam, bool quiet)
{
    QFileInfo finfo(fnam);
    QFile file(fnam);
    prevdir = finfo.absolutePath();

    if (!file.open(QIODevice::ReadOnly))
    {
        if (!quiet)
            warning("Warning", "Failed to open file.");
        return false;
    }

    int major = 0, minor = 0, patch = 0;
    int searchtype = ui->comboSearchType->currentIndex();
    int64_t seed = ui->lineStart->text().toLongLong();
    QVector<Condition> condvec;
    QVector<int64_t> seeds;

    QTextStream stream(&file);
    QString line;
    line = stream.readLine();
    if (sscanf(line.toLatin1().data(), "#Version: %d.%d.%d", &major, &minor, &patch) != 3)
        return false;
    if (cmpVers(major, minor, patch) > 0 && !quiet)
        warning("Warning", "Progress file was created with a newer version.");

    while (stream.status() == QTextStream::Ok)
    {
        line = stream.readLine();
        if (line.isEmpty())
            break;
        if (sscanf(line.toLatin1().data(), "#Search: %d", &searchtype) == 1)
            continue;
        else if (sscanf(line.toLatin1().data(), "#Progress: %" PRId64, &seed) == 1)
            continue;
        else if (line.startsWith("#Cond:"))
        {
            QString hex = line.mid(6).trimmed();
            QByteArray ba = QByteArray::fromHex(QByteArray(hex.toLatin1().data()));
            if (ba.size() == sizeof(Condition))
            {
                Condition c = *(Condition*) ba.data();
                condvec.push_back(c);
            }
            else return false;
        }
        else
        {
            int64_t seed;
            if (sscanf(line.toLatin1().data(), "%" PRId64, &seed) == 1)
                seeds.push_back(seed);
            else return false;
        }
    }

    on_buttonRemoveAll_clicked();
    on_buttonClear_clicked();

    ui->comboSearchType->setCurrentIndex(searchtype);
    ui->lineStart->setText(QString::asprintf("%" PRId64, seed));

    for (Condition &c : condvec)
    {
        QListWidgetItem *item = new QListWidgetItem();
        addItemCondition(item, c);
    }

    searchResultsAdd(seeds, false);

    return true;
}


QListWidgetItem *MainWindow::lockItem(QListWidgetItem *item)
{
    QListWidgetItem *edit = item->clone();
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    item->setSelected(false);
    item->setBackground(QColor(Qt::lightGray));
    edit->setData(Qt::UserRole+1, (qulonglong)item);
    return edit;
}

bool list_contains(QListWidget *list, QListWidgetItem *item)
{
    if (!item)
        return false;
    int n = list->count();
    for (int i = 0; i < n; i++)
        if (list->item(i) == item)
            return true;
    return false;
}

// [ID] Condition Cnt Rel Area
void MainWindow::setItemCondition(QListWidget *list, QListWidgetItem *item, Condition *cond)
{
    QListWidgetItem *target = (QListWidgetItem*) item->data(Qt::UserRole+1).toULongLong();
    if (list_contains(list, target) && !(target->flags() & Qt::ItemIsSelectable))
    {
        item->setData(Qt::UserRole+1, (qulonglong)0);
        *target = *item;
        delete item;
        item = target;
    }
    else
    {
        list->addItem(item);
        cond->save = getIndex(cond->save);
    }

    const FilterInfo& ft = g_filterinfo.list[cond->type];
    QString s = QString::asprintf("[%02d] %-28sx%-3d", cond->save, ft.name, cond->count);

    if (cond->relative)
        s += QString::asprintf("[%02d]+", cond->relative);
    else
        s += "     ";

    if (ft.coord)
        s += QString::asprintf("(%d,%d)", cond->x1*ft.step, cond->z1*ft.step);
    if (ft.area)
        s += QString::asprintf(",(%d,%d)", (cond->x2+1)*ft.step-1, (cond->z2+1)*ft.step-1);

    if (ft.cat == CAT_48)
        item->setBackground(QColor(Qt::yellow));
    else if (ft.cat == CAT_FULL)
        item->setBackground(QColor(Qt::green));

    item->setText(s);
    item->setData(Qt::UserRole, QVariant::fromValue(*cond));
}

void MainWindow::editCondition(QListWidgetItem *item)
{
    if (!(item->flags() & Qt::ItemIsSelectable))
        return;
    FilterDialog *dialog = new FilterDialog(this, item, (Condition*)item->data(Qt::UserRole).data());
    QObject::connect(dialog, SIGNAL(setCond(QListWidgetItem*,Condition)), this, SLOT(addItemCondition(QListWidgetItem*,Condition)), Qt::QueuedConnection);
    dialog->show();
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

void MainWindow::mapGoto(qreal x, qreal z, qreal scale)
{
    ui->mapView->setView(x, z, scale);
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
    int v = str2seed(a, &s);
    switch (v)
    {
        case 0: ui->labelSeedType->setText("(text)"); break;
        case 1: ui->labelSeedType->setText("(numeric)"); break;
        case 2: ui->labelSeedType->setText("(random)"); break;
    }
}

static void remove_selected(QListWidget *list)
{
    QList<QListWidgetItem*> selected = list->selectedItems();
    for (QListWidgetItem *item : selected)
    {
        delete list->takeItem(list->row(item));
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
    QListWidgetItem* edit = 0;
    QList<QListWidgetItem*> selected;

    list = ui->listConditions48;
    selected = list->selectedItems();
    if (!selected.empty())
        edit = lockItem(selected.first());
    else
    {
        list = ui->listConditionsFull;
        selected = list->selectedItems();
        if (!selected.empty())
            edit = lockItem(selected.first());
    }

    if (edit)
        editCondition(edit);
}

void MainWindow::on_buttonAddFilter_clicked()
{
    FilterDialog *dialog = new FilterDialog(this);
    QObject::connect(dialog, SIGNAL(setCond(QListWidgetItem*,Condition)), this, SLOT(addItemCondition(QListWidgetItem*,Condition)), Qt::QueuedConnection);
    dialog->show();
}


void MainWindow::on_listConditions48_itemDoubleClicked(QListWidgetItem *item)
{
    editCondition(lockItem(item));
}

void MainWindow::on_listConditionsFull_itemDoubleClicked(QListWidgetItem *item)
{
    editCondition(lockItem(item));
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
    searchProgress(0, 0, 0);
}

void MainWindow::on_buttonStart_clicked()
{
    if (ui->buttonStart->isChecked())
    {
        int mc = MC_1_16;
        getSeed(&mc, NULL);
        QVector<Condition> condvec = getConditions();
        int64_t sstart = (int64_t)ui->lineStart->text().toLongLong();
        int searchtype = ui->comboSearchType->currentIndex();
        int threads = ui->spinThreads->value();
        int ok = true;

        if (condvec.empty())
        {
            warning("Warning", "Please define some constraints using the \"Add\" button.");
            ok = false;
        }
        if (searchtype == SEARCH_LIST && slist64.empty())
        {
            warning("Warning", "No seed list file selected.");
            ok = false;
        }
        if (sthread.isRunning())
        {
            warning("Warning", "Search is still running.");
            ok = false;
        }

        if (ok)
            ok = sthread.set(searchtype, threads, slist64, sstart, mc, condvec, config.seedsPerItem, config.queueSize);

        if (ok)
        {
            ui->lineStart->setText(QString::asprintf("%" PRId64, sstart));
            ui->comboSearchType->setEnabled(false);
            ui->spinThreads->setEnabled(false);
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
        //sthread.quit(); // tell the event loop to exit
        //sthread.wait(); // wait for search to finish
        ui->buttonStart->setEnabled(true);
        ui->buttonStart->setText("Start search");
        ui->buttonStart->setIcon(QIcon::fromTheme("system-search"));
        ui->comboSearchType->setEnabled(true);
        ui->spinThreads->setEnabled(true);
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

void MainWindow::on_buttonSearchHelp_clicked()
{
    const char* msg =
            "<html><head/><body><p>The <span style=\" font-weight:600;\">incremental</span> "
            "search checks seeds in numerical order, save for grouping into work items for parallelization. "
            "This type of search is best suited for a non-exhaustive search space and with strong biome dependencies.</p>"
            "<p>With <span style=\" font-weight:600;\">48-bit family blocks</span> the search looks for suitable "
            "48-bit seeds first and parallelizes the search through the upper 16-bits. "
            "This type of search is best suited for exhaustive searches and for many types of structure restrictions.</p>"
            "<p>Load a <span style=\" font-weight:600;\">seed list from a file</span> to search through an existing set of seeds. "
            "The seeds should be in decimal ASCII text, separated by newline characters. "
            "You can browse for a file using the &quot;...&quot; button.</p></body></html>"
            ;
    QMessageBox::information(this, "Help: search types", msg, QMessageBox::Ok);
}


void MainWindow::on_actionSave_triggered()
{
    QString fnam = QFileDialog::getSaveFileName(this, "Save progress", prevdir, "Text files (*.txt);;Any files (*)");
    if (!fnam.isEmpty())
        saveProgress(fnam);
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
        if (!loadProgress(fnam))
            warning("Warning", "Failed to parse progress file.");
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}


void MainWindow::on_actionCopy_triggered()
{
    // TODO: different types of copy/paste based on context
    copyResults();
}

void MainWindow::on_actionPaste_triggered()
{
    pasteResults();
}

void MainWindow::on_actionPreferences_triggered()
{
    ConfigDialog *dialog = new ConfigDialog(this, &config);
    int status = dialog->exec();
    if (status == QDialog::Accepted)
    {
        config = dialog->getConfig();
        ui->mapView->hasinertia = config.smoothMotion;
    }
}

void MainWindow::on_actionGo_to_triggered()
{
    GotoDialog *dialog = new GotoDialog(this, ui->mapView->getX(), ui->mapView->getZ(), ui->mapView->getScale());
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


void MainWindow::on_actionSearch_seed_list_triggered()
{
    ui->comboSearchType->setCurrentIndex(SEARCH_LIST);
    on_buttonLoadList_clicked();
}

void MainWindow::on_actionSearch_full_seed_space_triggered()
{
    ui->comboSearchType->setCurrentIndex(SEARCH_BLOCKS);
    slistfnam.clear();
    slist64.clear();
}

void MainWindow::on_comboSearchType_currentIndexChanged(int index)
{
    ui->buttonLoadList->setEnabled(index == SEARCH_LIST);
}

void MainWindow::on_buttonLoadList_clicked()
{
    QString fnam = QFileDialog::getOpenFileName(this, "Load seed list", prevdir, "Text files (*.txt);;Any files (*)");
    if (!fnam.isEmpty())
    {
        QFileInfo finfo(fnam);
        prevdir = finfo.absolutePath();
        slistfnam = finfo.fileName();
        int64_t *l = NULL;
        int64_t len;
        QByteArray ba = fnam.toLatin1();
        l = loadSavedSeeds(ba.data(), &len);
        if (l && len > 0)
        {
            slist64.assign(l, l+len);
            searchProgress(0, len, l[0]);
            free(l);
        }
        else
        {
            warning("Warning", "Failed to load seed list from file");
        }
    }
}

void MainWindow::on_mapView_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction("Copy coordinates", this, &MainWindow::copyCoord);
    menu.addAction("Go to coordinates...", this, &MainWindow::on_actionGo_to_triggered);
    menu.exec(ui->mapView->mapToGlobal(pos));
}


void MainWindow::onActionMapToggled(int stype, bool show)
{
    ui->mapView->setShow(stype, show);
}

void MainWindow::addItemCondition(QListWidgetItem *item, Condition cond)
{
    const FilterInfo& ft = g_filterinfo.list[cond.type];

    if (ft.cat == CAT_FULL)
    {
        if (!item)
            item = new QListWidgetItem();
        setItemCondition(ui->listConditionsFull, item, &cond);
    }
    else if (ft.cat == CAT_48)
    {
        if (item)
        {
            setItemCondition(ui->listConditions48, item, &cond);
            return;
        }
        else
        {
            item = new QListWidgetItem();
            setItemCondition(ui->listConditions48, item, &cond);
        }

        if (cond.type >= F_QH_IDEAL && cond.type <= F_QH_BARELY)
        {
            Condition cq = cond;
            cq.type = F_HUT;
            cq.x1 = -128; cq.z1 = -128;
            cq.x2 = +128; cq.z2 = +128;
            cq.relative = cond.save;
            cq.save = cond.save+1;
            cq.count = 4;
            QListWidgetItem *item = new QListWidgetItem(ui->listConditionsFull, QListWidgetItem::UserType);
            setItemCondition(ui->listConditionsFull, item, &cq);
        }
        else if (cond.type == F_QM_90 || cond.type == F_QM_95)
        {
            Condition cq = cond;
            cq.type = F_MONUMENT;
            cq.x1 = -160; cq.z1 = -160;
            cq.x2 = +160; cq.z2 = +160;
            cq.relative = cond.save;
            cq.save = cond.save+1;
            cq.count = 4;
            QListWidgetItem *item = new QListWidgetItem(ui->listConditionsFull, QListWidgetItem::UserType);
            setItemCondition(ui->listConditionsFull, item, &cq);
        }
    }
}

int MainWindow::searchResultsAdd(QVector<int64_t> seeds, bool countonly)
{
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
        warning("Warning", QString::asprintf("Maximum number of results reached (%d).", config.maxMatching));
    }

    int addcnt = n - ns;
    if (ui->checkStop->isChecked() && addcnt)
    {
        sthread.reqstop = true;
        sthread.pool.clear();
    }

    return addcnt;
}

void MainWindow::searchProgress(uint64_t last, uint64_t end, int64_t seed)
{
//    if (sthread.itemgen.searchtype == SEARCH_BLOCKS)
//        seed &= MASK48;
    ui->lineStart->setText(QString::asprintf("%" PRId64, seed));

    if (end)
    {
        int v = (int) (10000 * (double)last / end);
        ui->progressBar->setValue(v);
        QString fmt = QString::asprintf(
                    "%" PRIu64 " / %" PRIu64 " (%d.%02d%%)", last, end, v / 100, v % 100);
        if (!slistfnam.isEmpty())
            fmt = slistfnam + ": " + fmt;
        ui->progressBar->setFormat(fmt);
    }
}

void MainWindow::searchFinish()
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
