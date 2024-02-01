#ifndef WIDGETS_H
#define WIDGETS_H

#include <QWidget>
#include <QSlider>
#include <QComboBox>
#include <QStyledItemDelegate>
#include <QSpinBox>
#include <QLineEdit>

class QParallelAnimationGroup;
class QToolButton;
class QFrame;
class QHBoxLayout;
class QLabel;

class Collapsible : public QWidget
{
    Q_OBJECT
public:
    explicit Collapsible(QWidget *parent = nullptr);
    void init(const QString& title, QWidget *widget, bool collapsed);
    void setInfo(const QString& title, const QString& text);
    void setCollapsed(bool collapsed); // without animation

public slots:
    void toggle(bool collapsed);
    void finishAnimation();
    void showInfo();

public:
    QParallelAnimationGroup* animgroup;
    QToolButton* toggleButton;
    QFrame* frameBar;
    QWidget *content;
    QHBoxLayout *layoutBar;
    QHBoxLayout *layoutContent;

    int contentHeight;
    QString infotitle;
    QString infomsg;
};

class RangeSlider : public QSlider
{
    Q_OBJECT
public:
    RangeSlider(QWidget *parent = nullptr, int vmin = 0, int vmax = 100);
    virtual ~RangeSlider();

    virtual void paintEvent(QPaintEvent *e) override;
    virtual void wheelEvent(QWheelEvent *e) override;
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;

    int vmin, vmax;
    int pos0, pos1;
    int holding;
    QColor colinner, colouter;
};

class LabeledRange : public QWidget
{
    Q_OBJECT
public:
    LabeledRange(QWidget *parent = nullptr, int vmin = 0, int vmax = 10);
    virtual ~LabeledRange();

    void setValues(int p0, int p1);
    void setLimitText(QString min, QString max);
    void setHighlight(QColor inner, QColor outer);

public slots:
    void rangeChanged();

signals:
    void onRangeChange();

public:
    RangeSlider *slider;
    QLabel *minlabel, *maxlabel;
    QString mintxt, maxtxt;
};


// QLineEdit defaults to a style hint width 17 characters, which is too long for coordinates
class CoordEdit : public QLineEdit
{
    Q_OBJECT
public:
    CoordEdit(QWidget *parent = nullptr) : QLineEdit(parent) {}
    virtual QSize sizeHint() const override;
};

class SpinExclude : public QSpinBox
{
    Q_OBJECT
public:
    SpinExclude(QWidget *parent = nullptr);
    virtual ~SpinExclude();
    virtual int valueFromText(const QString &text) const override;
    virtual QString textFromValue(int value) const override;

public slots:
    void change(int v);
};

class SpinInstances : public QSpinBox
{
    Q_OBJECT
public:
    SpinInstances(QWidget *parent = nullptr);
    virtual ~SpinInstances();
    virtual int valueFromText(const QString &text) const override;
    virtual QString textFromValue(int value) const override;
};

// QComboBox uses QItemDelegate, which would not support styles
class ComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ComboBoxDelegate(QObject *parent, QComboBox *cmb);
    virtual ~ComboBoxDelegate();
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QComboBox *combo;
};

class StyledComboBox : public QComboBox
{
    Q_OBJECT
public:
    StyledComboBox(QWidget *parent) : QComboBox(parent)
    {
        setItemDelegate(new ComboBoxDelegate(parent, this));
    }
};

#endif // WIDGETS_H
