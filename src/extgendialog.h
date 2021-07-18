#ifndef EXTGENDIALOG_H
#define EXTGENDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QCheckBox>
#include <QLineEdit>

#include "settings.h"


namespace Ui {
class ExtGenDialog;
}


class ExtGenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExtGenDialog(QWidget *parent, ExtGenSettings *extgen);
    ~ExtGenDialog();

    void initSettings(ExtGenSettings *extgen);

    ExtGenSettings getSettings();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

    void updateToggles();

private:
    Ui::ExtGenDialog *ui;
    QCheckBox *checkSalts[FEATURE_NUM];
    QLineEdit *lineSalts[FEATURE_NUM];

    ExtGenSettings extgen;
};

#endif // EXTGENDIALOG_H
