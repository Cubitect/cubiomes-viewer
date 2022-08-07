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

#include <atomic>

#include "analysis.h"
#include "mapview.h"
#include "searchthread.h"
#include "configdialog.h"
#include "formconditions.h"
#include "formgen48.h"
#include "formsearchcontrol.h"

namespace Ui {
class MainWindow;
}


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
    bool setSeed(WorldInfo wi, int dim = INT_MAX, int layeropt = -1);
    int getDim();
    MapView *getMapView();

protected:
    void saveSettings();
    void loadSettings();
    bool saveProgress(QString fnam, bool quiet = false);
    bool loadProgress(QString fnam, bool keepresults, bool quiet);
    void updateMapSeed();

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
    void on_actionScan_seed_for_Quad_Huts_triggered();
    void on_actionOpen_shadow_seed_triggered();
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

    void on_checkArea_toggled(bool checked);
    void on_lineRadius_editingFinished();
    void on_buttonFromVisible_clicked();
    void on_buttonAnalysis_clicked();
    void on_treeAnalysis_itemClicked(QTreeWidgetItem *item);
    void on_buttonExport_clicked();

    void on_actionSearch_seed_list_triggered();
    void on_actionSearch_full_seed_space_triggered();

    void on_actionDockable_toggled(bool dockable);

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
    void onAnalysisItemDone(QTreeWidgetItem *item);
    void onAnalysisFinished();

public:
    Ui::MainWindow *ui;
    QDockWidget *dock;
    MapView *mapView;
    Analysis analysis;

    FormConditions *formCond;
    FormGen48 *formGen48;
    FormSearchControl *formControl;
    Config config;
    QString prevdir;
    QTimer autosaveTimer;

    QVector<QAction*> saction;
    QAction *dimactions[3];
    QActionGroup *dimgroup;
};

#endif // MAINWINDOW_H
