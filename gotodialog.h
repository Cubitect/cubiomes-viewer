#ifndef GOTODIALOG_H
#define GOTODIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class GotoDialog;
}
class MainWindow;

class GotoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GotoDialog(MainWindow *parent, qreal x, qreal z);
    ~GotoDialog();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::GotoDialog *ui;
    MainWindow *mainwindow;
};

#endif // GOTODIALOG_H
