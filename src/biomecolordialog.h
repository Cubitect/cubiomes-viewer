#ifndef BIOMECOLORDIALOG_H
#define BIOMECOLORDIALOG_H

#include <QDialog>
#include <QLabel>

namespace Ui {
class BiomeColorDialog;
}

class BiomeColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BiomeColorDialog(QWidget *parent, QString initrc, int mc, int dim);
    ~BiomeColorDialog();

    int saveColormap(QString rc, QString desc);
    QString getRc();

    enum {
        SORT_ID,
        SORT_LEX,
    };
    void arrange(int sort);

signals:
    void yieldBiomeColorRc(QString path);

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
    int mc, dim;
    QLabel *separator;
    QPushButton *buttons[256][3];
    unsigned char colors[256][3];
    QString activerc;
    bool modified;
};

#endif // BIOMECOLORDIALOG_H
