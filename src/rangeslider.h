#ifndef RANGESLIDER_H
#define RANGESLIDER_H

#include <QSlider>
#include <QLabel>

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


#endif // RANGESLIDER_H
