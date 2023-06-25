#ifndef TABSTRUCTURES_H
#define TABSTRUCTURES_H

#include "mainwindow.h"

namespace Ui {
class TabStructures;
}


class AnalysisStructures : public QThread
{
    Q_OBJECT
public:
    explicit AnalysisStructures(QObject *parent = nullptr)
        : QThread(parent),idx() {}

    virtual void run() override;
    void runStructs(Generator *g);
    void runQuads(Generator *g);

signals:
    void itemDone(QTreeWidgetItem *item);
    void quadDone(QTreeWidgetItem *item);

public:
    std::vector<uint64_t> seeds;
    WorldInfo wi;
    std::atomic_bool stop;
    std::atomic_int idx;
    struct Dat { int x1, z1, x2, z2; } area;
    bool mapshow[D_STRUCT_NUM];
    bool collect;
    bool quad;
};

class TabStructures : public QWidget, public ISaveTab
{
    Q_OBJECT

public:
    explicit TabStructures(MainWindow *parent = nullptr);
    ~TabStructures();

    virtual void save(QSettings& settings) override;
    virtual void load(QSettings& settings) override;

private slots:
    void onHeaderClick(QTreeView *tree);

    void onAnalysisItemDone(QTreeWidgetItem *item);
    void onAnalysisQuadDone(QTreeWidgetItem *item);
    void onAnalysisFinished();
    void onBufferTimeout();

    void onTreeItemClicked(QTreeWidgetItem *item, int column);

    void on_pushStart_clicked();
    void on_pushExport_clicked();
    void on_buttonFromVisible_clicked();
    void on_tabWidget_currentChanged(int index);


private:
    Ui::TabStructures *ui;
    MainWindow *parent;
    AnalysisStructures thread;
    AnalysisStructures::Dat dats, datq;
    int sortcols, sortcolq;

    QElapsedTimer elapsed;
    uint64_t nextupdate;
    uint64_t updt;
    QList<QTreeWidgetItem*> qbufs;
    QList<QTreeWidgetItem*> qbufq;
};

#endif // TABSTRUCTURES_H
