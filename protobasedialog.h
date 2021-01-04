#ifndef PROTOBASEDIALOG_H
#define PROTOBASEDIALOG_H

#include <QDialog>

namespace Ui {
class ProtoBaseDialog;
}

class ProtoBaseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProtoBaseDialog(QWidget *parent = nullptr);
    ~ProtoBaseDialog();

    bool closeOnDone();
    void setPath(QString path);

private:
    Ui::ProtoBaseDialog *ui;
};

#endif // PROTOBASEDIALOG_H
