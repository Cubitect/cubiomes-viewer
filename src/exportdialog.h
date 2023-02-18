#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QVector>
#include <QList>
#include <QThread>
#include <QMutex>
#include <QDir>

#include "settings.h"

namespace Ui {
class ExportDialog;
}
class MainWindow;

struct ExportWorkItem
{
    uint64_t seed;
    int tx, tz;
    QString fnam;
};

class ExportDialog;
struct ExportWorker : QThread
{
    Q_OBJECT

 public:
    ExportWorker(ExportDialog *parent) : parent(parent) {}
    virtual ~ExportWorker() {}

    void runWorkItem(const ExportWorkItem& work);

    virtual void run() override;

signals:
    void workItemDone();

public:
    ExportDialog *parent;
};

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(MainWindow *parent);
    ~ExportDialog();

    bool initWork(ExportWorkItem *work, uint64_t seed, int tx, int tz);
    bool requestWork(ExportWorkItem *work);

    void startWorkers();

signals:
    void exportFinished();
    void workItemDone();

private slots:
    void cancel() { stop = true; }
    void onWorkerFinished();

    void update();

    void on_buttonFromVisible_clicked();
    void on_buttonDirSelect_clicked();

    void on_buttonBox_clicked(QAbstractButton *button);


private:
    Ui::ExportDialog *ui;
    MainWindow *mainwindow;

public:
    QDir dir;
    QString pattern;
    WorldInfo wi;
    int dim;
    int scale;
    int x, z, w, h, y; // block area
    int tilesize; // tile coordinates
    int bgmode;
    int heightvis;
    QMutex mutex;
    QList<ExportWorkItem> workitems;
    QVector<ExportWorker*> workers;

    std::atomic_bool stop;
};

#endif // EXPORTDIALOG_H
