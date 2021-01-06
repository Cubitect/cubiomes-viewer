#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include <QDialog>
#include <QCheckBox>

#include "mainwindow.h"
#include "search.h"

namespace Ui {
class FilterDialog;
}


class FilterDialog : public QDialog
{
    Q_OBJECT

public:

    explicit FilterDialog(MainWindow *parent = 0, Condition *initcond = 0);
    ~FilterDialog();

    void updateMode();

private slots:
    void on_comboBoxType_activated(int);

    void on_buttonUncheck_clicked();

    void on_buttonCheck_clicked();

    void on_buttonBox_accepted();

    void on_buttonArea_toggled(bool checked);

private:
    Ui::FilterDialog *ui;
    QCheckBox *biomecboxes[256];
    bool custom;

public:
    Condition cond;
};

#endif // FILTERDIALOG_H
