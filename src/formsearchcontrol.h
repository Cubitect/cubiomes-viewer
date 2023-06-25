#ifndef FORMSEARCHCONTROL_H
#define FORMSEARCHCONTROL_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <deque>

#include "searchthread.h"
#include "protobasedialog.h"
#include "config.h"

namespace Ui {
class FormSearchControl;
}

class MainWindow;

class SeedTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit SeedTableModel(QObject *parent = nullptr) :
        QAbstractTableModel(parent) {}

    enum { COL_SEED, COL_TOP16, COL_HEX48, COL_MAX };

    virtual int rowCount(const QModelIndex&) const override { return seeds.size(); }
    virtual int columnCount(const QModelIndex&) const override { return COL_MAX; }

    virtual QVariant data(const QModelIndex& index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    int insertSeeds(QVector<uint64_t> seeds);
    void removeRow(int row);
    void reset();

    struct Seed
    {
        uint64_t seed;
        QVariant varSeed, varHex48, varTop16;
        QVariant txtSeed, txtHex48, txtTop16;
    };
    QList<Seed> seeds;
};

class SeedSortProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    SeedSortProxy(QObject *parent = nullptr) : QSortFilterProxyModel(parent),column(),order(Qt::DescendingOrder) {}

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Vertical && role == Qt::DisplayRole)
            return QVariant::fromValue(section + 1);
        return QSortFilterProxyModel::headerData(section, orientation, role);
    }

    virtual bool lessThan(const QModelIndex& a, const QModelIndex& b) const override
    {
        uint64_t av = sourceModel()->data(a, Qt::UserRole).toULongLong();
        uint64_t bv = sourceModel()->data(b, Qt::UserRole).toULongLong();
        if (a.column() == SeedTableModel::COL_SEED)
            return (int64_t) bv < (int64_t) av;
        else
            return bv < av;
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

class FormSearchControl : public QWidget
{
    Q_OBJECT

public:
    explicit FormSearchControl(MainWindow *parent);
    ~FormSearchControl();

    std::vector<uint64_t> getResults();
    SearchConfig getSearchConfig();
    bool setSearchConfig(SearchConfig s, bool quiet);

    void stopSearch();
    bool setList64(QString path, bool quiet);

    void setResultsPath(QString path);

    void searchLockUi(bool lock);

    void setSearchMode(int mode);

signals:
    void selectedSeedChanged(uint64_t seed);
    void searchStatusChanged(bool running);
    void resultsAdded(int cnt);

public slots:
    int warning(QString text, QMessageBox::StandardButtons buttons = QMessageBox::Ok);
    void openProtobaseMsg(QString path);
    void closeProtobaseMsg();

    void on_buttonClear_clicked();
    void on_buttonStart_clicked();
    void on_buttonMore_clicked();

    void onSort(int column, Qt::SortOrder);
    void onSeedSelectionChanged();
    void on_results_clicked(const QModelIndex &index);
    void on_results_customContextMenuRequested(const QPoint& pos);

    void on_buttonSearchHelp_clicked();

    void on_comboSearchType_currentIndexChanged(int index);

    void pasteResults();
    int pasteList(bool dummy);
    void onBufferTimeout();
    void searchResult(uint64_t seed);
    int searchResultsAdd(std::vector<uint64_t> seeds, bool countonly);
    void searchProgressReset();
    void updateSearchProgress(uint64_t last, uint64_t end, int64_t seed);
    void searchFinish(bool done);
    void progressTimeout();
    void removeCurrent();
    void copySeed();
    void copyResults();

protected:
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    MainWindow *parent;
    Ui::FormSearchControl *ui;
    SeedTableModel *model;
    SeedSortProxy *proxy;
    ProtoBaseDialog *protodialog;
    SearchMaster sthread;
    QElapsedTimer elapsed;
    QTimer stimer;
    QFile resultfile;

    // the seed list option is not stored in a widget but is loaded with the "..." button
    QString slist64path;
    QString slist64fnam; // file name without directory
    std::vector<uint64_t> slist64;

    // min and max seeds values
    uint64_t smin, smax;

    // found seeds that are waiting to be added to results
    std::vector<uint64_t> qbuf;
    quint64 nextupdate;
    quint64 updt;
};

#endif // FORMSEARCHCONTROL_H
