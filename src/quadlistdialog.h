#ifndef QUADLISTDIALOG_H
#define QUADLISTDIALOG_H

#include <QDialog>

class MainWindow;

namespace Ui {
class QuadListDialog;
}

class QuadListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QuadListDialog(MainWindow *mainwindow);
    ~QuadListDialog();

    void loadSeed();
    void refresh();

    bool getSeed(int *mc, int64_t *seed);

private slots:
    void on_buttonGo_clicked();

    void on_listQuadStruct_customContextMenuRequested(const QPoint &pos);

    void gotoSwampHut();

    void on_buttonClose_clicked();

private:
    Ui::QuadListDialog *ui;
    MainWindow *mainwindow;
};

#endif // QUADLISTDIALOG_H
