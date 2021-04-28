#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>
#include <QWidget>

#include <QTimer>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QVector>

#include <atomic>

#include "searchthread.h"
#include "protobasedialog.h"
#include "configdialog.h"

namespace Ui {
class MainWindow;
}

Q_DECLARE_METATYPE(int64_t)
Q_DECLARE_METATYPE(uint64_t)
Q_DECLARE_METATYPE(Pos)
Q_DECLARE_METATYPE(Condition)
Q_DECLARE_METATYPE(Config)

class MapView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QAction *addMapAction(int stype, const char *iconpath, const char *tip);

    bool getSeed(int *mc, int64_t *seed, bool applyrand = true);
    bool setSeed(int mc, int64_t seed);
    QVector<Condition> getConditions() const;
    MapView *getMapView();

protected:
    void saveSettings();
    void loadSettings();
    bool saveProgress(QString fnam, bool quiet = false);
    bool loadProgress(QString fnam, bool quiet = false);
    QListWidgetItem *lockItem(QListWidgetItem *item);
    void setItemCondition(QListWidget *list, QListWidgetItem *item, Condition *cond);
    void editCondition(QListWidgetItem *item);
    void updateMapSeed();
    void updateSensitivity();
    int getIndex(int idx) const;

public slots:
    void warning(QString title, QString text);
    void mapGoto(qreal x, qreal z, qreal scale);
    void openProtobaseMsg(QString path);
    void closeProtobaseMsg();

    int searchResultsAdd(QVector<int64_t> seeds, bool countonly);

private slots:
    void on_comboBoxMC_currentIndexChanged(int a);
    void on_seedEdit_editingFinished();
    void on_seedEdit_textChanged(const QString &arg1);

    void on_buttonRemoveAll_clicked();
    void on_buttonRemove_clicked();
    void on_buttonEdit_clicked();
    void on_buttonAddFilter_clicked();

    void on_listConditions48_itemDoubleClicked(QListWidgetItem *item);
    void on_listConditionsFull_itemDoubleClicked(QListWidgetItem *item);
    void on_listConditions48_itemSelectionChanged();
    void on_listConditionsFull_itemSelectionChanged();

    void on_buttonClear_clicked();
    void on_buttonStart_clicked();

    void on_listResults_itemSelectionChanged();
    void on_listResults_customContextMenuRequested(const QPoint &pos);

    void on_buttonInfo_clicked();
    void on_buttonSearchHelp_clicked();

    void on_actionSave_triggered();
    void on_actionLoad_triggered();
    void on_actionQuit_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionPreferences_triggered();
    void on_actionGo_to_triggered();
    void on_actionScan_seed_for_Quad_Huts_triggered();
    void on_actionOpen_shadow_seed_triggered();
    void on_actionAbout_triggered();

    void on_actionSearch_seed_list_triggered();
    void on_actionSearch_full_seed_space_triggered();
    void on_comboSearchType_currentIndexChanged(int index);
    void on_buttonLoadList_clicked();

    void on_mapView_customContextMenuRequested(const QPoint &pos);

    void on_cboxArea_toggled(bool checked);
    void on_lineRadius_editingFinished();
    void on_buttonFromVisible_clicked();
    void on_buttonAnalysis_clicked();
    void on_treeAnalysis_itemDoubleClicked(QTreeWidgetItem *item);
    void on_buttonExport_clicked();

    // internal events

    void onActionMapToggled(int stype, bool a);
    void addItemCondition(QListWidgetItem *item, Condition cond);
    void searchProgress(uint64_t last, uint64_t end, int64_t seed);
    void searchFinish();
    void resultTimeout();
    void removeCurrent();
    void copyResults();
    void pasteResults();
    int pasteList(bool dummy = false);
    void copyCoord();


public:
    Ui::MainWindow *ui;
    QVector<QAction*> saction;
    SearchThread sthread;
    QTimer stimer;
    ProtoBaseDialog *protodialog;
    QString prevdir;
    QString slistfnam;
    std::vector<int64_t> slist64;
    Config config;
};

#endif // MAINWINDOW_H
