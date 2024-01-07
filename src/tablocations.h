#ifndef TABLOCATIONS_H
#define TABLOCATIONS_H

#include <QWidget>
#include <QThread>
#include <QTreeWidgetItem>

#include "mainwindow.h"
#include "search.h"
#include "world.h"

namespace Ui {
class TabLocations;
}

class AnalysisLocations : public QThread
{
    Q_OBJECT
public:
    explicit AnalysisLocations(QObject *parent = nullptr)
        : QThread(parent), wi(),stop(),idx() {}

    QString set(WorldInfo wi, const QVector<Condition>& conds);

    virtual void run() override;

signals:
    void itemDone(QTreeWidgetItem *item);
    int warning(QString text, QMessageBox::StandardButtons buttons);

public:
    WorldInfo wi;
    ConditionTree condtree;
    SearchThreadEnv env;
    std::vector<Pos> pos;
    std::atomic_bool stop;
    std::atomic_long idx;
};

class TabLocations : public QWidget, public ISaveTab
{
    Q_OBJECT

public:
    explicit TabLocations(MainWindow *parent = nullptr);
    ~TabLocations();

    virtual bool event(QEvent *e) override;

    virtual void save(QSettings& settings) override;
    virtual void load(QSettings& settings) override;

private slots:
    int warning(QString text, QMessageBox::StandardButtons buttons);
    void onAnalysisItemDone(QTreeWidgetItem *item);
    void onAnalysisFinished();
    void onBufferTimeout();
    void onProgressTimeout();

    void on_pushStart_clicked();
    void on_pushExpand_clicked();
    void on_pushExport_clicked();
    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

private:
    Ui::TabLocations *ui;
    MainWindow *parent;
    AnalysisLocations thread;
    QTimer timer;
    int maxresults;

    QElapsedTimer elapsed;
    uint64_t nextupdate;
    uint64_t updt;
    QList<QTreeWidgetItem*> qbuf;
};

#endif // TABLOCATIONS_H
