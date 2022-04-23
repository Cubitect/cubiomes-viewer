#ifndef BIOMECOLORDIALOG_H
#define BIOMECOLORDIALOG_H

#include <QDialog>

namespace Ui {
class BiomeColorDialog;
}

class MainWindow;

class BiomeColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BiomeColorDialog(MainWindow *parent, QString initrc);
    ~BiomeColorDialog();

    int saveColormap(QString rc, QString desc);

public slots:
    void setBiomeColor(int id, const QColor &col);
    void editBiomeColor(int id);

private slots:
    void on_comboColormaps_currentIndexChanged(int index);
    void on_buttonSaveAs_clicked();
    void on_buttonRemove_clicked();
    void on_buttonOk_clicked();

    void onAllToDefault();
    void onAllToDimmed();

private:
    Ui::BiomeColorDialog *ui;
    MainWindow *parent;
    QPushButton *buttons[256];
    unsigned char colors[256][3];
    QString activerc;
    bool modified;
};

#endif // BIOMECOLORDIALOG_H
