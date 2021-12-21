#include "collapsible.h"

#include <QPropertyAnimation>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QPushButton>
#include <QMessageBox>

Collapsible::Collapsible(QWidget *parent)
    : QWidget(parent)
    , animgroup(new QParallelAnimationGroup(this))
    , toggleButton(new QToolButton(this))
    , frameBar(new QFrame(this))
    , content()
    , contentHeight()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    QFont font = toggleButton->font();
    //font.setPixelSize(12);
    font.setBold(true);
    toggleButton->setFont(font);
    toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggleButton->setArrowType(Qt::ArrowType::RightArrow);
    toggleButton->setCheckable(true);


    frameBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    frameBar->setFrameShape(QFrame::HLine);

    QVBoxLayout *vbox = new QVBoxLayout();
    layoutBar = new QHBoxLayout();
    layoutBar->addWidget(toggleButton, Qt::AlignBottom);
    layoutBar->addWidget(frameBar, Qt::AlignCenter);
    layoutBar->setSpacing(5);
    vbox->addLayout(layoutBar);

    layoutContent = new QHBoxLayout();
    layoutContent->setContentsMargins(20, 0, 0, 0);
    vbox->addLayout(layoutContent);

    vbox->setSizeConstraint(QLayout::SetMaximumSize);
    vbox->setContentsMargins(0, 0, 0, 8);
    vbox->setSpacing(0);
    setLayout(vbox);

    connect(toggleButton, &QToolButton::toggled, this, &Collapsible::toggle);
    connect(animgroup, &QAbstractAnimation::finished, this, &Collapsible::finishAnimation);
}

void Collapsible::toggle(bool collapsed)
{
    if (!content)
        return;

    if (collapsed)
    {
        content->setMinimumHeight(0);
        content->setMaximumHeight(0);
        toggleButton->setArrowType(Qt::ArrowType::DownArrow);
        animgroup->setDirection(QAbstractAnimation::Forward);
    }
    else
    {
        contentHeight = content->size().height();
        toggleButton->setArrowType(Qt::ArrowType::RightArrow);
        animgroup->setDirection(QAbstractAnimation::Backward);
    }

    for (int i = 0, n = animgroup->animationCount(); i < n; i++)
    {
        QPropertyAnimation *anim;
        anim = (QPropertyAnimation*) animgroup->animationAt(i);
        anim->setStartValue(0);
        anim->setEndValue(contentHeight);
        anim->setDuration(150);
    }

    animgroup->start();
}

void Collapsible::finishAnimation()
{
    if (toggleButton->isChecked())
    {
        content->setMinimumHeight(0);
        content->setMaximumHeight(16777215);
    }
}

void Collapsible::init(const QString& title, QWidget *widget, bool collapsed)
{
    toggleButton->setText(title);
    toggleButton->setChecked(!collapsed);
    layoutContent->addWidget(widget);
    contentHeight = widget->sizeHint().height();
    content = widget;
    animgroup->addAnimation(new QPropertyAnimation(content, "minimumHeight"));
    animgroup->addAnimation(new QPropertyAnimation(content, "maximumHeight"));
    if (collapsed)
    {
        content->setMinimumHeight(0);
        content->setMaximumHeight(0);
    }
}

void Collapsible::setInfo(const QString& title, const QString& text)
{
    QPixmap pixmap = QPixmap(":/icons/info.png");
    QPushButton *button = new QPushButton(pixmap, "", this);
    button->setStyleSheet("border: none;");
    button->setIconSize(pixmap.rect().size());
    button->setToolTip(tr("Show help"));
    button->setMaximumSize(14, 14);

    connect(button, SIGNAL(clicked()), this, SLOT(showInfo()));

    layoutBar->addWidget(button, Qt::AlignLeft);
    infotitle = title;
    infomsg = text;
}

void Collapsible::showInfo()
{
    QMessageBox::information(this, infotitle, infomsg, QMessageBox::Ok);
}
