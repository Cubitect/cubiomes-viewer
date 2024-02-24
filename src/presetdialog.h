#ifndef EXAMPLESDIALOG_H
#define EXAMPLESDIALOG_H

#include "config.h"
#include "search.h"

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
class PresetDialog;
}

struct Preset
{
    std::vector<Condition> condvec;
    QString title;
    QString desc;
};

bool loadConditions(Preset& preset, QString rc);
bool saveConditions(const Preset& preset, QString rc);

class MainWindow;

class PresetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PresetDialog(QWidget *parent, WorldInfo wi, bool showExamples);
    ~PresetDialog();

    void setActiveFilter(const std::vector<Condition>& condvec);

    QListWidgetItem *addPreset(QString rc, QString title, QString desc, bool enabled);

    QString getPreset();
    void updateSelection();

private slots:
    void on_tabWidget_currentChanged(int idx);
    void on_listFilters_itemSelectionChanged();
    void on_listExamples_itemSelectionChanged();

    void on_buttonSave_clicked();
    void on_buttonDelete_clicked();

private:
    Ui::PresetDialog *ui;
    std::map<QString, Preset> presets;
public:
    std::vector<Condition> activeFilter;
    QString rc;
};

#endif // EXAMPLESDIALOG_H
