#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QVector>
#include <QThreadPool>
#include <QThread>
#include <QDir>

#include "settings.h"

namespace Ui {
class ExportDialog;
}
class MainWindow;

struct ExportWorker;
struct ExportThread : public QThread
{
Q_OBJECT

public:
    ExportThread(QObject *parent) : QThread(parent), pool(this), stop() {}
    virtual ~ExportThread();

    void run() override;

signals:
    void workerDone();

public slots:
    void cancel() { stop = true; }

public:
    QThreadPool pool;
    QDir dir;
    QString pattern;
    WorldInfo wi;
    int dim;
    int scale;
    int x, z, w, h, y; // block area
    int tilesize; // tile coordinates
    int bgmode;
    int heightvis;
    QVector<ExportWorker*> workers;

    std::atomic_bool stop;
};


class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(MainWindow *parent);
    ~ExportDialog();

private slots:
    void update();

    void on_buttonFromVisible_clicked();
    void on_buttonDirSelect_clicked();

    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::ExportDialog *ui;
    MainWindow *mainwindow;
};

#endif // EXPORTDIALOG_H
