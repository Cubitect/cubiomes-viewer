#ifndef CONDITIONDIALOG_H
#define CONDITIONDIALOG_H

#include "config.h"
#include "formconditions.h"
#include "search.h"
#include "util.h"
#include "widgets.h"

#include <QCheckBox>
#include <QDialog>
#include <QMouseEvent>
#include <QTextEdit>
#include <QVBoxLayout>

class MainWindow;
class MapView;

namespace Ui {
class ConditionDialog;
}

class NoiseBiomeIndicator : public QCheckBox
{
    Q_OBJECT
public:
    NoiseBiomeIndicator(QString title, QWidget *parent)
        : QCheckBox(title, parent)
    {
    }
    virtual ~NoiseBiomeIndicator() {}
    void mousePressEvent(QMouseEvent *event)
    {   // make read only
        if (event->button() == 0)
            QCheckBox::mousePressEvent(event);
    }
};

class VariantCheckBox : public QCheckBox
{
    Q_OBJECT
public:
    VariantCheckBox(const StartPiece *sp) : QCheckBox(sp->name),sp(sp) {}
    virtual ~VariantCheckBox() {}
    const StartPiece *sp;
};


class ConditionDialog : public QDialog
{
    Q_OBJECT

public:

    explicit ConditionDialog(FormConditions *parent, MapView *mapview, Config *config, WorldInfo wi, QListWidgetItem *item = 0, Condition *initcond = 0);
    virtual ~ConditionDialog();

    void addTempCat(int temp, QString name);
    void initComboY(QComboBox *cb, int y);
    void updateMode();
    void updateBiomeSelection();
    bool warnIfBad(Condition cond);

    void onReject();
    void onAccept();

    void getClimateLimits(int limok[6][2], int limex[6][2]);
    void getClimateLimits(LabeledRange *ranges[6], int limits[6][2]);
    void setClimateLimits(LabeledRange *ranges[6], int limits[6][2], bool complete);

signals:
    void setCond(QListWidgetItem *item, Condition cond, int modified);

private slots:
    void on_comboCat_currentIndexChanged(int);
    void on_comboType_activated(int);
    void on_comboScale_activated(int);

    void on_comboRelative_activated(int);

    void on_buttonUncheck_clicked();
    void on_buttonInclude_clicked();
    void on_buttonExclude_clicked();

    void on_buttonAreaInfo_clicked();
    void on_buttonFromVisible_clicked();

    void on_checkRadius_toggled(bool checked);
    void on_radioSquare_toggled(bool checked);
    void on_radioCustom_toggled(bool checked);

    void on_lineSquare_editingFinished();
    //void on_lineRadius_editingFinished();

    void on_ConditionDialog_finished(int result);

    void onCheckStartChanged(int state);
    void onClimateLimitChanged();

    void on_lineBiomeSize_textChanged(const QString &text);

    void onLuaSaveAs(const QString& fileName);
    void on_comboLua_currentIndexChanged(int index);
    void on_pushLuaSaveAs_clicked();
    void on_pushLuaSave_clicked();
    void on_pushLuaOpen_clicked();
    void on_pushLuaExample_clicked();

    void on_comboHeightRange_currentIndexChanged(int index);

    void on_pushInfoLua_clicked();

    void on_comboClimatePara_currentIndexChanged(int index);
    void on_comboOctaves_currentIndexChanged(int index);

    void on_comboY1_currentTextChanged(const QString &text);
    void on_comboY2_currentTextChanged(const QString &text);

private:
    Ui::ConditionDialog *ui;
    QTextEdit *textDescription;

    QFrame *separator;
    std::map<int, QCheckBox*> biomecboxes;
    SpinExclude *tempsboxes[9];
    LabeledRange *climaterange[2][6];
    QCheckBox *climatecomplete[6];
    std::map<int, NoiseBiomeIndicator*> noisebiomes;

    QVector<VariantCheckBox*> variantboxes;
    uint64_t luahash;

public:
    MapView *mapview;
    Config *config;
    QListWidgetItem *item;
    Condition cond;
    WorldInfo wi;
};

#endif // CONDITIONDIALOG_H
