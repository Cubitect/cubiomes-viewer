#ifndef STRUCTUREDIALOG_H
#define STRUCTUREDIALOG_H

#include <QDialog>

#include <map>

namespace Ui {
class StructureDialog;
}

class QLineEdit;

class StructureDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StructureDialog(QWidget *parent = nullptr);
    ~StructureDialog();

public slots:
    void onAccept();
    void onReset();

private:
    Ui::StructureDialog *ui;
    std::map<int, QLineEdit*> entries;

public:
    std::map<int, double> structvis;
    bool modified;
};

#endif // STRUCTUREDIALOG_H
