#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "quadlistdialog.h"
#include "presetdialog.h"
#include "aboutdialog.h"
#include "conditiondialog.h"
#include "extgendialog.h"
#include "biomecolordialog.h"
#include "structuredialog.h"
#include "exportdialog.h"

#if WITH_UPDATER
#include "updater.h"
#endif

#include "quad.h"
#include "cutil.h"

#include <QIntValidator>
#include <QMetaType>
#include <QtDebug>
#include <QDataStream>
#include <QMenu>
#include <QClipboard>
#include <QFont>
#include <QFileDialog>
#include <QTextStream>
#include <QSettings>
#include <QTreeWidget>
#include <QDateTime>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>


// Keep the extended generator settings in global scope, but we mainly need
// them in this file. (Pass through via pointer elsewhere.)
static ExtGenSettings g_extgen;

extern "C"
int getStructureConfig_override(int stype, int mc, StructureConfig *sconf)
{
    if U(mc == INT_MAX) // to check if override is enabled in cubiomes
        mc = 0;
    int ok = getStructureConfig(stype, mc, sconf);
    if (ok && g_extgen.saltOverride)
    {
        uint64_t salt = g_extgen.salts[stype];
        if (salt <= MASK48)
            sconf->salt = salt;
    }
    return ok;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , formCond()
    , formGen48()
    , formControl()
    , config()
    , prevdir(".")
    , autosaveTimer()
    , dimactions{}
    , dimgroup()
{
    ui->setupUi(this);

    formCond = new FormConditions(this);
    formGen48 = new FormGen48(this);
    formControl = new FormSearchControl(this);

    QList<QAction*> layeracts = ui->menuBiome_layer->actions();
    for (int i = 0; i < layeracts.size(); i++)
    {
        connect(layeracts[i], &QAction::toggled,
            [=](bool state) {
                this->onActionBiomeLayerSelect(state, layeracts[i], i);
            });
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
    ui->toolBar->addSeparator();

    saction.resize(STRUCT_NUM);
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
    addMapAction(D_OUTPOST, "outpost", tr("Show illager outposts"));
    addMapAction(D_ANCIENTCITY, "ancient_city", tr("Show ancient cities"));
    addMapAction(D_PORTAL, "portal", tr("Show ruined portals"));
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

    QIntValidator *intval = new QIntValidator(this);
    ui->lineRadius->setValidator(intval);
    ui->lineEditX1->setValidator(intval);
    ui->lineEditZ1->setValidator(intval);
    ui->lineEditX2->setValidator(intval);
    ui->lineEditZ2->setValidator(intval);
    on_checkArea_toggled(false);

    ui->comboY->lineEdit()->setValidator(new QIntValidator(-64, 320, this));

    formCond->updateSensitivity();

    connect(&autosaveTimer, &QTimer::timeout, this, &MainWindow::onAutosaveTimeout);

    ui->treeAnalysis->sortByColumn(0, Qt::AscendingOrder);

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
        "Conditions can reference each other to produce relative positionial "
        "dependencies (indicated with the ID in square brackets [XY]). These "
        "will usually be checked at the geometric <b>average position</b> of the "
        "parent trigger. When multiple trigger positions are encountered in the "
        "same seed <b>and the required instance count is exactly one</b>, the "
        "instances are checked individually instead."
        "</p><p>"
        "Biome conditions <b>do not have trigger instances</b> and always yield "
        "the center point of the testing area. You can use reference point "
        "helpers to construct relative biome dependencies."
        "</p></body></html>"
    ));

    // 48-bit generator settings are not all that interesting unless we are
    // using them, so start as collapsed if they are on the "Auto" setting.
    Gen48Settings gen48 = formGen48->getSettings(false);
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
    return ui->mapView;
}

bool MainWindow::getSeed(WorldInfo *wi, bool applyrand)
{
    bool ok = true;
    const std::string& mcs = ui->comboBoxMC->currentText().toStdString();
    wi->mc = str2mc(mcs.c_str());
    if (wi->mc < 0)
    {
        wi->mc = MC_NEWEST;
        qDebug() << "Unknown MC version: " << wi->mc;
        ok = false;
    }

    int v = str2seed(ui->seedEdit->text(), &wi->seed);
    if (applyrand && v == S_RANDOM)
    {
        ui->seedEdit->setText(QString::asprintf("%" PRId64, (int64_t)wi->seed));
    }

    wi->large = ui->checkLarge->isChecked();
    wi->y = ui->comboY->currentText().section(' ', 0, 0).toInt();

    return ok;
}

bool MainWindow::setSeed(WorldInfo wi, int dim, int layeropt)
{
    const char *mcstr = mc2str(wi.mc);
    if (!mcstr)
    {
        qDebug() << "Unknown MC version: " << wi.mc;
        return false;
    }

    if (dim == INT_MAX)
        dim = getDim();

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
    ui->mapView->setSeed(wi, dim, layeropt);

    ui->checkLarge->setEnabled(wi.mc >= MC_1_3);
    ui->comboY->setEnabled(wi.mc >= MC_1_16);
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
    QSettings settings("cubiomes-viewer", "cubiomes-viewer");

    settings.setValue("mainwindow/size", size());
    settings.setValue("mainwindow/pos", pos());
    settings.setValue("mainwindow/prevdir", prevdir);

    settings.setValue("analysis/structs", ui->checkStructs->isChecked());
    settings.setValue("analysis/biomes", ui->checkBiomes->isChecked());
    settings.setValue("analysis/conditions", ui->checkConditions->isChecked());
    settings.setValue("analysis/maponly", ui->checkMapOnly->isChecked());
    settings.setValue("analysis/customarea", ui->checkArea->isChecked());
    settings.setValue("analysis/x1", ui->lineEditX1->text().toInt());
    settings.setValue("analysis/z1", ui->lineEditZ1->text().toInt());
    settings.setValue("analysis/x2", ui->lineEditX2->text().toInt());
    settings.setValue("analysis/z2", ui->lineEditZ2->text().toInt());

    settings.setValue("config/smoothMotion", config.smoothMotion);
    settings.setValue("config/showBBoxes", config.showBBoxes);
    settings.setValue("config/restoreSession", config.restoreSession);
    settings.setValue("config/checkForUpdates", config.checkForUpdates);
    settings.setValue("config/autosaveCycle", config.autosaveCycle);
    settings.setValue("config/uistyle", config.uistyle);
    settings.setValue("config/maxMatching", config.maxMatching);
    settings.setValue("config/gridSpacing", config.gridSpacing);
    settings.setValue("config/mapCacheSize", config.mapCacheSize);
    settings.setValue("config/biomeColorPath", config.biomeColorPath);

    settings.setValue("world/saltOverride", g_extgen.saltOverride);
    for (int st = 0; st < FEATURE_NUM; st++)
    {
        uint64_t salt = g_extgen.salts[st];
        if (salt <= MASK48)
            settings.setValue(QString("world/salt_") + struct2str(st), (qulonglong)salt);
    }

    WorldInfo wi;
    getSeed(&wi, false);
    settings.setValue("map/mc", wi.mc);
    settings.setValue("map/large", wi.large);
    settings.setValue("map/seed", (qlonglong)wi.seed);
    settings.setValue("map/dim", getDim());
    settings.setValue("map/x", ui->mapView->getX());
    settings.setValue("map/y", wi.y);
    settings.setValue("map/z", ui->mapView->getZ());
    settings.setValue("map/scale", ui->mapView->getScale());
    for (int stype = 0; stype < STRUCT_NUM; stype++)
    {
        QString s = QString("map/show_") + mapopt2str(stype);
        settings.setValue(s, ui->mapView->getShow(stype));
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


static void loadCheck(QSettings *s, QCheckBox *cb, const char *key)
{
    cb->setChecked( s->value(key, cb->isChecked()).toBool() );
}

static void loadLine(QSettings *s, QLineEdit *line, const char *key)
{
    // only load as integer
    qlonglong x = line->text().toLongLong();
    line->setText( QString::number(s->value(key, x).toLongLong()) );
}

void MainWindow::loadSettings()
{
    QSettings settings("cubiomes-viewer", "cubiomes-viewer");

    resize(settings.value("mainwindow/size", size()).toSize());
    move(settings.value("mainwindow/pos", pos()).toPoint());
    prevdir = settings.value("mainwindow/prevdir", pos()).toString();

    loadCheck(&settings, ui->checkStructs, "analysis/structs");
    loadCheck(&settings, ui->checkBiomes, "analysis/biomes");
    loadCheck(&settings, ui->checkConditions, "analysis/conditions");
    loadCheck(&settings, ui->checkMapOnly, "analysis/maponly");
    loadCheck(&settings, ui->checkArea, "analysis/customarea");
    loadLine(&settings, ui->lineEditX1, "analysis/x1");
    loadLine(&settings, ui->lineEditZ1, "analysis/z1");
    loadLine(&settings, ui->lineEditX2, "analysis/x2");
    loadLine(&settings, ui->lineEditZ2, "analysis/z2");

    config.smoothMotion = settings.value("config/smoothMotion", config.smoothMotion).toBool();
    config.showBBoxes = settings.value("config/showBBoxes", config.showBBoxes).toBool();
    config.restoreSession = settings.value("config/restoreSession", config.restoreSession).toBool();
    config.checkForUpdates = settings.value("config/checkForUpdates", config.checkForUpdates).toBool();
    config.autosaveCycle = settings.value("config/autosaveCycle", config.autosaveCycle).toInt();
    config.uistyle = settings.value("config/uistyle", config.uistyle).toInt();
    config.maxMatching = settings.value("config/maxMatching", config.maxMatching).toInt();
    config.gridSpacing = settings.value("config/gridSpacing", config.gridSpacing).toInt();
    config.mapCacheSize = settings.value("config/mapCacheSize", config.mapCacheSize).toInt();
    config.biomeColorPath = settings.value("config/biomeColorPath", config.biomeColorPath).toString();

    if (!config.biomeColorPath.isEmpty())
        onBiomeColorChange();

    ui->mapView->setConfig(config);
    onStyleChanged(config.uistyle);

    g_extgen.saltOverride = settings.value("world/saltOverride", g_extgen.saltOverride).toBool();
    for (int st = 0; st < FEATURE_NUM; st++)
    {
        QVariant v = QVariant::fromValue(~(qulonglong)0);
        g_extgen.salts[st] = settings.value(QString("world/salt_") + struct2str(st), v).toULongLong();
    }

    int dim = settings.value("map/dim", getDim()).toInt();
    if (dim == -1)
        dimactions[1]->setChecked(true);
    else if (dim == +1)
        dimactions[2]->setChecked(true);
    else
        dimactions[0]->setChecked(true);

    WorldInfo wi;
    getSeed(&wi, true);
    wi.mc = settings.value("map/mc", wi.mc).toInt();
    wi.large = settings.value("map/large", wi.large).toBool();
    wi.seed = (uint64_t) settings.value("map/seed", QVariant::fromValue((qlonglong)wi.seed)).toLongLong();
    wi.y = settings.value("map/y", 256).toInt();
    setSeed(wi);

    qreal x = ui->mapView->getX();
    qreal z = ui->mapView->getZ();
    qreal scale = ui->mapView->getScale();

    x = settings.value("map/x", x).toDouble();
    z = settings.value("map/z", z).toDouble();
    scale = settings.value("map/scale", scale).toDouble();

    for (int sopt = 0; sopt < STRUCT_NUM; sopt++)
    {
        bool show = ui->mapView->getShow(sopt);
        QString soptstr = QString("map/show_") + mapopt2str(sopt);
        show = settings.value(soptstr, show).toBool();
        if (saction[sopt])
            saction[sopt]->setChecked(show);
        ui->mapView->setShow(sopt, show);
    }
    mapGoto(x, z, scale);

    if (config.restoreSession)
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        path += "/session.save";
        if (QFile::exists(path))
        {
            loadProgress(path, false);
        }
    }

    if (config.autosaveCycle > 0)
    {
        autosaveTimer.setInterval(config.autosaveCycle * 60000);
        autosaveTimer.start();
    }
    else
    {
        autosaveTimer.stop();
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
    Gen48Settings gen48 = formGen48->getSettings(false);
    QVector<Condition> condvec = formCond->getConditions();
    QVector<uint64_t> results = formControl->getResults();

    WorldInfo wi;
    getSeed(&wi);

    QTextStream stream(&file);
    stream << "#Version:  " << VERS_MAJOR << "." << VERS_MINOR << "." << VERS_PATCH << "\n";
    stream << "#Time:     " << QDateTime::currentDateTime().toString() << "\n";
    // MC version of the session should take priority over the one in the settings
    stream << "#MC:       " << mc2str(wi.mc) << "\n";
    stream << "#Large:    " << wi.large << "\n";

    stream << "#Search:   " << searchconf.searchtype << "\n";
    if (!searchconf.slist64path.isEmpty())
        stream << "#List64:   " << searchconf.slist64path.replace("\n", "") << "\n";
    stream << "#Progress: " << searchconf.startseed << "\n";
    stream << "#Threads:  " << searchconf.threads << "\n";
    stream << "#ResStop:  " << (int)searchconf.stoponres << "\n";

    stream << "#Mode48:   " << gen48.mode << "\n";
    if (!gen48.slist48path.isEmpty())
        stream << "#List48:   " << gen48.slist48path.replace("\n", "") << "\n";
    stream << "#HutQual:  " << gen48.qual << "\n";
    stream << "#MonArea:  " << gen48.qmarea << "\n";
    if (gen48.salt != 0)
        stream << "#Salt:     " << gen48.salt << "\n";
    if (gen48.listsalt != 0)
        stream << "#LSalt:    " << gen48.listsalt << "\n";
    if (gen48.manualarea)
    {
        stream << "#Gen48X1:  " << gen48.x1 << "\n";
        stream << "#Gen48Z1:  " << gen48.z1 << "\n";
        stream << "#Gen48X2:  " << gen48.x2 << "\n";
        stream << "#Gen48Z2:  " << gen48.z2 << "\n";
    }
    if (searchconf.smin != 0)
        stream << "#SMin:     " << searchconf.smin << "\n";
    if (searchconf.smax != ~(uint64_t)0)
        stream << "#SMax:     " << searchconf.smax << "\n";

    for (Condition &c : condvec)
        stream << "#Cond: " << c.toHex() << "\n";

    for (uint64_t s : results)
        stream << QString::asprintf("%" PRId64 "\n", (int64_t)s);

    return true;
}

bool MainWindow::loadProgress(QString fnam, bool quiet)
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
    Gen48Settings gen48 = formGen48->getSettings(false);
    QVector<Condition> condvec;
    QVector<uint64_t> seeds;

    char buf[4096];
    int tmp;
    WorldInfo wi;
    getSeed(&wi, true);

    QTextStream stream(&file);
    QString line;
    line = stream.readLine();
    int lno = 1;

    if (sscanf(line.toLatin1().data(), "#Version: %d.%d.%d", &major, &minor, &patch) != 3)
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
            "Continue anyway?"),
            QMessageBox::Abort|QMessageBox::Yes);
        if (button == QMessageBox::Abort)
            return false;
    }

    while (stream.status() == QTextStream::Ok && !stream.atEnd())
    {
        lno++;
        line = stream.readLine();
        QByteArray ba = line.toLatin1();
        const char *p = ba.data();

        if (line.isEmpty()) continue;
        if (line.startsWith("#Time:")) continue;
        if (line.startsWith("#Title:")) continue;
        if (line.startsWith("#Desc:")) continue;
        else if (sscanf(p, "#MC:       %8[^\n]", buf) == 1)                     { wi.mc = str2mc(buf); if (wi.mc < 0) wi.mc = MC_NEWEST; }
        else if (sscanf(p, "#Large:    %d", &tmp) == 1)                         { wi.large = tmp; }
        // SearchConfig
        else if (sscanf(p, "#Search:   %d", &searchconf.searchtype) == 1)       {}
        else if (sscanf(p, "#Progress: %" PRId64, &searchconf.startseed) == 1)  {}
        else if (sscanf(p, "#Threads:  %d", &searchconf.threads) == 1)          {}
        else if (sscanf(p, "#ResStop:  %d", &tmp) == 1)                         { searchconf.stoponres = tmp; }
        else if (line.startsWith("#List64:   "))                                { searchconf.slist64path = line.mid(11).trimmed(); }
        // Gen48Settings
        else if (sscanf(p, "#Mode48:   %d", &gen48.mode) == 1)                  {}
        else if (sscanf(p, "#HutQual:  %d", &gen48.qual) == 1)                  {}
        else if (sscanf(p, "#MonArea:  %d", &gen48.qmarea) == 1)                {}
        else if (sscanf(p, "#Salt:     %" PRIu64, &gen48.salt) == 1)            {}
        else if (sscanf(p, "#LSalt:    %" PRIu64, &gen48.listsalt) == 1)        {}
        else if (sscanf(p, "#Gen48X1:  %d", &gen48.x1) == 1)                    { gen48.manualarea = true; }
        else if (sscanf(p, "#Gen48Z1:  %d", &gen48.z1) == 1)                    { gen48.manualarea = true; }
        else if (sscanf(p, "#Gen48X2:  %d", &gen48.x2) == 1)                    { gen48.manualarea = true; }
        else if (sscanf(p, "#Gen48Z2:  %d", &gen48.z2) == 1)                    { gen48.manualarea = true; }
        else if (line.startsWith("#List48:   "))                                { gen48.slist48path = line.mid(11).trimmed(); }
        else if (sscanf(p, "#SMin:     %" PRIu64, &searchconf.smin) == 1)       {}
        else if (sscanf(p, "#SMax:     %" PRIu64, &searchconf.smax) == 1)       {}
        else if (line.startsWith("#Cond:"))
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
            uint64_t s;
            if (sscanf(line.toLatin1().data(), "%" PRId64, (int64_t*)&s) == 1)
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

    formGen48->setSettings(gen48, quiet);
    formGen48->updateCount();
    formControl->on_buttonClear_clicked();
    formControl->setSearchConfig(searchconf, quiet);
    formControl->searchResultsAdd(seeds, false);
    formControl->searchProgressReset();

    return true;
}


void MainWindow::updateMapSeed()
{
    WorldInfo wi;
    if (getSeed(&wi))
        setSeed(wi);
    emit mapUpdated();
}


int MainWindow::warning(QString text, QMessageBox::StandardButtons buttons)
{
    return QMessageBox::warning(this, tr("Warning"), text, buttons);
}

void MainWindow::mapGoto(qreal x, qreal z, qreal scale)
{
    ui->mapView->setView(x, z, scale);
}

void MainWindow::setBiomeColorRc(QString rc)
{
    config.biomeColorPath = rc;
    onBiomeColorChange();
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
void MainWindow::on_checkLarge_toggled()
{
    updateMapSeed();
    update();
}
void MainWindow::on_comboY_currentIndexChanged(int)
{
    updateMapSeed();
    update();
}

void MainWindow::on_seedEdit_textChanged(const QString &a)
{
    uint64_t s;
    int v = str2seed(a, &s);
    QString typ = "";
    switch (v)
    {
    case S_TEXT:    typ = tr("text", "Seed input type"); break;
    case S_NUMERIC: typ = ""; break;
    case S_RANDOM:  typ = tr("random", "Seed input type"); break;
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
        loadProgress(fnam, false);
    }
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::on_actionPreferences_triggered()
{
    ConfigDialog *dialog = new ConfigDialog(this, &config);
    int status = dialog->exec();
    if (status == QDialog::Accepted)
    {
        Config oldConfig = config;
        config = dialog->getSettings();
        ui->mapView->setConfig(config);
        if (oldConfig.uistyle != config.uistyle)
            onStyleChanged(config.uistyle);

        if (config.autosaveCycle)
        {
            autosaveTimer.setInterval(config.autosaveCycle * 60000);
            autosaveTimer.start();
        }
        else
        {
            autosaveTimer.stop();
        }

        if (!config.biomeColorPath.isEmpty() || !oldConfig.biomeColorPath.isEmpty())
        {
            onBiomeColorChange();
        }
    }
    if (dialog->structVisModified)
    {   // NOTE: structure visibility limits are not currently stored in config
        // so the changes have to be applied regardless whether the dialog is accepted.
        on_actionStructure_visibility_triggered();
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
            initBiomeColors(biomeColors);
            parseBiomeColors(biomeColors, buf);
        }
    }
    else
    {
        initBiomeColors(biomeColors);
    }
    ui->mapView->refreshBiomeColors();
}

void MainWindow::on_actionGo_to_triggered()
{
    ui->mapView->onGoto();
}

void MainWindow::on_actionScan_seed_for_Quad_Huts_triggered()
{
    QuadListDialog *dialog = new QuadListDialog(this);
    dialog->show();
}

void MainWindow::on_actionOpen_shadow_seed_triggered()
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
    if (dialog->exec() != QDialog::Accepted || !dialog->modified)
        return;
    saveStructVis(dialog->structvis);
    getMapView()->deleteWorld();
    updateMapSeed();
    update();
}

void MainWindow::on_actionBiome_colors_triggered()
{
    BiomeColorDialog *dialog = new BiomeColorDialog(this, config.biomeColorPath);
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
        loadProgress(dialog->rc);
}

void MainWindow::on_actionExamples_triggered()
{
    WorldInfo wi;
    getSeed(&wi);
    PresetDialog *dialog = new PresetDialog(this, wi, true);
    dialog->setActiveFilter(formCond->getConditions());
    if (dialog->exec() && !dialog->rc.isEmpty())
        loadProgress(dialog->rc);
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
        updateMapSeed();
        update();
    }
}

void MainWindow::on_actionExportImg_triggered()
{
    ExportDialog *dialog = new ExportDialog(this);
    dialog->show();
}


void MainWindow::on_checkArea_toggled(bool checked)
{
    ui->labelSquareArea->setEnabled(!checked);
    ui->lineRadius->setEnabled(!checked);
    ui->labelX1->setEnabled(checked);
    ui->labelZ1->setEnabled(checked);
    ui->labelX2->setEnabled(checked);
    ui->labelZ2->setEnabled(checked);
    ui->lineEditX1->setEnabled(checked);
    ui->lineEditZ1->setEnabled(checked);
    ui->lineEditX2->setEnabled(checked);
    ui->lineEditZ2->setEnabled(checked);
}

void MainWindow::on_lineRadius_editingFinished()
{
    on_buttonAnalysis_clicked();
}

void MainWindow::on_buttonFromVisible_clicked()
{
    qreal uiw = ui->mapView->width() * ui->mapView->getScale();
    qreal uih = ui->mapView->height() * ui->mapView->getScale();
    int bx0 = (int) floor(ui->mapView->getX() - uiw/2);
    int bz0 = (int) floor(ui->mapView->getZ() - uih/2);
    int bx1 = (int) ceil(ui->mapView->getX() + uiw/2);
    int bz1 = (int) ceil(ui->mapView->getZ() + uih/2);

    ui->checkArea->setChecked(true);
    ui->lineEditX1->setText( QString::number(bx0) );
    ui->lineEditZ1->setText( QString::number(bz0) );
    ui->lineEditX2->setText( QString::number(bx1) );
    ui->lineEditZ2->setText( QString::number(bz1) );
}

static
QTreeWidgetItem *setConditionTreeItems(ConditionTree& ctree, int node, Pos cpos[], QTreeWidgetItem* parent)
{
    Condition& c = ctree.condvec[node];
    Pos p = cpos[c.save];
    const std::vector<char>& branches = ctree.references[c.save];

    QTreeWidgetItem* item = new QTreeWidgetItem(parent);
    item->setText(0, c.summary());
    item->setData(0, Qt::UserRole, QVariant::fromValue(p));

    if (branches.empty())
    {
        item->setText(1, MainWindow::tr("incomplete"));
    }
    else
    {
        item->setText(1, QString::asprintf("%d,\t%d", p.x, p.z));
        for (char b : branches)
            setConditionTreeItems(ctree, b, cpos, item);
    }
    return item;
}

void MainWindow::on_buttonAnalysis_clicked()
{
    int x1, z1, x2, z2;

    if (ui->lineRadius->isEnabled())
    {
        int d = ui->lineRadius->text().toInt();
        x1 = (-d) >> 1;
        z1 = (-d) >> 1;
        x2 = (d) >> 1;
        z2 = (d) >> 1;
    }
    else
    {
        x1 = ui->lineEditX1->text().toInt();
        z1 = ui->lineEditZ1->text().toInt();
        x2 = ui->lineEditX2->text().toInt();
        z2 = ui->lineEditZ2->text().toInt();
    }
    if (x2 < x1) std::swap(x1, x2);
    if (z2 < z1) std::swap(z1, z2);

    bool ck_struct = ui->checkStructs->isChecked();
    bool ck_biome = ui->checkBiomes->isChecked();
    bool ck_conds = ui->checkConditions->isChecked();
    bool map_only = ui->checkMapOnly->isChecked();

    QTreeWidget *tree = ui->treeAnalysis;
    while (tree->topLevelItemCount() > 0)
        delete tree->takeTopLevelItem(0);


    WorldInfo wi;
    if (!getSeed(&wi))
        return;

    QVector<Condition> conds;
    if (ck_conds)
        conds = formCond->getConditions();

    uint64_t areasiz = (uint64_t)(x2 - x1) * (uint64_t)(z2 - z1);
    int64_t cstepx = 1, cstepz = 1;

    bool areawarn = false;
    if (ck_struct && areasiz > 1e10)
        areawarn = true;
    if (ck_biome && areasiz > 1e8)
        areawarn = true;
    if (ck_conds && !conds.empty())
    {
        Condition *c = &conds[0];
        const FilterInfo& finfo = g_filterinfo.list[c->type];
        cstepx = (c->x2 - c->x1);
        cstepz = (c->z2 - c->z1);

        if (cstepx < 1) cstepx = 1;
        if (cstepz < 1) cstepz = 1;

        if (finfo.step > 1)
        {
            cstepx *= finfo.step;
            cstepz *= finfo.step;
        }
        else if (finfo.stype > 0)
        {
            if (cstepx < 16) cstepx = 16;
            if (cstepz < 16) cstepz = 16;
        }
    }

    if (areawarn)
    {
        QString msg = tr(
                "Area for analysis is very large (%1, %2).\n"
                "The analysis might take a while. Do you want to continue?")
                .arg(x2-x1+1).arg(z2-z1+1);
        int button = QMessageBox::warning(
            this, tr("Warning"), msg, QMessageBox::Cancel|QMessageBox::Yes);
        if (button != QMessageBox::Yes)
            return;

        // update to close message box
        update();
        QApplication::processEvents();
    }

    ui->buttonAnalysis->setEnabled(false);


    int dim = getDim();
    Generator g;
    setupGenerator(&g, wi.mc, wi.large);

    if (ck_biome)
    {
        const int step = 512;
        long idcnt[256] = {0};

        for (int x = x1; x <= x2; x += step)
        {
            for (int z = z1; z <= z2; z += step)
            {
                int w = x2-x+1 < step ? x2-x+1 : step;
                int h = z2-z+1 < step ? z2-z+1 : step;
                Range r = {1, x, z, w, h, wi.y, 1};
                int *ids = allocCache(&g, r);

                int dims[] = {0, -1, +1};
                for (int d = 0; d < 3; d++)
                {
                    if (dims[d] == dim || !map_only)
                    {
                        applySeed(&g, dims[d], wi.seed);
                        genBiomes(&g, ids, r);
                        for (int i = 0; i < w*h; i++)
                            idcnt[ ids[i] & 0xff ]++;
                    }
                }
                free(ids);
            }
        }

        int bcnt = 0;
        for (int i = 0; i < 256; i++)
            bcnt += !!idcnt[i];

        QTreeWidgetItem* item_cat = new QTreeWidgetItem(tree);
        item_cat->setText(0, tr("biomes", "Analysis ID"));
        item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(bcnt));

        for (int id = 0; id < 256; id++)
        {
            long cnt = idcnt[id];
            if (cnt <= 0)
                continue;
            const char *s;
            if (!(s = biome2str(wi.mc, id)))
                continue;
            QTreeWidgetItem* item =
                new QTreeWidgetItem(item_cat, QTreeWidgetItem::UserType + id);
            item->setText(0, s);
            item->setData(1, Qt::DisplayRole, QVariant::fromValue(cnt));
        }
    }

    if (ck_struct)
    {
        std::vector<VarPos> st;
        for (int sopt = D_DESERT; sopt < D_SPAWN; sopt++)
        {
            int sdim = 0;
            if (sopt == D_FORTESS || sopt == D_BASTION || sopt == D_PORTALN)
                sdim = -1;
            if (sopt == D_ENDCITY || sopt == D_GATEWAY)
                sdim = 1;
            if (map_only)
            {
                if (!getMapView()->getShow(sopt))
                    continue;
                if (sdim != dim)
                    continue;
            }
            int stype = mapopt2stype(sopt);
            st.clear();
            StructureConfig sconf;
            if (!getStructureConfig_override(stype, wi.mc, &sconf))
                continue;
            getStructs(&st, sconf, wi, sdim, x1, z1, x2, z2);
            if (st.empty())
                continue;

            QTreeWidgetItem* item_cat = new QTreeWidgetItem(tree);
            const char *s = struct2str(stype);
            item_cat->setText(0, s);
            item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(st.size()));

            for (size_t i = 0; i < st.size(); i++)
            {
                VarPos vp = st[i];
                QTreeWidgetItem* item = new QTreeWidgetItem(item_cat);
                item->setData(0, Qt::UserRole, QVariant::fromValue(vp.p));
                item->setText(0, QString::asprintf("%d,\t%d", vp.p.x, vp.p.z));
                if (vp.v.abandoned)
                {
                    if (stype == Village)
                        item->setText(1, tr("abandoned", "Village variant"));
                }
            }
        }

        if ((dim == 0 && getMapView()->getShow(D_SPAWN)) || !map_only)
        {
            applySeed(&g, 0, wi.seed);
            Pos pos = getSpawn(&g);
            if (pos.x >= x1 && pos.x <= x2 && pos.z >= z1 && pos.z <= z2)
            {
                QTreeWidgetItem* item_cat = new QTreeWidgetItem(tree);
                item_cat->setText(0, tr("spawn", "Analysis ID"));
                item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(1));
                QTreeWidgetItem* item = new QTreeWidgetItem(item_cat);
                item->setData(0, Qt::UserRole, QVariant::fromValue(pos));
                item->setText(0, QString::asprintf("%d,\t%d", pos.x, pos.z));
            }
        }

        if ((dim == 0 && getMapView()->getShow(D_STRONGHOLD)) || !map_only)
        {
            StrongholdIter sh;
            initFirstStronghold(&sh, wi.mc, wi.seed);
            std::vector<Pos> shp;
            applySeed(&g, dim, wi.seed);
            while (nextStronghold(&sh, &g) > 0)
            {
                Pos pos = sh.pos;
                if (pos.x >= x1 && pos.x <= x2 && pos.z >= z1 && pos.z <= z2)
                    shp.push_back(pos);
            }

            if (!shp.empty())
            {
                QTreeWidgetItem* item_cat = new QTreeWidgetItem(tree);
                item_cat->setText(0, tr("stronghold", "Analysis ID"));
                item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(shp.size()));
                for (Pos pos : shp)
                {
                    QTreeWidgetItem* item = new QTreeWidgetItem(item_cat);
                    item->setData(0, Qt::UserRole, QVariant::fromValue(pos));
                    item->setText(0, QString::asprintf("%d,\t%d", pos.x, pos.z));
                }
            }
        }
    }

    QElapsedTimer timer;
    timer.start();
    int64_t warn_ms = 1000;

    if (ck_conds && !conds.empty())
    {
        WorldGen gen;
        gen.init(wi.mc, wi.large);
        gen.setSeed(wi.seed);

        ConditionTree condtree;
        condtree.set(conds, wi);

        //Condition& c0 = conds[0];
        int xr1 = 0; //(int)( cstepx * floor( (x1+c0.x1) / (double)cstepx ) );
        int zr1 = 0; //(int)( cstepz * floor( (z1+c0.z1) / (double)cstepz ) );
        int xr2 = 0; //(int)( cstepx * ceil( (x2+c0.x2) / (double)cstepx ) );
        int zr2 = 0; //(int)( cstepz * ceil( (z2+c0.z2) / (double)cstepz ) );

        QList<QTreeWidgetItem*> items;

        for (int x = xr1; x <= xr2; x += cstepx)
        {
            //if (x + c0.x1 > x2 || x + c0.x2 < x1)
            //    continue;
            for (int z = zr1; z <= zr2; z += cstepz)
            {
                //if (z + c0.z1 > z2 || z + c0.z2 < z1)
                //    continue;

                if (timer.elapsed() > warn_ms)
                {
                    QString msg = tr(
                            "The search is taking some time.\n"
                            "Locations found so far: %1\n"
                            "Continue search?")
                            .arg(items.size());
                    int button = QMessageBox::warning(
                        this, tr("Warning"), msg, QMessageBox::Abort|QMessageBox::Yes);
                    if (button != QMessageBox::Yes)
                        goto L_scan_end;
                    update();
                    QApplication::processEvents();
                    warn_ms *= 4;
                    timer.start();
                }

                Pos origin = {x, z};
                Pos cpos[MAX_INSTANCES] = {};
                std::atomic_bool ab;
                ab = false;
                if (testTreeAt(origin, &condtree, PASS_FULL_64, &gen, &ab, cpos)
                    != COND_OK)
                {
                    continue;
                }

                QTreeWidgetItem* loc = setConditionTreeItems(condtree, 0, cpos, nullptr);
                //new QTreeWidgetItem();
                //loc->setData(0, Qt::UserRole, QVariant::fromValue(origin));
                //loc->setText(0, tr("condition @[%1, %2]").arg(origin.x).arg(origin.z));


                items.push_back(loc);

                if (items.size() >= 65536)
                {
                    warning(tr("Reached maximum number of results (%1).\n"
                        "Stopping search.").arg(items.size()));
                    goto L_scan_end;
                }
            }
        }
L_scan_end:

        if (!items.empty())
        {
            /*
            QTreeWidgetItem* item_cat = new QTreeWidgetItem(tree);
            item_cat->setText(0, "conditions");
            item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(items.size()));
            item_cat->addChildren(items);
            */
            tree->addTopLevelItems(items);
        }
    }

    ui->buttonExport->setEnabled(true);
    ui->buttonAnalysis->setEnabled(true);
}

void MainWindow::on_buttonExport_clicked()
{
    QString fnam = QFileDialog::getSaveFileName(
        this, tr("Export analysis"), prevdir, tr("Text files (*.txt *csv);;Any files (*)"));
    if (fnam.isEmpty())
        return;

    QFileInfo finfo(fnam);
    QFile file(fnam);
    prevdir = finfo.absolutePath();

    if (!file.open(QIODevice::WriteOnly))
    {
        warning(tr("Failed to open file for export:\n\"%1\"").arg(fnam));
        return;
    }

    QTextStream stream(&file);

    QTreeWidgetItemIterator it(ui->treeAnalysis);
    for (; *it; ++it)
    {
        QTreeWidgetItem *item = *it;
        QStringList cols;
        if (item->type() >= QTreeWidgetItem::UserType)
            cols << QString::number(item->type() - QTreeWidgetItem::UserType);
        for (int i = 0; i <= 1; i++)
        {
            QString txt = item->text(i).replace('\t', ' ');
            if (txt.isEmpty())
                continue;
            if (txt.contains("["))
                txt = "\"" + txt + "\"";
            cols << txt;
        }
        stream << cols.join(", ") << "\n";
    }
}

void MainWindow::on_treeAnalysis_itemClicked(QTreeWidgetItem *item)
{
    QVariant dat = item->data(0, Qt::UserRole);
    if (dat.isValid())
    {
        Pos p = qvariant_cast<Pos>(dat);
        ui->mapView->setView(p.x+0.5, p.z+0.5);
    }
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

void MainWindow::onActionMapToggled(int sopt, bool show)
{
    if (sopt == D_PORTAL) // overworld porals should also control nether
        ui->mapView->setShow(D_PORTALN, show);
    ui->mapView->setShow(sopt, show);
}

void MainWindow::onActionBiomeLayerSelect(bool state, QAction *src, int lopt)
{
    if (state == false)
        return;
    const QList<QAction*> actions = ui->menuBiome_layer->actions();
    for (QAction *act : actions)
        if (act != src)
            act->setChecked(false);
    WorldInfo wi;
    if (getSeed(&wi))
        setSeed(wi, INT_MAX, lopt);
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
    ui->seedEdit->setText(QString::asprintf("%" PRId64, (int64_t)seed));
    on_seedEdit_editingFinished();
}

void MainWindow::onSearchStatusChanged(bool running)
{
    formGen48->setEnabled(!running);
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


