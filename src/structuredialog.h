#ifndef STRUCTUREDIALOG_H
#define STRUCTUREDIALOG_H

#include <QDialog>
#include <QAbstractButton>

#include "config.h"

namespace Ui {
class StructureDialog;
}

class QCheckBox;
class QLineEdit;

class StructureDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StructureDialog(QWidget *parent = nullptr);
    ~StructureDialog();

    void refresh();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

signals:
    void updateMapConfig();

private:
    Ui::StructureDialog *ui;
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

#endif // STRUCTUREDIALOG_H
