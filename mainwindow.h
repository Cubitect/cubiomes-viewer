#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QTableWidgetItem>
#include <QWidget>

#include <QTimer>
#include <QThreadPool>
#include <QRunnable>
#include <QMutex>
#include <QVector>

#include <atomic>

#include "searchthread.h"
#include "protobasedialog.h"


namespace Ui {
class MainWindow;
}

Q_DECLARE_METATYPE(int64_t)
Q_DECLARE_METATYPE(Pos)
Q_DECLARE_METATYPE(Condition)

class MapView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool getSeed(int *mc, int64_t *seed, bool applyrand = true);
    bool setSeed(int mc, int64_t seed);
    QVector<Condition> getConditions() const;
    MapView *getMapView();

protected:
    void setItemCondition(QListWidgetItem *item, Condition cond) const;
    void editCondition(QListWidgetItem *item);

    void updateMapSeed();
    void updateSensitivity();

public slots:
    void warning(QString title, QString text);
    void mapGoto(qreal x, qreal z);
    void openProtobaseMsg(QString path);
    void closeProtobaseMsg();

private slots:
    void on_comboBoxMC_currentIndexChanged(int a);
    void on_seedEdit_editingFinished();
    void on_seedEdit_textChanged(const QString &arg1);

    void on_actionDesert_toggled(bool arg1);
    void on_actionJungle_toggled(bool arg1);
    void on_actionIgloo_toggled(bool arg1);
    void on_actionHut_toggled(bool arg1);
    void on_actionMonument_toggled(bool arg1);
    void on_actionVillage_toggled(bool arg1);
    void on_actionRuin_toggled(bool arg1);
    void on_actionShipwreck_toggled(bool arg1);
    void on_actionMansion_toggled(bool arg1);
    void on_actionOutpost_toggled(bool arg1);
    void on_actionPortal_toggled(bool arg1);
    void on_actionSpawn_toggled(bool arg1);
    void on_actionStronghold_toggled(bool arg1);

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

    void on_actionGo_to_triggered();
    void on_actionScan_seed_for_Quad_Huts_triggered();
    void on_actionOpen_shadow_seed_triggered();
    void on_actionAbout_triggered();

    void on_mapView_customContextMenuRequested(const QPoint &pos);

    // internal events
    int searchResultsAdd(QVector<int64_t> seeds, bool countonly);
    void searchBaseDone(int64_t s48);
    void searchFinish(int64_t s48);
    void resultTimeout();
    void removeCurrent();
    void copyResults();
    void pasteResults();
    int pasteList(bool dummy = false);
    void copyCoord();


public:
    Ui::MainWindow *ui;
    int id_counter;
    SearchThread sthread;
    QTimer stimer;
    ProtoBaseDialog *protodialog;
};

#endif // MAINWINDOW_H
