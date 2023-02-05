#ifndef LAYERDIALOG_H
#define LAYERDIALOG_H

#include <QDialog>
#include <QRadioButton>
#include <QComboBox>

#include "settings.h"

namespace Ui {
class LayerDialog;
}

const char *getLayerOptionText(int mode, int disp);

class LayerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LayerDialog(QWidget *parent, int mc);
    ~LayerDialog();

    void setLayerOptions(LayerOpt lopts);
    LayerOpt getLayerOptions();

signals:
    void apply();

public slots:
    void onRadioChange();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::LayerDialog *ui;
    QRadioButton *radio[LOPT_MAX];
    QComboBox *combo[LOPT_MAX];
};

#endif // LAYERDIALOG_H
