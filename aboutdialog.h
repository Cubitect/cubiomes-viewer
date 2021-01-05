#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

#define VERS_MAJOR 1
#define VERS_MINOR 1
#define VERS_PATCH 0

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog();

private:
    Ui::AboutDialog *ui;
};

#endif // ABOUTDIALOG_H
