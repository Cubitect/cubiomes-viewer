#ifndef BIOMECOLORDIALOG_H
#define BIOMECOLORDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTextStream>

namespace Ui {
class BiomeColorDialog;
}
class MainWindow;

class BiomeColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BiomeColorDialog(MainWindow *parent, QString initrc, int mc, int dim);
    ~BiomeColorDialog();

    int saveColormap(QString rc, QString desc);
    int loadColormap(QIODevice *iodevice, bool reset);

    void arrange(int sort);

signals:
    void yieldBiomeColorRc(QString path);

public slots:
    void setBiomeColor(int id, const QColor &col);
    void editBiomeColor(int id);

private slots:
    void on_comboColormaps_currentIndexChanged(int index);

    void onSaveAs();
    void onSaveSelect(const QString &rc, const QString &text);
    void onExport();
    void onRemove();
    void onAccept();
    void onImport();
    void onColorHelp();
    void onAllToDefault();
    void onAllToDimmed();

private:
    void exportColors(QTextStream& stream);
    void importColors(QTextStream& stream);

private:
    Ui::BiomeColorDialog *ui;
    MainWindow *mainwindow;
    int mc, dim;
    QLabel *separator;
    QPushButton *buttons[256][3];
    unsigned char colors[256][3];
    QString activerc;
    bool modified;
};

#endif // BIOMECOLORDIALOG_H
