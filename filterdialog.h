#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>

#include "mainwindow.h"
#include "search.h"

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
        if (v < 0)
            setStyleSheet("background: lightcoral");
        else if (v > 0)
            setStyleSheet("background: lightgreen");
        else
            setStyleSheet("");
        findChild<QLineEdit*>()->deselect();
    }
};

class FilterDialog : public QDialog
{
    Q_OBJECT

public:

    explicit FilterDialog(MainWindow *parent, QListWidgetItem *item = 0, Condition *initcond = 0);
    ~FilterDialog();

    void updateMode();
    void updateBiomeSelection();

signals:
    void setCond(QListWidgetItem *item, Condition cond);

private slots:
    void on_comboBoxType_activated(int);

    void on_buttonUncheck_clicked();

    void on_buttonInclude_clicked();

    void on_buttonExclude_clicked();

    void on_buttonArea_toggled(bool checked);

    void on_lineRadius_editingFinished();

    void on_buttonCancel_clicked();

    void on_buttonOk_clicked();

    void on_FilterDialog_finished(int result);

private:
    Ui::FilterDialog *ui;
    QCheckBox *biomecboxes[256];
    SpinExclude *tempsboxes[9];
    bool custom;

public:
    QListWidgetItem *item;
    Condition cond;
};

#endif // FILTERDIALOG_H
