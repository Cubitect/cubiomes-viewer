#ifndef FORMGEN48_H
#define FORMGEN48_H

#include <QWidget>
#include <QThread>

#include "settings.h"
#include "search.h"

namespace Ui {
class FormGen48;
}

class MainWindow;

class FormGen48 : public QWidget
{
    Q_OBJECT

public:
    explicit FormGen48(MainWindow *parent);
    ~FormGen48();

    void setSettings(const Gen48Settings& gen48, bool quiet);
    Gen48Settings getSettings(bool resolveauto = false);

    bool setList48(QString path, bool quiet);
    const std::vector<int64_t>& getList48() { return slist48; }

    uint64_t estimateSeedCnt();
    void updateCount();

    void updateAutoConditions(const QVector<Condition>& condlist);
    void updateAutoUi();

signals:
    void changed();

private:
    void setPathEnabled(bool enabled);
    void setSaltEnabled(bool enabled);
    void setAreaEnabled(bool enabled);
    void setQualEnabled(bool enabled);
    void setQMEnabled(bool enabled);

    void updateMode();

private slots:
    void on_comboMode_currentIndexChanged(int idx);
    void on_comboLow20_currentIndexChanged(int idx);

    void on_buttonBrowse_clicked();

    void on_radioAuto_toggled();

    void on_spinMonumentArea_editingFinished();

    void on_lineSalt_editingFinished();

private:
    MainWindow *parent;
    Ui::FormGen48 *ui;

    // main condition for "auto" mode (updated when conditions change)
    Condition cond;

    QString slist48path;
    std::vector<int64_t> slist48;
};

#endif // FORMGEN48_H
