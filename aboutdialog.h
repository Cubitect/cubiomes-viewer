#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

#define VERS_MAJOR 1
#define VERS_MINOR 4
#define VERS_PATCH 0

// returns +1 if newer, -1 if older  and 0 if equal
inline int cmpVers(int major, int minor, int patch)
{
    int s;
    s = (major > VERS_MAJOR) - (major < VERS_MAJOR);
    if (s) return s;
    s = (minor > VERS_MINOR) - (minor < VERS_MINOR);
    if (s) return s;
    s = (patch > VERS_PATCH) - (patch < VERS_PATCH);
    return s;
}

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
