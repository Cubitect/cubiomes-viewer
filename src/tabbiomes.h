#ifndef TABBIOMES_H
#define TABBIOMES_H

#include <QWidget>
#include <QThread>
#include <QTreeWidgetItem>
#include <QSortFilterProxyModel>
#include <QHeaderView>

#include "mainwindow.h"
#include "search.h"
#include "world.h"
#include "cutil.h"

namespace Ui {
class TabBiomes;
}

class AnalysisBiomes : public QThread
{
    Q_OBJECT
public:
    explicit AnalysisBiomes(QObject *parent = nullptr)
        : QThread(parent) {}

    virtual void run() override;
    void runStatistics(Generator *g);
    void runLocate(Generator *g);

signals:
    void seedDone(uint64_t seed, QVector<uint64_t> cnt);
    void seedItem(QTreeWidgetItem *item);

public:
    QVector<uint64_t> seeds;
    WorldInfo wi;
    std::atomic_bool stop;
    int dims[3];
    int x1, z1, x2, z2;
    int scale;
    uint64_t samples;
    int locate;
    int minsize;
    int tolerance;
};

class BiomeTableModel : public QAbstractTableModel
{
public:
    explicit BiomeTableModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {}

    virtual int rowCount(const QModelIndex&) const override { return ids.size(); }
    virtual int columnCount(const QModelIndex&) const override { return seeds.size(); }

    virtual QVariant data(const QModelIndex& index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    bool insertId(int id);
    void insertSeed(uint64_t seed);
    void reset(int mc);

    QList<int> ids;
    QList<uint64_t> seeds;
    QMap<int, QMap<uint64_t, QVariant>> cnt; // cnt[id][seed]
    IdCmp cmp;
};

class BiomeSortProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    BiomeSortProxy(QObject *parent = nullptr) : QSortFilterProxyModel(parent),column(),order() {}

    virtual bool lessThan(const QModelIndex& a, const QModelIndex& b) const override
    {
        QVariant av = sourceModel()->data(a, Qt::DisplayRole);
        QVariant bv = sourceModel()->data(b, Qt::DisplayRole);
        return bv.toInt() < av.toInt();
    }

    virtual void sort(int column, Qt::SortOrder order) override
    {
        if (this->column == -1)
            QSortFilterProxyModel::sort(-1, order);
        else
            QSortFilterProxyModel::sort(column, order);
        this->column = column;
        this->order = order;
    }

    int column;
    Qt::SortOrder order;
};

class TabBiomes : public QWidget, public ISaveTab
{
    Q_OBJECT

public:
    explicit TabBiomes(MainWindow *parent = nullptr);
    ~TabBiomes();

    virtual void save(QSettings& settings) override;
    virtual void load(QSettings& settings) override;
    virtual void refresh() override { refreshBiomes(); }

    void refreshBiomes(int activeid = -1);

private slots:
    void onAnalysisSeedDone(uint64_t seed, QVector<uint64_t> idcnt);
    void onAnalysisSeedItem(QTreeWidgetItem *item);
    void onAnalysisFinished();
    void onSort(int column, Qt::SortOrder);

    void on_pushStart_clicked();
    void on_pushExport_clicked();

    void on_table_doubleClicked(const QModelIndex &index);

    void on_buttonFromVisible_clicked();

    void on_radioFullSample_toggled(bool checked);

    void on_lineBiomeSize_textChanged(const QString &arg1);

    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

private:
    Ui::TabBiomes *ui;
    MainWindow *parent;
    AnalysisBiomes thread;
    BiomeTableModel model;
    BiomeSortProxy proxy;
    std::map<QString, int> str2biome;
};

#endif // TABBIOMES_H
