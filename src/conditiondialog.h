#ifndef CONDITIONDIALOG_H
#define CONDITIONDIALOG_H

#include "search.h"
#include "formconditions.h"
#include "rangeslider.h"
#include "util.h"

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QTextEdit>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QComboBox>
#include <QStyledItemDelegate>
#include <QStandardItemModel>

class MainWindow;

namespace Ui {
class ConditionDialog;
}

// QComboBox uses QItemDelegate, which would not support styles
class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ComboBoxDelegate(QObject *parent, QComboBox *cmb) : QStyledItemDelegate(parent), combo(cmb) {}

    static bool isSeparator(const QModelIndex &index)
    {
        return index.data(Qt::AccessibleDescriptionRole).toString() == QString::fromLatin1("separator");
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (isSeparator(index))
        {
            QStyleOptionViewItem opt = option;
            if (const QAbstractItemView *view = qobject_cast<const QAbstractItemView*>(option.widget))
                opt.rect.setWidth(view->viewport()->width());
            combo->style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, painter, combo);
        }
        else
        {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (isSeparator(index))
        {
            int pm = combo->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, combo) + 4;
            return QSize(pm, pm);
        }
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        if (size.height() < combo->iconSize().height() + 1)
            size.setHeight(combo->iconSize().height() + 1);
        return size;
    }

private:
    QComboBox *combo;
};

// QLineEdit defaults to a style hint width 17 characters, which is too long for coordinates
class CoordEdit : public QLineEdit
{
    Q_OBJECT
public:
    CoordEdit(QWidget *parent = nullptr) : QLineEdit(parent) {}

    virtual QSize sizeHint() const override
    {
        QSize size = QLineEdit::minimumSizeHint();
        QFontMetrics fm(font());
        size.setWidth(size.width() + fm.horizontalAdvance("-30000000"));
        return size;
    }
};

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
    VariantCheckBox(const StartPiece *sp) : QCheckBox(sp->name),sp(sp) {}
    virtual ~VariantCheckBox() {}
    const StartPiece *sp;
};


class ConditionDialog : public QDialog
{
    Q_OBJECT

public:

    explicit ConditionDialog(FormConditions *parent, Config *config, int mc, QListWidgetItem *item = 0, Condition *initcond = 0);
    virtual ~ConditionDialog();

    void addTempCat(int temp, QString name);
    void updateMode();
    void updateBiomeSelection();
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

    void on_comboBoxCat_currentIndexChanged(int);

    void onCheckStartChanged(int state);
    void onClimateLimitChanged();

    void on_lineBiomeSize_textChanged(const QString &text);

    void on_comboLua_currentIndexChanged(int index);
    void on_pushLuaSaveAs_clicked();
    void on_pushLuaSave_clicked();
    void on_pushLuaOpen_clicked();
    void on_pushLuaExample_clicked();

    void on_comboHeightRange_currentIndexChanged(int index);

    void on_pushInfoLua_clicked();

    void on_comboClimatePara_currentIndexChanged(int index);

    void on_comboY_currentTextChanged(const QString &text);
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
    Config *config;
    QListWidgetItem *item;
    Condition cond;
    int mc;
};

#endif // CONDITIONDIALOG_H
