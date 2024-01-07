#ifndef LAYERDIALOG_H
#define LAYERDIALOG_H

#include <QDialog>
#include <QRadioButton>
#include <QComboBox>

#include "config.h"

namespace Ui {
class LayerDialog;
}

struct LayerOptInfo
{
    QString summary;
    QString tooltip;
};

bool getLayerOptionInfo(LayerOptInfo *info, int mode, int disp, WorldInfo wi);

class LayerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LayerDialog(QWidget *parent, WorldInfo mc);
    ~LayerDialog();

    void setLayerOptions(LayerOpt lopts);
    LayerOpt getLayerOptions();

signals:
    void apply();

public slots:
    void onRadioChange();
    void onComboChange(QComboBox *combo);

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::LayerDialog *ui;
    QRadioButton *radio[LOPT_MAX];
    QComboBox *combo[LOPT_MAX];
};

#endif // LAYERDIALOG_H
