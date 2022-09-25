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
        : QThread(parent) {}

    virtual void run() override;

signals:
    void itemDone(QTreeWidgetItem *item);

public:
    QVector<uint64_t> seeds;
    WorldInfo wi;
    std::atomic_bool stop;
    int x1, z1, x2, z2;
    bool mapshow[STRUCT_NUM];
    bool collect;
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
    void onAnalysisItemDone(QTreeWidgetItem *item);
    void onAnalysisFinished();

    void on_pushStart_clicked();

    void on_pushExport_clicked();

    void on_buttonFromVisible_clicked();

    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

private:
    Ui::TabStructures *ui;
    MainWindow *parent;
    AnalysisStructures thread;
};

#endif // TABSTRUCTURES_H
