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
    explicit MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();
    virtual void closeEvent(QCloseEvent *event) override;

    QAction *addMapAction(int sopt, const char *iconpath, QString tip);

    bool getSeed(WorldInfo *wi, bool applyrand = true);
    bool setSeed(WorldInfo wi, int dim = DIM_UNDEF, int layeropt = -1);
    int getDim();
    MapView *getMapView();

protected:
    void saveSettings();
    void loadSettings();
    bool saveProgress(QString fnam, bool quiet = false);
    bool loadProgress(QString fnam, bool keepresults, bool quiet);
    void updateMapSeed();
    void setDockable(bool dockable);
    void applyConfigChanges(const Config old, const Config conf);
    void setMCList(bool experimental);

signals:
    void mapUpdated();

public slots:
    int warning(QString text, QMessageBox::StandardButtons buttons = QMessageBox::Ok);
    void mapGoto(qreal x, qreal z, qreal scale);
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
    void on_actionGo_to_triggered();
    void on_actionOpenShadow_triggered();
    void on_actionStructure_visibility_triggered();
    void on_actionBiome_colors_triggered();
    void on_actionPresetLoad_triggered();
    void on_actionExamples_triggered();
    void on_actionAbout_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionAddShadow_triggered();
    void on_actionExtGen_triggered();
    void on_actionExportImg_triggered();
    void on_actionScreenshot_triggered();

    void on_tabContainer_currentChanged(int index);

    void on_actionSearch_seed_list_triggered();
    void on_actionSearch_full_seed_space_triggered();

    // internal events
    void onAutosaveTimeout();
    void onActionMapToggled(int sopt, bool a);
    void onActionBiomeLayerSelect(bool state, QAction *src, int lopt);
    void onConditionsChanged();
    void onGen48Changed();
    void onSelectedSeedChanged(uint64_t seed);
    void onSearchStatusChanged(bool running);
    void onStyleChanged(int style);
    void onBiomeColorChange();

public:
    Ui::MainWindow *ui;
    QDockWidget *dock;
    MapView *mapView;

    FormConditions *formCond;
    FormGen48 *formGen48;
    FormSearchControl *formControl;
    Config config;
    QString prevdir;
    QTimer autosaveTimer;
    int prevtab;

    QVector<QAction*> saction;
    QAction *dimactions[3];
    QActionGroup *dimgroup;
};

#endif // MAINWINDOW_H
