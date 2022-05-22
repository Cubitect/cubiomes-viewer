#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class ExportDialog;
}
class MainWindow;

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(MainWindow *parent);
    ~ExportDialog();

private:
    int saveimg(QString dir, QString pattern, uint64_t seed, int tx, int tz, QImage *img);

private slots:
    void update();

    void on_comboSeed_activated(int);
    void on_comboScale_activated(int);
    void on_comboTileSize_activated(int);

    void on_buttonFromVisible_clicked();
    void on_buttonDirSelect_clicked();

    void on_groupTiled_toggled(bool on);

    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::ExportDialog *ui;
    MainWindow *mainwindow;
};

#endif // EXPORTDIALOG_H
