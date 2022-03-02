#include "rangeslider.h"

#include <QStyleOptionSlider>
#include <QPainter>
#include <QMouseEvent>
#include <QBoxLayout>
#include <QApplication>

RangeSlider::RangeSlider(QWidget *parent, int vmin, int vmax)
    : QSlider(parent)
    , vmin(vmin)
    , vmax(vmax)
    , pos0(vmin)
    , pos1(vmax)
    , holding(0)
{
    setRange(vmin, vmax);
    setOrientation(Qt::Horizontal);

    // drawComplexControl() draws the background on every call if there is a stylesheet
    // to mitigate this, we set the background transparent
    setStyleSheet(
        "QSlider { background-color: rgba(180, 180, 180, 0); }\n"
        "QSlider::sub-page { background-color: rgba(255, 255, 255, 0); }"
    );
    colinner = palette().color(QPalette::Highlight);
    colouter = QColor(0, 0, 0, 0);
}

RangeSlider::~RangeSlider()
{
}

void RangeSlider::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QStyleOptionSlider opt;
    initStyleOption(&opt);

    // draw the back groove of the slider without range highlighting
    opt.subControls = QStyle::SC_SliderGroove;
    opt.sliderValue = opt.sliderPosition = vmin;
    style()->drawComplexControl(QStyle::CC_Slider, &opt, &painter, this);

    // draw the range highlighting between the handles
    QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    opt.sliderValue = opt.sliderPosition = pos0;
    QRect handle0 = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    opt.sliderValue = opt.sliderPosition = pos1;
    QRect handle1 = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    if (colinner.alpha() > 0)
    {
        painter.setBrush(QBrush(colinner));
        painter.setPen(QPen(colinner, 0));
        int x0 = handle0.center().x();
        int x1 = handle1.center().x();
        int y  = handle0.center().y();
        QRect span = QRect(x0, y-1, x1-x0, 3);
        painter.drawRect(groove.intersected(span));
    }
    if (colouter.alpha() > 0)
    {
        painter.setBrush(QBrush(colouter));
        painter.setPen(QPen(colouter, 0));
        int x0 = handle0.center().x();
        int x1 = handle1.center().x();
        int y  = handle0.center().y();
        QRect left = QRect(groove.x()+1, y-1, x0-groove.x()-2, 3);
        painter.drawRect(groove.intersected(left));
        QRect right = QRect(x1+1, y-1, groove.right()-x1-2, 3);
        painter.drawRect(groove.intersected(right));
    }

    int pmin = pos0 < pos1 ? pos0 : pos1;
    int pmax = pos0 > pos1 ? pos0 : pos1;
    // draw handles
    opt.subControls = QStyle::SC_SliderHandle;
    opt.sliderValue = opt.sliderPosition = pmax;
    style()->drawComplexControl(QStyle::CC_Slider, &opt, &painter, this);
    opt.sliderValue = opt.sliderPosition = pmin;
    style()->drawComplexControl(QStyle::CC_Slider, &opt, &painter, this);
}


void RangeSlider::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        // determine which handle we are holding if any
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        QStyle::SubControl hit;
        holding = 0;
        opt.sliderValue = opt.sliderPosition = pos0;
        hit = style()->hitTestComplexControl(QStyle::CC_Slider, &opt, e->pos(), this);
        if (hit == QStyle::SC_SliderHandle)
            holding = -1;
        opt.sliderValue = opt.sliderPosition = pos1;
        hit = style()->hitTestComplexControl(QStyle::CC_Slider, &opt, e->pos(), this);
        if (hit == QStyle::SC_SliderHandle)
            holding = +1;
    }

    QSlider::mousePressEvent(e);
}

void RangeSlider::mouseReleaseEvent(QMouseEvent *e)
{
    holding = 0;
    if (pos0 > pos1)
    {
        int tmp = pos0;
        pos0 = pos1;
        pos1 = tmp;
    }
    emit valueChanged(0);
    QSlider::mouseReleaseEvent(e);
}

void RangeSlider::wheelEvent(QWheelEvent *e)
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);

    int delta = e->angleDelta().y() / 8 / 15;
    int x;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    x = e->x() - groove.x();
#else
    x = (int)e->position().x() - groove.x();
#endif
    x = style()->sliderValueFromPosition(vmin, vmax, x, groove.width());

    int h = 0;
    if (x < pos0)
        h = -1;
    else if (x > pos1)
        h = +1;
    else if (abs(pos0 - x) < abs(pos1 - x))
        h = -1;
    else if (abs(pos0 - x) > abs(pos1 - x))
        h = +1;

    if (e->modifiers() & Qt::ControlModifier)
        delta *= 50;

    if (h < 0)
    {
        pos0 += delta;
        if (pos0 < vmin) pos0 = vmin;
        if (pos0 > pos1) pos0 = pos1;
        emit valueChanged(pos0);
    }
    else
    {
        pos1 += delta;
        if (pos1 > vmax) pos1 = vmax;
        if (pos1 < pos0) pos1 = pos0;
        emit valueChanged(pos1);
    }

    update();
}

void RangeSlider::mouseMoveEvent(QMouseEvent *e)
{
    if (holding == 0)
        return;

    QStyleOptionSlider opt;
    initStyleOption(&opt);

    opt.sliderValue = opt.sliderPosition = holding < 0 ? pos0 : pos1;

    QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);

    int x = e->x() - groove.x();
    x = style()->sliderValueFromPosition(vmin, vmax, x, groove.width());

    if (holding < 0)
        pos0 = x;
    else
        pos1 = x;

    emit valueChanged(x);
    update();
}


LabeledRange::LabeledRange(QWidget *parent, int vmin, int vmax)
    : QWidget(parent)
{
    slider = new RangeSlider(this, vmin, vmax);
    minlabel = new QLabel(this);
    maxlabel = new QLabel(this);
    minlabel->setAlignment(Qt::AlignRight);
    maxlabel->setAlignment(Qt::AlignLeft);
    int w = fontMetrics().tightBoundingRect("+012345").width();
    minlabel->setFixedWidth(w);
    maxlabel->setFixedWidth(w);

    QBoxLayout *l = new QBoxLayout(QBoxLayout::LeftToRight, this);
    l->addWidget(minlabel);
    l->addWidget(slider);
    l->addWidget(maxlabel);
    l->setMargin(0);
    setLayout(l);

    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(rangeChanged(void)));
    rangeChanged();
}

LabeledRange::~LabeledRange()
{
}

void LabeledRange::setLimitText(QString min, QString max)
{
    mintxt = min;
    maxtxt = max;
    rangeChanged();
}

void LabeledRange::setValues(int p0, int p1)
{
    slider->pos0 = p0;
    slider->pos1 = p1;
    slider->update();
    rangeChanged();
}

void LabeledRange::setHighlight(QColor inner, QColor outer)
{
    slider->colinner = inner;
    slider->colouter = outer;
    slider->update();
}

void LabeledRange::rangeChanged()
{
    int pmin = slider->pos0 < slider->pos1 ? slider->pos0 : slider->pos1;
    int pmax = slider->pos0 > slider->pos1 ? slider->pos0 : slider->pos1;
    minlabel->setText(QString::number(pmin));
    maxlabel->setText(QString::number(pmax));
    if (!mintxt.isEmpty() && pmin == slider->vmin)
        minlabel->setText(mintxt);
    if (!maxtxt.isEmpty() && pmax == slider->vmax)
        maxlabel->setText(maxtxt);
    emit onRangeChange();
}


