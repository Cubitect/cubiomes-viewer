#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>
#include <QWidget>
#include <QAction>
#include <QActionGroup>
#include <QMessageBox>

#include <QTimer>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QVector>
#include <QSettings>

#include <atomic>

#include "mapview.h"
#include "searchthread.h"
#include "configdialog.h"
#include "formconditions.h"
#include "formgen48.h"
#include "formsearchcontrol.h"

namespace Ui {
class MainWindow;
}

struct ISaveTab
{
    virtual void save(QSettings& settings) = 0;
    virtual void load(QSettings& settings) = 0;
    virtual void refresh() {}
};

class MapView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString sessionpath, QString resultspath, QWidget *parent = 0);
    virtual ~MainWindow();
    virtual void closeEvent(QCloseEvent *event) override;

    bool loadTranslation(QString lang);

    QAction *addMapAction(int opt);
    QAction *addMapAction(QString rcbase, QString tip);

    bool getSeed(WorldInfo *wi, bool applyrand = true);
    bool setSeed(WorldInfo wi, int dim = DIM_UNDEF);
    int getDim();
    MapView *getMapView();

protected:
    void saveSettings();
    void loadSettings();
    bool saveSession(QString fnam, bool quiet = false);
    bool loadSession(QString fnam, bool keepresults, bool quiet);
    void updateMapSeed();
    void setDockable(bool dockable);
    void setMCList(bool experimental);

signals:
    void mapUpdated();

public slots:
    void mapGoto(qreal x, qreal z, qreal scale);
    void mapZoom(qreal factor);
    void setBiomeColorRc(QString rc);

private slots:
    void on_comboBoxMC_currentIndexChanged(int a);
    void on_seedEdit_editingFinished();
    void on_seedEdit_textChanged(const QString &arg1);
    void on_checkLarge_toggled();
    void on_comboY_currentIndexChanged(int index);

    void on_actionSave_triggered();
    void on_actionLoad_triggered();
    void on_actionQuit_triggered();
    void on_actionPreferences_triggered();
    void on_actionGoto_triggered();
    void on_actionOpenShadow_triggered();
    void on_actionToolbarConfig_triggered();
    void on_actionBiomeColors_triggered();
    void on_actionPresetLoad_triggered();
    void on_actionExamples_triggered();
    void on_actionAbout_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionAddShadow_triggered();
    void on_actionRedistribute_triggered();
    void on_actionExtGen_triggered();
    void on_actionExportImg_triggered();
    void on_actionScreenshot_triggered();
    void on_actionDock_triggered();
    void on_actionLayerDisplay_triggered();

    void on_tabContainer_currentChanged(int index);
    void on_tabContainerSearch_currentChanged(int index);

    void on_actionSearch_seed_list_triggered();
    void on_actionSearch_full_seed_space_triggered();

    // internal events
    void onAutosaveTimeout();
    void onActionMapToggled(int sopt, bool a);
    void onActionHistory(QAction *act);
    void onActionBiomeLayerSelect(int lopt, int disp = -1);
    void onConditionsChanged();
    void onConditionsSelect(const QVector<Condition>& selection);
    void onGen48Changed();
    void onSelectedSeedChanged(uint64_t seed);
    void onSearchStatusChanged(bool running);
    void onUpdateConfig();
    void onUpdateMapConfig();
    void onBiomeColorChange();
    void onStyleChanged(int style);
    void onDockFloating(bool floating);

public:
    Ui::MainWindow *ui;
    QDockWidget *dock;
    MapView *mapView;

    FormConditions *formCond;
    FormGen48 *formGen48;
    FormSearchControl *formControl;
    LayerOpt lopt;
    Config config;
    MapConfig mconfig;
    QString sessionpath;
    QString prevdir;
    QTimer autosaveTimer;
    int tabidx;
    int tabsearch;

    QVector<QAction*> laction;
    QVector<QAction*> saction;
    QAction *acthome;
    QAction *actzoom[2];
    QAction *dimactions[3];
    QActionGroup *dimgroup;
};

#endif // MAINWINDOW_H
