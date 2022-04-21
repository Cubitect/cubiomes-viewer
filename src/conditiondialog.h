#ifndef CONDITIONDIALOG_H
#define CONDITIONDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QTextEdit>
#include <QMouseEvent>

#include "search.h"
#include "formconditions.h"
#include "rangeslider.h"

class MainWindow;

namespace Ui {
class ConditionDialog;
}

class SpinExclude : public QSpinBox
{
    Q_OBJECT
public:
    SpinExclude(QWidget *parent = nullptr)
        : QSpinBox(parent)
    {
        setMinimum(-1);
        QObject::connect(this, SIGNAL(valueChanged(int)), this, SLOT(change(int)), Qt::QueuedConnection);
    }
    virtual ~SpinExclude() {}
    virtual int valueFromText(const QString &text) const override
    {
        return QSpinBox::valueFromText(text.section(" ", 0, 0));
    }
    virtual QString textFromValue(int value) const override
    {
        QString txt = QSpinBox::textFromValue(value);
        if (value == 0)
            txt += " " + tr("(ignore)");
        if (value == -1)
            txt += " " + tr("(exclude)");
        return txt;
    }

public slots:
    void change(int v)
    {
        const char *style = "";
        if (v < 0)
            style = "background: #28ff0000";
        if (v > 0)
            style = "background: #2800ff00";
        setStyleSheet(style);
        findChild<QLineEdit*>()->deselect();
    }
};

class SpinInstances : public QSpinBox
{
    Q_OBJECT
public:
    SpinInstances(QWidget *parent = nullptr)
        : QSpinBox(parent)
    {
        setRange(0, 99);
    }
    virtual ~SpinInstances() {}
    virtual int valueFromText(const QString &text) const override
    {
        return QSpinBox::valueFromText(text.section(" ", 0, 0));
    }
    virtual QString textFromValue(int value) const override
    {
        QString txt = QSpinBox::textFromValue(value);
        if (value == 0)
            txt += " " + tr("(exclude)");
        if (value > 1)
            txt += " " + tr("(cluster)");
        return txt;
    }
};


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
    VariantCheckBox(QString name, int b, int v) : QCheckBox(name),biome(b),variant(v) {}
    virtual ~VariantCheckBox() {}
    int biome;
    int variant;
    uint64_t getMask()
    {
        return 1ULL << Condition::toVariantBit(biome, variant);
    }
};

class ConditionDialog : public QDialog
{
    Q_OBJECT

public:

    explicit ConditionDialog(FormConditions *parent, Config *config, int mc, QListWidgetItem *item = 0, Condition *initcond = 0);
    virtual ~ConditionDialog();

    void addVariant(QString name, int biome, int variant);
    void setActiveTab(QWidget *tab);
    void updateMode();
    void updateBiomeSelection();
    void enableSet(const int *ids, int n);
    int warnIfBad(Condition cond);

    void getClimateLimits(int limok[6][2], int limex[6][2]);
    void getClimateLimits(LabeledRange *ranges[6], int limits[6][2]);
    void setClimateLimits(LabeledRange *ranges[6], int limits[6][2], bool complete);

signals:
    void setCond(QListWidgetItem *item, Condition cond, int modified);

private slots:
    void on_comboBoxType_activated(int);

    void on_comboBoxRelative_activated(int);

    void on_buttonUncheck_clicked();
    void on_buttonInclude_clicked();
    void on_buttonExclude_clicked();

    void on_buttonAreaInfo_clicked();

    void on_checkRadius_toggled(bool checked);
    void on_radioSquare_toggled(bool checked);
    void on_radioCustom_toggled(bool checked);

    void on_lineSquare_editingFinished();
    //void on_lineRadius_editingFinished();

    void on_buttonCancel_clicked();

    void on_buttonOk_clicked();

    void on_ConditionDialog_finished(int result);

    void on_comboBoxCat_currentIndexChanged(int index);

    void on_checkStartPiece_stateChanged(int state);

    void onClimateLimitChanged();

private:
    Ui::ConditionDialog *ui;
    QTextEdit *textDescription;

    QCheckBox *biomecboxes[256];
    SpinExclude *tempsboxes[9];
    QVector<VariantCheckBox*> variantboxes;
    LabeledRange *climaterange[2][6];
    QCheckBox *climatecomplete[6];
    std::map<int, NoiseBiomeIndicator*> noisebiomes;

public:
    Config *config;
    QListWidgetItem *item;
    Condition cond;
    int mc;
};

#endif // CONDITIONDIALOG_H
