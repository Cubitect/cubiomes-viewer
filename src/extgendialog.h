#ifndef EXTGENDIALOG_H
#define EXTGENDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QCheckBox>
#include <QLineEdit>

#include "config.h"


namespace Ui {
class ExtGenDialog;
}


class ExtGenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExtGenDialog(QWidget *parent, ExtGenConfig *extgen);
    ~ExtGenDialog();

    void initSettings(ExtGenConfig *extgen);

    ExtGenConfig getSettings();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

    void updateToggles();

private:
    Ui::ExtGenDialog *ui;
    QCheckBox *checkSalts[FEATURE_NUM];
    QLineEdit *lineSalts[FEATURE_NUM];

    ExtGenConfig extgen;
};

#endif // EXTGENDIALOG_H
