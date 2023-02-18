#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QString>

#define VERS_MAJOR 3
#define VERS_MINOR 2
#define VERS_PATCH 0   // negative patch number designates a development version

// returns +1 if newer, -1 if older  and 0 if equal
inline int cmpVers(int major, int minor, int patch)
{
    int s;
    s = (major > VERS_MAJOR) - (major < VERS_MAJOR);
    if (s) return s;
    s = (minor > VERS_MINOR) - (minor < VERS_MINOR);
    if (s) return s;
    int p0 = VERS_PATCH >= 0 ? 1000+VERS_PATCH : -VERS_PATCH;
    int p1 = patch      >= 0 ? 1000+patch      : -patch;
    s = (p1 > p0) - (p1 < p0);
    return s;
}

inline QString getVersStr()
{
    QString s = QString("%1.%2.").arg(VERS_MAJOR).arg(VERS_MINOR);
    if (VERS_PATCH < 0)
        s += QString("dev%1").arg(-1-VERS_PATCH);
    else
        s += QString("%1").arg(VERS_PATCH);
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
