#ifndef TABTRIGGERS_H
#define TABTRIGGERS_H

#include <QWidget>
#include <QThread>
#include <QTreeWidgetItem>

#include "mainwindow.h"
#include "search.h"
#include "world.h"

namespace Ui {
class TabTriggers;
}

class AnalysisTriggers : public QThread
{
    Q_OBJECT
public:
    explicit AnalysisTriggers(QObject *parent = nullptr)
        : QThread(parent), conds(),seeds(),wi(),stop(),idx() {}

    virtual void run() override;

signals:
    void itemDone(QTreeWidgetItem *item);
    int warning(QString text, QMessageBox::StandardButtons buttons);

public:
    QVector<Condition> conds;
    std::vector<uint64_t> seeds;
    WorldInfo wi;
    std::atomic_bool stop;
    std::atomic_long idx;
};


class TabTriggers : public QWidget, public ISaveTab
{
    Q_OBJECT

public:
    explicit TabTriggers(MainWindow *parent);
    ~TabTriggers();

    virtual void save(QSettings& settings) override;
    virtual void load(QSettings& settings) override;

private slots:
    int warning(QString text, QMessageBox::StandardButtons buttons);
    void onAnalysisItemDone(QTreeWidgetItem *item);
    void onAnalysisFinished();
    void onBufferTimeout();

    void on_pushStart_clicked();
    void on_pushExpand_clicked();
    void on_pushExport_clicked();
    void on_treeWidget_itemClicked(QTreeWidgetItem *item, int column);

private:
    Ui::TabTriggers *ui;
    MainWindow *parent;
    AnalysisTriggers thread;

    QElapsedTimer elapsed;
    uint64_t nextupdate;
    uint64_t updt;
    QList<QTreeWidgetItem*> qbuf;
};

#endif // TABTRIGGERS_H
