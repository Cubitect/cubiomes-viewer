#ifndef RANGEDIALOG_H
#define RANGEDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class RangeDialog;
}

class RangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RangeDialog(QWidget *parent, uint64_t smin, uint64_t smax);
    ~RangeDialog();

    bool getBounds(uint64_t *smin, uint64_t *smax);

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::RangeDialog *ui;
};

#endif // RANGEDIALOG_H
