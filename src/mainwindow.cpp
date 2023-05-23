#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "presetdialog.h"
#include "aboutdialog.h"
#include "conditiondialog.h"
#include "extgendialog.h"
#include "biomecolordialog.h"
#include "structuredialog.h"
#include "exportdialog.h"
#include "layerdialog.h"
#include "tabtriggers.h"
#include "tabbiomes.h"
#include "tabstructures.h"

#if WITH_UPDATER
#include "updater.h"
#endif

#include "world.h"
#include "cutil.h"

#include <QIntValidator>
#include <QMetaType>
#include <QtDebug>
#include <QDataStream>
#include <QMenu>
#include <QFileDialog>
#include <QTextStream>
#include <QSettings>
#include <QTreeWidget>
#include <QDateTime>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include <QTranslator>
#include <QLibraryInfo>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui()
    , dock()
    , mapView()
    , formCond()
    , formGen48()
    , formControl()
    , lopt()
    , config()
    , mconfig(false)
    , prevdir(".")
    , autosaveTimer()
    , prevtab(-1)
    , dimactions{}
    , dimgroup()
{
    QSettings settings(APP_STRING, APP_STRING);
    QString lang = settings.value("config/lang", QLocale().name()).toString();
    if (!loadTranslation(lang))
        loadTranslation("en_US");

    ui = new Ui::MainWindow;
    dock = new QDockWidget(tr("Map"), this);
    mapView = new MapView(this);

    ui->setupUi(this);

    dock->setWidget(mapView);
    dock->setFeatures(QDockWidget::DockWidgetFloatable);
    connect(dock, &QDockWidget::topLevelChanged, this, &MainWindow::onDockFloating);
    QMainWindow *submain = new QMainWindow(this);
    ui->frameMap->layout()->addWidget(submain);
    submain->addDockWidget(Qt::LeftDockWidgetArea, dock);
    mapView->setFocusPolicy(Qt::StrongFocus);
    setDockable(false);

    formCond = new FormConditions(this);
    formGen48 = new FormGen48(this);
    formControl = new FormSearchControl(this);

    ui->menuHistory->clear();

    ui->tabContainer->addTab(new TabTriggers(this), tr("Triggers"));
    ui->tabContainer->addTab(new TabBiomes(this), tr("Biomes"));
    ui->tabContainer->addTab(new TabStructures(this), tr("Structures"));

    laction.resize(LOPT_MAX);
    laction[LOPT_BIOMES] = ui->actionBiomes;
    laction[LOPT_NOISE_T_4] = ui->actionParaTemperature;
    laction[LOPT_NOISE_H_4] = ui->actionParaHumidity;
    laction[LOPT_NOISE_C_4] = ui->actionParaContinentalness;
    laction[LOPT_NOISE_E_4] = ui->actionParaErosion;
    laction[LOPT_NOISE_W_4] = ui->actionParaWeirdness;
    laction[LOPT_NOISE_D_4] = ui->actionParaDepth;
    laction[LOPT_RIVER_4] = ui->actionRiver;
    laction[LOPT_OCEAN_256] = ui->actionOceanTemp;
    laction[LOPT_NOOCEAN_1] = ui->actionNoOceans;
    laction[LOPT_BETA_T_1] = ui->actionBetaTemperature;
    laction[LOPT_BETA_H_1] = ui->actionBetaHumidity;
    laction[LOPT_HEIGHT_4] = ui->actionHeight;
    laction[LOPT_STRUCTS] = ui->actionStructures;

    QActionGroup *grp = new QActionGroup(this);
    for (int i = 0; i < laction.size(); i++)
    {
        QAction *act = laction[i];
        if (!act) continue;
        connect(act, &QAction::toggled, [=](bool) {
            this->onActionBiomeLayerSelect(i);
        });
        act->setActionGroup(grp);
    }
    connect(mapView, &MapView::layerChange, this, &MainWindow::onActionBiomeLayerSelect);
    for (int i = 0; i < 9; i++)
    {
        QAction *act = new QAction(this);
        act->setShortcut(QKeySequence(Qt::ALT+Qt::Key_1+i));
        act->setEnabled(true);
        connect(act, &QAction::triggered, [=](){
            this->onActionBiomeLayerSelect(lopt.mode, i);
        });
        addAction(act);
    }

    QAction *toorigin = new QAction(QIcon(":/icons/origin.png"), "Goto origin", this);
    connect(toorigin, &QAction::triggered, [=](){ this->mapGoto(0,0,16); });
    ui->toolBar->addAction(toorigin);
    ui->toolBar->addSeparator();

    dimactions[0] = addMapAction(-1, "overworld", tr("Overworld"));
    dimactions[1] = addMapAction(-1, "nether", tr("Nether"));
    dimactions[2] = addMapAction(-1, "the_end", tr("End"));
    dimgroup = new QActionGroup(this);

    for (int i = 0; i < 3; i++)
    {
        connect(dimactions[i], &QAction::triggered, [=](){ this->updateMapSeed(); });
        ui->toolBar->addAction(dimactions[i]);
        dimgroup->addAction(dimactions[i]);
    }
    dimactions[0]->setChecked(true);

    saction.resize(D_STRUCT_NUM);
    addMapAction(D_GRID, "grid", tr("Show grid"));
    addMapAction(D_SLIME, "slime", tr("Show slime chunks"));
    addMapAction(D_SPAWN, "spawn", tr("Show world spawn"));
    addMapAction(D_STRONGHOLD, "stronghold", tr("Show strongholds"));
    addMapAction(D_VILLAGE, "village", tr("Show villages"));
    addMapAction(D_MINESHAFT, "mineshaft", tr("Show abandoned mineshafts"));
    addMapAction(D_DESERT, "desert", tr("Show desert pyramid"));
    addMapAction(D_JUNGLE, "jungle", tr("Show jungle temples"));
    addMapAction(D_HUT, "hut", tr("Show swamp huts"));
    addMapAction(D_MONUMENT, "monument", tr("Show ocean monuments"));
    addMapAction(D_IGLOO, "igloo", tr("Show igloos"));
    addMapAction(D_MANSION, "mansion", tr("Show woodland mansions"));
    addMapAction(D_RUINS, "ruins", tr("Show ocean ruins"));
    addMapAction(D_SHIPWRECK, "shipwreck", tr("Show shipwrecks"));
    addMapAction(D_TREASURE, "treasure", tr("Show buried treasures"));
    addMapAction(D_WELL, "well", tr("Show desert wells"));
    addMapAction(D_GEODE, "geode", tr("Show amethyst geodes"));
    addMapAction(D_OUTPOST, "outpost", tr("Show pillager outposts"));
    addMapAction(D_PORTAL, "portal", tr("Show ruined portals"));
    addMapAction(D_ANCIENTCITY, "ancient_city", tr("Show ancient cities"));
    addMapAction(D_TRAIL, "trail", tr("Show trail ruins"));
    ui->toolBar->addSeparator();
    addMapAction(D_FORTESS, "fortress", tr("Show nether fortresses"));
    addMapAction(D_BASTION, "bastion", tr("Show bastions"));
    ui->toolBar->addSeparator();
    addMapAction(D_ENDCITY, "endcity", tr("Show end cities"));
    addMapAction(D_GATEWAY, "gateway", tr("Show end gateways"));

    saction[D_GRID]->setChecked(true);

    ui->splitterMap->setSizes(QList<int>({6000, 10000}));
    ui->splitterSearch->setSizes(QList<int>({1000, 1000, 2000}));

    qRegisterMetaType< int64_t >("int64_t");
    qRegisterMetaType< uint64_t >("uint64_t");
    qRegisterMetaType< QVector<uint64_t> >("QVector<uint64_t>");
    qRegisterMetaType< Config >("Config");

    ui->comboY->lineEdit()->setValidator(new QIntValidator(-64, 320, this));

    formCond->updateSensitivity();

    connect(&autosaveTimer, &QTimer::timeout, this, &MainWindow::onAutosaveTimeout);

    loadSettings();

    ui->collapseConstraints->init(tr("Conditions"), formCond, false);
    connect(formCond, &FormConditions::changed, this, &MainWindow::onConditionsChanged);
    ui->collapseConstraints->setInfo(
        tr("Help: Conditions"),
        tr(
        "<html><head/><body><p>"
        "The search conditions define the properties by which potential seeds "
        "are filtered."
        "</p><p>"
        "Conditions can reference each other to produce relative positional "
        "dependencies (indicated with the ID in square brackets [XY]). "
        "When a condition passes its check, it usually yields just one location "
        "that other conditions can reference. An exception to this are structure "
        "conditions with exactly one required instance. In this case, each found "
        "structure occurence is examined separately instead. On the other hand, "
        "a condition that checks for a structure cluster, will average the "
        "position of all occurences and yield a single position."
        "</p><p>"
        "Standard biome conditions yield the center of the testing area "
        "as they evaluate the area as a whole. To locate the position of a "
        "given biome you can use the designated <b>locate</b> filters, or use a "
        "spiral iterator to scan an area with a localized condition."
        "</p></body></html>"
    ));

    // 48-bit generator settings are not all that interesting unless we are
    // using them, so start as collapsed if they are on the "Auto" setting.
    Gen48Config gen48 = formGen48->getConfig(false);
    ui->collapseGen48->init(tr("Seed generator (48-bit)"), formGen48, gen48.mode == GEN48_AUTO);
    connect(formGen48, &FormGen48::changed, this, &MainWindow::onGen48Changed);
    ui->collapseGen48->setInfo(
        tr("Help: Seed generator"),
        tr(
        "<html><head/><body><p>"
        "For some searches, the 48-bit structure seed candidates can be "
        "generated without searching, which can vastly reduce the search space "
        "that has to be checked."
        "</p><p>"
        "The generator mode <b>Auto</b> is recommended for general use, which "
        "automatically selects suitable options based on the conditions list."
        "</p><p>"
        "The <b>Quad-feature</b> mode produces candidates for "
        "quad&#8209;structures that have a uniform distribution of "
        "region&#8209;size=32 and chunk&#8209;gap=8, such as swamp huts."
        "</p><p>"
        "A perfect <b>Quad-monument</b> structure constellation does not "
        "actually exist, but some extremely rare structure seed bases get close, "
        "with over 90&#37; of the area within 128 blocks. The generator uses a "
        "precomputed list of these seed bases."
        "</p><p>"
        "Using a <b>Seed list</b> you can provide a custom set of 48-bit "
        "candidates. Optionally, a salt value can be added and the seeds can "
        "be region transposed."
        "</p></body></html>"
    ));

    ui->collapseControl->init("Matching seeds", formControl, false);
    connect(formControl, &FormSearchControl::selectedSeedChanged, this, &MainWindow::onSelectedSeedChanged);
    connect(formControl, &FormSearchControl::searchStatusChanged, this, &MainWindow::onSearchStatusChanged);
    ui->collapseControl->setInfo(
        tr("Help: Matching seeds"),
        tr(
        "<html><head/><body><p>"
        "The list of seeds acts as a buffer onto which suitable seeds are added "
        "when they are found. You can also copy the seed list, or paste seeds "
        "into the list. Selecting a seed will open it in the map view."
        "</p></body></html>"
    ));

    onConditionsChanged();
    update();

#if WITH_UPDATER
    QAction *updateaction = new QAction("Check for updates", this);
    connect(updateaction, &QAction::triggered, [=]() { searchForUpdates(false); });
    ui->menuHelp->insertAction(ui->actionAbout, updateaction);
    ui->menuHelp->insertSeparator(ui->actionAbout);

    if (config.checkForUpdates)
        searchForUpdates(true);
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    formControl->stopSearch();
    QThreadPool::globalInstance()->clear();
    saveSettings();
    QMainWindow::closeEvent(event);
}

bool MainWindow::loadTranslation(QString lang)
{
    static QTranslator rc_translator;
    static QTranslator qt_translator;
    if (!rc_translator.load(lang, ":/lang"))
        return false;
    QLocale::setDefault(lang);
    QString qt_locale = "qtbase_" + lang;
    QString qt_trpath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    if (qt_translator.load(qt_locale, qt_trpath))
        qApp->installTranslator(&qt_translator);
    qApp->installTranslator(&rc_translator);
    config.lang = lang;
    return true;
}

QAction *MainWindow::addMapAction(int sopt, const char *iconpath, QString tip)
{
    QIcon icon;
    QString inam = QString(":icons/") + iconpath;
    icon.addPixmap(QPixmap(inam + ".png"), QIcon::Normal, QIcon::On);
    icon.addPixmap(QPixmap(inam + "_d.png"), QIcon::Normal, QIcon::Off);
    QAction *action = new QAction(icon, tip, this);
    action->setCheckable(true);
    ui->toolBar->addAction(action);
    if (sopt >= 0)
    {
        action->connect(action, &QAction::toggled, [=](bool state){ this->onActionMapToggled(sopt, state); });
        saction[sopt] = action;
    }
    return action;
}


MapView* MainWindow::getMapView()
{
    return mapView;
}

bool MainWindow::getSeed(WorldInfo *wi, bool applyrand)
{
    bool ok = true;
    const std::string& mcs = ui->comboBoxMC->currentText().toStdString();
    wi->mc = str2mc(mcs.c_str());
    if (wi->mc < 0)
    {
        if (applyrand)
            qDebug() << "Unknown MC version: " << mcs.c_str();
        wi->mc = MC_NEWEST;
        ok = false;
    }

    int v = str2seed(ui->seedEdit->text(), &wi->seed);
    if (v == S_RANDOM)
    {
        if (applyrand)
            ui->seedEdit->setText(QString::asprintf("%" PRId64, (int64_t)wi->seed));
        else
            ok = false;
    }

    wi->large = ui->checkLarge->isChecked();
    wi->y = ui->comboY->currentText().section(' ', 0, 0).toInt();

    return ok;
}

bool MainWindow::setSeed(WorldInfo wi, int dim)
{
    const char *mcstr = mc2str(wi.mc);
    if (!mcstr)
    {
        qDebug() << "Unknown MC version: " << wi.mc;
        return false;
    }

    if (dim == DIM_OVERWORLD)
        dimactions[0]->setChecked(true);
    if (dim == DIM_NETHER)
        dimactions[1]->setChecked(true);
    else if (dim == DIM_END)
        dimactions[2]->setChecked(true);
    else
        dim = getDim();

    MapView *mapview = getMapView();
    uint64_t current = mapview->world ? mapview->world->wi.seed : wi.seed;
    if (current != wi.seed)
    {
        QList<QAction*> hist = ui->menuHistory->actions();
        bool rm = false;
        for (QAction *act : qAsConst(hist))
        {
            if (act->data().toULongLong() == current)
            {
                ui->menuHistory->removeAction(act);
                hist.back()->deleteLater();
                rm = true;
            }
        }
        if (!rm && hist.size() >= 12)
        {
            ui->menuHistory->removeAction(hist.back());
            hist.back()->deleteLater();
        }
        QString s = QString::asprintf("%" PRId64, current);
        QAction *act = new QAction(s, this);
        act->setData(QVariant::fromValue(current));
        act->connect(act, &QAction::triggered, [=](){ this->onActionHistory(act); });
        ui->menuHistory->insertAction(hist.empty() ? 0 : hist.first(), act);
        ui->menuHistory->setEnabled(true);
    }

    // temporarily disable UI to prevent recursive setSeed() updates
    ui->comboBoxMC->setEnabled(false);
    ui->checkLarge->setEnabled(false);
    ui->seedEdit->setEnabled(false);
    ui->comboY->setEnabled(false);

    ui->checkLarge->setChecked(wi.large);
    int i, n = ui->comboY->count();
    for (i = 0; i < n; i++)
        if (ui->comboY->itemText(i).section(' ', 0, 0).toInt() == wi.y)
            break;
    if (i >= n)
        ui->comboY->addItem(QString::number(wi.y));
    ui->comboY->setCurrentIndex(i);

    ui->comboBoxMC->setCurrentText(mcstr);
    ui->seedEdit->setText(QString::asprintf("%" PRId64, (int64_t)wi.seed));
    getMapView()->setSeed(wi, dim, lopt);

    ui->comboBoxMC->setEnabled(true);
    ui->checkLarge->setEnabled(wi.mc >= MC_1_3);
    ui->seedEdit->setEnabled(true);
    ui->comboY->setEnabled(true);

    ISaveTab *tab = dynamic_cast<ISaveTab*>(ui->tabContainer->currentWidget());
    if (tab)
        tab->refresh();
    return true;
}

int MainWindow::getDim()
{
    if (!dimgroup)
        return 0;
    QAction *active = dimgroup->checkedAction();
    if (active == dimactions[1])
        return -1; // nether
    if (active == dimactions[2])
        return +1; // end
    return 0;
}

void MainWindow::saveSettings()
{
    QSettings settings(APP_STRING, APP_STRING);

    settings.setValue("mainwindow/size", size());
    settings.setValue("mainwindow/pos", pos());
    settings.setValue("mainwindow/prevdir", prevdir);

    settings.setValue("toolbar/area", toolBarArea(ui->toolBar));

    config.save(settings);

    g_extgen.save(settings);

    on_tabContainer_currentChanged(-1);

    WorldInfo wi;
    getSeed(&wi, false);
    wi.save(settings);
    settings.setValue("map/dim", getDim());
    settings.setValue("map/x", getMapView()->getX());
    settings.setValue("map/z", getMapView()->getZ());
    settings.setValue("map/scale", getMapView()->getScale());
    for (int stype = 0; stype < D_STRUCT_NUM; stype++)
    {
        QString s = QString("map/show_") + mapopt2str(stype);
        settings.setValue(s, getMapView()->getShow(stype));
    }

    if (config.restoreSession)
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        QDir dir(path);
        if (!dir.exists())
            dir.mkpath(".");
        saveProgress(path + "/session.save", true);
    }
}


void MainWindow::loadSettings()
{
    QSettings settings(APP_STRING, APP_STRING);

    getMapView()->deleteWorld();

    resize(settings.value("mainwindow/size", size()).toSize());
    move(settings.value("mainwindow/pos", pos()).toPoint());
    prevdir = settings.value("mainwindow/prevdir", pos()).toString();

    int toolarea = toolBarArea(ui->toolBar);
    int toolarea_new = settings.value("toolbar/area", toolarea).toInt();
    if (toolarea != toolarea_new)
    {
        removeToolBar(ui->toolBar);
        addToolBar((Qt::ToolBarArea) toolarea_new, ui->toolBar);
        ui->toolBar->show();
    }

    qreal x = getMapView()->getX();
    qreal z = getMapView()->getZ();
    qreal scale = getMapView()->getScale();
    x = settings.value("map/x", x).toDouble();
    z = settings.value("map/z", z).toDouble();
    scale = settings.value("map/scale", scale).toDouble();
    mapGoto(x, z, scale);

    for (int sopt = 0; sopt < D_STRUCT_NUM; sopt++)
    {
        bool show = getMapView()->getShow(sopt);
        QString soptstr = QString("map/show_") + mapopt2str(sopt);
        show = settings.value(soptstr, show).toBool();
        if (saction[sopt])
            saction[sopt]->setChecked(show);
        getMapView()->setShow(sopt, show);
    }

    g_extgen.load(settings);
    setMCList(g_extgen.experimentalVers);

    WorldInfo wi;
    // NOTE: version can be wrong when the mc-enum changes, but the session file should correct it
    getSeed(&wi, false);
    wi.load(settings);
    int dim = settings.value("map/dim", getDim()).toInt();
    setSeed(wi, dim);

    onUpdateConfig();
    onUpdateMapConfig();

    if (config.restoreSession)
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        path += "/session.save";
        if (QFile::exists(path))
        {
            loadProgress(path, false, false);
        }
    }
}


bool MainWindow::saveProgress(QString fnam, bool quiet)
{
    QFile file(fnam);

    if (!file.open(QIODevice::WriteOnly))
    {
        if (!quiet)
            warning(tr("Failed to open file:\n\"%1\"").arg(fnam));
        return false;
    }

    SearchConfig searchconf = formControl->getSearchConfig();
    Gen48Config gen48 = formGen48->getConfig(false);
    QVector<Condition> condvec = formCond->getConditions();
    QVector<uint64_t> results = formControl->getResults();

    WorldInfo wi;
    getSeed(&wi);

    QTextStream stream(&file);
    stream << "#Version:  " << VERS_MAJOR << "." << VERS_MINOR << "." << VERS_PATCH << "\n";
    stream << "#Time:     " << QDateTime::currentDateTime().toString() << "\n";
    // MC version of the session should take priority over the one in the settings
    wi.write(stream);

    searchconf.write(stream);
    gen48.write(stream);

    for (Condition &c : condvec)
        stream << "#Cond: " << c.toHex() << "\n";

    for (uint64_t s : qAsConst(results))
        stream << QString::asprintf("%" PRId64 "\n", (int64_t)s);

    return true;
}

bool MainWindow::loadProgress(QString fnam, bool keepresults, bool quiet)
{
    QFile file(fnam);

    if (!file.open(QIODevice::ReadOnly))
    {
        if (!quiet)
            warning(tr("Failed to open progress file:\n\"%1\"").arg(fnam));
        return false;
    }

    int major = 0, minor = 0, patch = 0;
    SearchConfig searchconf = formControl->getSearchConfig();
    Gen48Config gen48 = formGen48->getConfig(false);
    QVector<Condition> condvec;
    QVector<uint64_t> seeds;

    WorldInfo wi;
    getSeed(&wi, true);

    QTextStream stream(&file);
    QString line;
    line = stream.readLine();
    int lno = 1;

    if (sscanf(line.toLocal8Bit().data(), "#Version: %d.%d.%d", &major, &minor, &patch) != 3)
    {
        if (quiet)
            return false;
        int button = QMessageBox::warning(this, tr("Warning"),
            tr("File does not look like a progress file.\n"
            "Progress may be incomplete or broken.\n\n"
            "Continue anyway?"),
            QMessageBox::Abort|QMessageBox::Yes);
        if (button == QMessageBox::Abort)
            return false;
    }
    else if (cmpVers(major, minor, patch) > 0)
    {
        if (quiet)
            return false;
        int button = QMessageBox::warning(this, tr("Warning"),
            tr("File was created with a newer version.\n"
            "Progress may be incomplete or broken.\n\n"
            "Continue loading progress anyway?"),
            QMessageBox::Abort|QMessageBox::Yes);
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
        if (searchconf.read(line)) continue;
        if (gen48.read(line)) continue;
        if (wi.read(line)) continue;

        if (line.startsWith("#Cond:"))
        {   // Conditions
            Condition c;
            if (c.readHex(line.mid(6).trimmed()))
            {
                condvec.push_back(c);
            }
            else
            {
                if (quiet)
                    return false;
                int button = QMessageBox::warning(this, tr("Warning"),
                    tr("Condition [%1] at line %2 is not supported.\n\n"
                    "Continue anyway?").arg(c.save).arg(lno),
                    QMessageBox::Abort|QMessageBox::Yes);
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
                seeds.push_back(s);
            }
            else
            {
                if (quiet)
                    return false;
                int button = QMessageBox::warning(this, tr("Warning"),
                    tr("Failed to parse line %1 of progress file:\n%2\n\n"
                    "Continue anyway?").arg(lno).arg(line),
                    QMessageBox::Abort|QMessageBox::Yes);
                if (button == QMessageBox::Abort)
                    return false;
            }
        }
    }

    setSeed(wi);

    formCond->on_buttonRemoveAll_clicked();
    for (Condition &c : condvec)
    {
        QListWidgetItem *item = new QListWidgetItem();
        formCond->addItemCondition(item, c);
    }

    formGen48->setConfig(gen48, quiet);
    formGen48->updateCount();

    if (!keepresults)
        formControl->on_buttonClear_clicked();
    formControl->setSearchConfig(searchconf, quiet);
    formControl->searchResultsAdd(seeds, false);
    formControl->searchProgressReset();

    return true;
}

void MainWindow::updateMapSeed()
{
    WorldInfo wi;
    if (getSeed(&wi, true))
        setSeed(wi);

    bool state;
    state = (wi.mc <= MC_B1_7);
    ui->actionNoOceans->setEnabled(state);
    ui->actionBetaTemperature->setEnabled(state);
    ui->actionBetaHumidity->setEnabled(state);
    state = (wi.mc >= MC_1_13 && wi.mc <= MC_1_17);
    ui->actionRiver->setEnabled(state);
    ui->actionOceanTemp->setEnabled(state);
    state = (wi.mc >= MC_1_18);
    ui->actionParaTemperature->setEnabled(state);
    ui->actionParaHumidity->setEnabled(state);
    ui->actionParaContinentalness->setEnabled(state);
    ui->actionParaErosion->setEnabled(state);
    ui->actionParaDepth->setEnabled(state);
    ui->actionParaWeirdness->setEnabled(state);

    ui->actionAddShadow->setEnabled(wi.mc <= MC_1_17);
    ui->actionOpenShadow->setEnabled(wi.mc <= MC_1_17);

    emit mapUpdated();
    update();
}

void MainWindow::setDockable(bool dockable)
{
    if (dockable)
    {   // reset to default
        QWidget *title = dock->titleBarWidget();
        dock->setTitleBarWidget(nullptr);
        delete title;
        //dock->resize(1920, 1080);
    }
    else
    {   // add a dummy widget with a layout to hide the title bar
        // and avoid a warning about negative size widget
        QWidget *title = new QWidget(this);
        QHBoxLayout *l = new QHBoxLayout(title);
        l->setMargin(0);
        title->setLayout(l);
        dock->setTitleBarWidget(title);
        dock->setFloating(false);
    }
}

void MainWindow::applyConfigChanges(const Config old, const Config conf)
{
    this->config = conf;
    getMapView()->setConfig(conf);
    if (old.uistyle != conf.uistyle)
        onStyleChanged(conf.uistyle);

    if (conf.autosaveCycle)
    {
        autosaveTimer.setInterval(conf.autosaveCycle * 60000);
        autosaveTimer.start();
    }
    else
    {
        autosaveTimer.stop();
    }

    if (!conf.biomeColorPath.isEmpty() || !old.biomeColorPath.isEmpty())
        onBiomeColorChange();
}

void MainWindow::setMCList(bool experimental)
{
    WorldInfo wi;
    if (ui->comboBoxMC->count())
        getSeed(&wi, false);
    else
        wi.mc = MC_NEWEST;
    QStringList mclist;
    for (int mc = MC_NEWEST; mc > MC_UNDEF; mc--)
    {
        if (!experimental && mc != wi.mc)
        {
            if (mc <= MC_1_0 || mc == MC_1_16_1 || mc == MC_1_19_2)
                continue;
        }
        const char *mcs = mc2str(mc);
        if (mcs)
            mclist.append(mcs);
    }
    const QString s = mc2str(wi.mc);
    ui->comboBoxMC->setEnabled(false);
    ui->comboBoxMC->clear();
    ui->comboBoxMC->addItems(mclist);
    ui->comboBoxMC->setCurrentText(s);
    ui->comboBoxMC->setEnabled(true);
}

int MainWindow::warning(QString text, QMessageBox::StandardButtons buttons)
{
    return QMessageBox::warning(this, tr("Warning"), text, buttons);
}

void MainWindow::mapGoto(qreal x, qreal z, qreal scale)
{
    getMapView()->setView(x, z, scale);
}

void MainWindow::setBiomeColorRc(QString rc)
{
    config.biomeColorPath = rc;
    onBiomeColorChange();
}

void MainWindow::on_comboBoxMC_currentIndexChanged(int)
{
    if (ui->comboBoxMC->isEnabled() && ui->comboBoxMC->count())
        updateMapSeed();
}
void MainWindow::on_seedEdit_editingFinished()
{
    if (ui->seedEdit->isEnabled())
        updateMapSeed();
}
void MainWindow::on_checkLarge_toggled()
{
    if (ui->checkLarge->isEnabled())
        updateMapSeed();
}
void MainWindow::on_comboY_currentIndexChanged(int)
{
    if (ui->comboY->isEnabled())
        updateMapSeed();
}

void MainWindow::on_seedEdit_textChanged(const QString &a)
{
    uint64_t s;
    int v = str2seed(a, &s);
    QString typ = "";
    switch (v)
    {
    case S_TEXT:    typ = " " + tr("text", "Seed input type"); break;
    case S_NUMERIC: typ = ""; break;
    case S_RANDOM:  typ = " " + tr("random", "Seed input type"); break;
    }
    ui->labelSeedType->setText(typ);
}

void MainWindow::on_actionSave_triggered()
{
    QString fnam = QFileDialog::getSaveFileName(
        this, tr("Save progress"), prevdir, tr("Text files (*.txt);;Any files (*)"));
    if (!fnam.isEmpty())
    {
        QFileInfo finfo(fnam);
        prevdir = finfo.absolutePath();
        saveProgress(fnam);
    }
}

void MainWindow::on_actionLoad_triggered()
{
    QString fnam = QFileDialog::getOpenFileName(
        this, tr("Load progress"), prevdir, tr("Text files (*.txt);;Any files (*)"));
    if (!fnam.isEmpty())
    {
        QFileInfo finfo(fnam);
        prevdir = finfo.absolutePath();
        loadProgress(fnam, false, false);
    }
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::on_actionPreferences_triggered()
{
    ConfigDialog *dialog = new ConfigDialog(this, &config);
    connect(dialog, SIGNAL(updateConfig()), this, SLOT(onUpdateConfig()));
    connect(dialog, SIGNAL(updateMapConfig()), this, SLOT(onUpdateMapConfig()));
    dialog->show();
}

void MainWindow::on_actionGo_to_triggered()
{
    getMapView()->onGoto();
}

void MainWindow::on_actionOpenShadow_triggered()
{
    WorldInfo wi;
    if (getSeed(&wi))
    {
        wi.seed = getShadow(wi.seed);
        setSeed(wi);
    }
}

void MainWindow::on_actionStructure_visibility_triggered()
{
    StructureDialog *dialog = new StructureDialog(this);
    connect(dialog, SIGNAL(updateMapConfig()), this, SLOT(onUpdateMapConfig()));
    dialog->show();
}

void MainWindow::on_actionBiome_colors_triggered()
{
    WorldInfo wi;
    getSeed(&wi);
    BiomeColorDialog *dialog = new BiomeColorDialog(this, config.biomeColorPath, wi.mc, getDim());
    connect(dialog, SIGNAL(yieldBiomeColorRc(QString)), this, SLOT(setBiomeColorRc(QString)));
    dialog->show();
}

void MainWindow::on_actionPresetLoad_triggered()
{
    WorldInfo wi;
    getSeed(&wi);
    PresetDialog *dialog = new PresetDialog(this, wi, false);
    dialog->setActiveFilter(formCond->getConditions());
    if (dialog->exec() && !dialog->rc.isEmpty())
        loadProgress(dialog->rc, true, false);
}

void MainWindow::on_actionExamples_triggered()
{
    WorldInfo wi;
    getSeed(&wi);
    PresetDialog *dialog = new PresetDialog(this, wi, true);
    dialog->setActiveFilter(formCond->getConditions());
    if (dialog->exec() && !dialog->rc.isEmpty())
        loadProgress(dialog->rc, true, false);
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog *dialog = new AboutDialog(this);
    dialog->show();
}

void MainWindow::on_actionCopy_triggered()
{
    formControl->copyResults();
}

void MainWindow::on_actionPaste_triggered()
{
    formControl->pasteResults();
}

void MainWindow::on_actionAddShadow_triggered()
{
    const QVector<uint64_t> results = formControl->getResults();
    QVector<uint64_t> shadows;
    shadows.reserve(results.size());
    for (uint64_t s : results)
        shadows.push_back( getShadow(s) );
    formControl->searchResultsAdd(shadows, false);
}

void MainWindow::on_actionExtGen_triggered()
{
    ExtGenDialog *dialog = new ExtGenDialog(this, &g_extgen);
    int status = dialog->exec();
    if (status == QDialog::Accepted)
    {
        g_extgen = dialog->getSettings();
        // invalidate the map world, forcing an update
        getMapView()->deleteWorld();
        setMCList(g_extgen.experimentalVers);
        updateMapSeed();
    }
}

void MainWindow::on_actionExportImg_triggered()
{
    ExportDialog *dialog = new ExportDialog(this);
    dialog->show();
}

void MainWindow::on_actionScreenshot_triggered()
{
    QString fnam = QFileDialog::getSaveFileName(
        this, tr("Save screenshot"), prevdir, tr("Images (*.png *.jpg *.ppm)"));
    if (!fnam.isEmpty())
    {
        QPixmap pixmap = getMapView()->screenshot();
        QImage img = pixmap.toImage();
        img.save(fnam);
    }
}

void MainWindow::on_actionDock_triggered()
{
    if (dock->isFloating())
    {
        setDockable(false);
        ui->actionDock->setText(tr("Undock map"));
    }
    else
    {
        setDockable(true);
        dock->setFloating(true);
        ui->actionDock->setText(tr("Redock map"));
    }
}

void MainWindow::on_actionLayerDisplay_triggered()
{
    WorldInfo wi;
    getSeed(&wi, false);
    LayerDialog *dialog = new LayerDialog(this, wi.mc);
    dialog->setLayerOptions(lopt);
    connect(dialog, &LayerDialog::apply, [=](){
        lopt = dialog->getLayerOptions();
        laction[lopt.mode]->setChecked(true);
        updateMapSeed();
    });
    dialog->show();
}

void MainWindow::on_tabContainer_currentChanged(int index)
{
    QSettings settings(APP_STRING, APP_STRING);
    ISaveTab *tabold = dynamic_cast<ISaveTab*>(ui->tabContainer->widget(prevtab));
    ISaveTab *tabnew = dynamic_cast<ISaveTab*>(ui->tabContainer->widget(index));
    if (tabold) tabold->save(settings);
    if (tabnew) tabnew->load(settings);
    prevtab = index;
}

void MainWindow::on_actionSearch_seed_list_triggered()
{
    formControl->setSearchMode(SEARCH_LIST);
}

void MainWindow::on_actionSearch_full_seed_space_triggered()
{
    formControl->setSearchMode(SEARCH_BLOCKS);
}

void MainWindow::onAutosaveTimeout()
{
    if (config.autosaveCycle)
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        saveProgress(path + "/session.save", true);
        //int dispms = 10000;
        //if (saveProgress(path + "/session.save", true))
        //    ui->statusBar->showMessage(tr("Session autosaved"), dispms);
        //else
        //    ui->statusBar->showMessage(tr("Autosave failed"), dispms);
    }
}

void MainWindow::onActionHistory(QAction *act)
{
    uint64_t seed = act->data().toULongLong();
    onSelectedSeedChanged(seed);
    ui->menuHistory->removeAction(act);
    act->deleteLater();
}

void MainWindow::onActionMapToggled(int sopt, bool show)
{
    if (sopt == D_PORTAL) // overworld portals should also control nether
        getMapView()->setShow(D_PORTALN, show);
    getMapView()->setShow(sopt, show);
}

void MainWindow::onActionBiomeLayerSelect(int mode, int disp)
{
    if (disp >= 0)
    {
        if (!getLayerOptionText(mode, disp))
            return; // unsupported display mode
        lopt.disp[mode] = disp;
    }
    lopt.mode = mode;
    WorldInfo wi;
    if (getSeed(&wi, false))
        setSeed(wi, DIM_UNDEF);
}

void MainWindow::onConditionsChanged()
{
    QVector<Condition> conds = formCond->getConditions();
    formGen48->updateAutoConditions(conds);
}

void MainWindow::onGen48Changed()
{
    formGen48->updateCount();
    formControl->searchProgressReset();
}

void MainWindow::onSelectedSeedChanged(uint64_t seed)
{
    WorldInfo wi;
    getSeed(&wi, false);
    wi.seed = seed;
    setSeed(wi);
}

void MainWindow::onSearchStatusChanged(bool running)
{
    formGen48->setEnabled(!running);
}

void MainWindow::onUpdateConfig()
{
    Config old = config;
    config.load();

    if (old.lang != config.lang)
    {
        QString msg = tr("The application will need to be restarted before all changes can take effect.");
        QMessageBox::information(this, tr("Restart required"), msg);
    }

    getMapView()->setConfig(config);
    if (old.uistyle != config.uistyle)
        onStyleChanged(config.uistyle);
    if (!old.biomeColorPath.isEmpty() || !config.biomeColorPath.isEmpty())
        onBiomeColorChange();

    if (config.autosaveCycle)
    {
        autosaveTimer.setInterval(config.autosaveCycle * 60000);
        autosaveTimer.start();
    }
    else
    {
        autosaveTimer.stop();
    }
}

void MainWindow::onUpdateMapConfig()
{
    MapConfig old = mconfig;
    mconfig.load();

    if (!old.equals(mconfig))
    {
        for (int opt = D_GRID; opt < D_STRUCT_NUM; opt++)
        {
            if (saction[opt] && mconfig.valid(opt))
                saction[opt]->setVisible(mconfig.enabled(opt));
        }
        getMapView()->deleteWorld();
        updateMapSeed();
    }
}

void MainWindow::onBiomeColorChange()
{
    QFile file(config.biomeColorPath);
    if (!config.biomeColorPath.isEmpty() && file.open(QIODevice::ReadOnly))
    {
        char buf[32*1024];
        qint64 siz = file.read(buf, sizeof(buf)-1);
        file.close();
        if (siz >= 0)
        {
            buf[siz] = 0;
            initBiomeColors(g_biomeColors);
            parseBiomeColors(g_biomeColors, buf);
        }
    }
    else
    {
        initBiomeColors(g_biomeColors);
    }
    getMapView()->refreshBiomeColors();
    ISaveTab *tab = dynamic_cast<ISaveTab*>(ui->tabContainer->currentWidget());
    if (tab)
        tab->refresh();
}

void MainWindow::onStyleChanged(int style)
{
    //QCoreApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, false);
    if (style == STYLE_DARK)
    {
        QFile file(":dark.qss");
        file.open(QFile::ReadOnly);
        QString st = file.readAll();
        qApp->setStyleSheet(st);
    }
    else
    {
        qApp->setStyleSheet("");
        qApp->setStyle("");
    }
}

void MainWindow::onDockFloating(bool floating)
{
    if (!floating)
    {
        setDockable(false);
        ui->actionDock->setText(tr("Undock map"));
    }
}

