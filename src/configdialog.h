#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QAbstractButton>

#include "config.h"


namespace Ui {
class ConfigDialog;
}


class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent, Config *config);
    ~ConfigDialog();

    void initConfig(Config *config);
    Config getConfig();

signals:
    void updateConfig();
    void updateMapConfig();

private slots:
    void onUpdateMapConfig();

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_lineGridSpacing_textChanged(const QString &arg1);

private:
    Ui::ConfigDialog *ui;
    Config conf;
};

#endif // CONFIGDIALOG_H
