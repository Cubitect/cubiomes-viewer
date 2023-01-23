#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QAbstractButton>

#include "settings.h"


namespace Ui {
class ConfigDialog;
}


class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent, Config *config);
    ~ConfigDialog();

    void initSettings(Config *config);

    Config getSettings();

    void setBiomeColorPath(QString path);

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

    void on_buttonBiomeColorEditor_clicked();
    void on_buttonBiomeColor_clicked();

    void on_buttonStructVisEdit_clicked();

    void on_buttonClear_clicked();

    void on_buttonColorHelp_clicked();

    void on_comboHeightVis_currentIndexChanged(int index);

private:
    Ui::ConfigDialog *ui;
    Config conf;
public:
    bool visModified;
};

#endif // CONFIGDIALOG_H
