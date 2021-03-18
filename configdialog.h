#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class ConfigDialog;
}

struct Config
{
    bool restoreSession;
    bool smoothMotion;
    bool showGrid;
    int seedsPerItem;
    int queueSize;
    int maxMatching;

    void reset();
};

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent, Config *config);
    ~ConfigDialog();

    void initSettings(Config *config);

    Config getConfig();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::ConfigDialog *ui;
    Config conf;
};

#endif // CONFIGDIALOG_H
