#ifndef MAPTOOLSDIALOG_H
#define MAPTOOLSDIALOG_H

#include <QDialog>
#include <QAbstractButton>

#include "config.h"

namespace Ui {
class MapToolsDialog;
}

class QCheckBox;
class QLineEdit;

class MapToolsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MapToolsDialog(QWidget *parent = nullptr);
    ~MapToolsDialog();

    void refresh();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

signals:
    void updateMapConfig();

private:
    Ui::MapToolsDialog *ui;
    struct VisUi
    {
        QCheckBox *check;
        QLineEdit *line;
    };
    std::map<int, VisUi> vui;

public:
    MapConfig mconfig;
    bool modified;
};

#endif // MAPTOOLSDIALOG_H
