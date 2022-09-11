#ifndef STATSDIALOG_H
#define STATSDIALOG_H

#include <QDialog>

namespace Ui {
class StatsDialog;
}

class StatsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatsDialog(QWidget *parent = nullptr);
    ~StatsDialog();

private:
    Ui::StatsDialog *ui;
};

#endif // STATSDIALOG_H
