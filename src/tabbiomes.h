#ifndef TABBIOMES_H
#define TABBIOMES_H

#include <QWidget>
#include <QThread>
#include <QTreeWidgetItem>
#include <QSortFilterProxyModel>
#include <QHeaderView>

#include "mainwindow.h"
#include "util.h"

namespace Ui {
class TabBiomes;
}

class AnalysisBiomes : public QThread
{
    Q_OBJECT
public:
    explicit AnalysisBiomes(QObject *parent = nullptr)
        : QThread(parent),idx() {}

    virtual void run() override;
    void runStatistics(Generator *g);
    void runLocate(Generator *g);

signals:
    void seedDone(uint64_t seed, QVector<uint64_t> cnt);
    void seedItem(QTreeWidgetItem *item);

public:
    std::vector<uint64_t> seeds;
    WorldInfo wi;
    std::atomic_bool stop;
    std::atomic_long idx;
    int dims[3];
    struct Dat {
        int x1, z1, x2, z2;
        int scale;
        int locate;
        uint64_t samples;
    } dat;
    int minsize;
    int tolerance;
};

class BiomeTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit BiomeTableModel(QObject *parent = nullptr) :
        QAbstractTableModel(parent), cmp(IdCmp::SORT_LEX, -1, DIM_UNDEF) {}
    virtual ~BiomeTableModel() {}

    virtual int rowCount(const QModelIndex&) const override { return seeds.size(); }
    virtual int columnCount(const QModelIndex&) const override { return ids.size(); }

    virtual QVariant data(const QModelIndex& index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void insertIds(QSet<int>& ids);
    void insertSeeds(QList<uint64_t>& seeds);
    void reset(int mc);

    QList<int> ids; // biome column
    QList<uint64_t> seeds; // seed rows
    QMap<int, QMap<uint64_t, QVariant>> cnt; // cnt[id][seed]
    IdCmp cmp;
};

class BiomeSortProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    BiomeSortProxy(QObject *parent = nullptr) : QSortFilterProxyModel(parent),column(),order(Qt::AscendingOrder) {}

    virtual bool lessThan(const QModelIndex& a, const QModelIndex& b) const override
    {
        QVariant av = sourceModel()->data(a, Qt::DisplayRole);
        QVariant bv = sourceModel()->data(b, Qt::DisplayRole);
        return bv.toInt() < av.toInt();
    }

    virtual void sort(int column, Qt::SortOrder order) override
    {
        if (column >= columnCount())
            return;
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

class BiomeHeader : public QHeaderView
{
    Q_OBJECT

public:
    BiomeHeader(QWidget *parent = nullptr);

    void onSectionPress(int section);
    virtual bool event(QEvent *e) override;
    virtual void paintSection(QPainter *painter, const QRect& rect, int section) const override;
    virtual QSize sectionSizeFromContents(int section) const override;

    int hover;
    int pressed;
};

class TabBiomes : public QWidget, public ISaveTab
{
    Q_OBJECT

public:
    explicit TabBiomes(MainWindow *parent = nullptr);
    ~TabBiomes();

    virtual bool event(QEvent *e) override;

    virtual void save(QSettings& settings) override;
    virtual void load(QSettings& settings) override;
    virtual void refresh() override { refreshBiomes(); }

    void refreshBiomes(int activeid = -1);

private slots:
    void onLocateHeaderClick();
    void onTableSort(int column, Qt::SortOrder);
    void onVHeaderClicked(int row);
    void onAnalysisSeedDone(uint64_t seed, QVector<uint64_t> idcnt);
    void onAnalysisSeedItem(QTreeWidgetItem *item);
    void onAnalysisFinished();
    void onBufferTimeout();

    void on_pushStart_clicked();
    void on_pushExport_clicked();
    void on_buttonFromVisible_clicked();
    void on_radioFullSample_toggled(bool checked);
    void on_lineBiomeSize_textChanged(const QString &arg1);
    void on_treeLocate_itemClicked(QTreeWidgetItem *item, int column);
    void on_tabWidget_currentChanged(int index);

private:
    void exportResults(QTextStream& stream);

private:
    Ui::TabBiomes *ui;
    MainWindow *parent;
    AnalysisBiomes thread;
    BiomeTableModel *model;
    BiomeSortProxy *proxy;
    QMap<QString, int> str2biome;
    AnalysisBiomes::Dat dats, datl;
    int sortcol;

    QElapsedTimer elapsed;
    uint64_t updt;
    uint64_t nextupdate;
    QVector<QVector<uint64_t>> qbufs;
    QList<QTreeWidgetItem*> qbufl;
};

#endif // TABBIOMES_H
