#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QTextEdit>

#include "search.h"
#include "formconditions.h"

class MainWindow;

namespace Ui {
class FilterDialog;
}

class SpinExclude : public QSpinBox
{
    Q_OBJECT
public:
    SpinExclude()
    {
        setMinimum(-1);
        QObject::connect(this, SIGNAL(valueChanged(int)), this, SLOT(change(int)), Qt::QueuedConnection);
    }
    virtual ~SpinExclude() {}
    virtual int valueFromText(const QString &text) const override
    {
        if (text == "0 (ignore)")
            return 0;
        if (text == "-1 (exclude)")
            return -1;
        return QSpinBox::valueFromText(text);
    }
    virtual QString textFromValue(int value) const override
    {
        if (value == 0)
            return "0 (ignore)";
        if (value == -1)
            return "-1 (exclude)";
        return QSpinBox::textFromValue(value);
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

class VariantCheckBox : public QCheckBox
{
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

class FilterDialog : public QDialog
{
    Q_OBJECT

public:

    explicit FilterDialog(FormConditions *parent, Config *config, int mc, QListWidgetItem *item = 0, Condition *initcond = 0);
    virtual ~FilterDialog();

    void addVariant(QString name, int biome, int variant);
    void updateMode();
    void updateBiomeSelection();
    void enableSet(const int *ids, int n);
    int warnIfBad(Condition cond);

signals:
    void setCond(QListWidgetItem *item, Condition cond);

private slots:
    void on_comboBoxType_activated(int);

    void on_comboBoxRelative_activated(int);

    void on_buttonUncheck_clicked();

    void on_buttonInclude_clicked();

    void on_buttonExclude_clicked();

    void on_buttonArea_toggled(bool checked);

    void on_lineRadius_editingFinished();

    void on_buttonCancel_clicked();

    void on_buttonOk_clicked();

    void on_FilterDialog_finished(int result);

    void on_comboBoxCat_currentIndexChanged(int index);

    void on_checkStartPiece_stateChanged(int state);

private:
    Ui::FilterDialog *ui;
    QTextEdit *textDescription;

    QCheckBox *biomecboxes[256];
    SpinExclude *tempsboxes[9];
    QVector<VariantCheckBox*> variantboxes;
    bool custom;

public:
    Config *config;
    QListWidgetItem *item;
    Condition cond;
    int mc;
};

#endif // FILTERDIALOG_H
