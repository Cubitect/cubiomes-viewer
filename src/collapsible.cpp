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

    int height = content->size().height();
    if (height)
        contentHeight = height;

    if (collapsed)
    {
        content->setMinimumHeight(0);
        content->setMaximumHeight(0);
        toggleButton->setArrowType(Qt::ArrowType::DownArrow);
        animgroup->setDirection(QAbstractAnimation::Forward);
    }
    else
    {
        toggleButton->setArrowType(Qt::ArrowType::RightArrow);
        animgroup->setDirection(QAbstractAnimation::Backward);
    }

    for (int i = 0, n = animgroup->animationCount(); i < n; i++)
    {
        QPropertyAnimation *anim;
        anim = (QPropertyAnimation*) animgroup->animationAt(i);
        anim->setStartValue(0);
        anim->setEndValue(contentHeight);
        anim->setDuration(200);
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
    layoutContent->addWidget(widget);
    contentHeight = widget->sizeHint().height();
    content = widget;
    animgroup->addAnimation(new QPropertyAnimation(content, "minimumHeight"));
    animgroup->addAnimation(new QPropertyAnimation(content, "maximumHeight"));
    setCollapsed(collapsed);
}

void Collapsible::setCollapsed(bool collapsed)
{
    if (collapsed)
    {
        toggleButton->setChecked(false);
        content->setMinimumHeight(0);
        content->setMaximumHeight(0);
        toggleButton->setArrowType(Qt::ArrowType::RightArrow);
        animgroup->setDirection(QAbstractAnimation::Backward);
    }
    else
    {
        toggleButton->setChecked(true);
        content->setMinimumHeight(0);
        content->setMaximumHeight(16777215);
        toggleButton->setArrowType(Qt::ArrowType::DownArrow);
        animgroup->setDirection(QAbstractAnimation::Forward);
    }
    animgroup->stop();
}

void Collapsible::setInfo(const QString& title, const QString& text)
{
    QPushButton *button = new QPushButton("", this);
    button->setStyleSheet(
      "QPushButton {"
      "    background-color: rgba(255, 255, 255, 0);"
      "    border: none;"
      "    image: url(:/icons/info.png);"
      "    image-position: right;"
      "}"
      "QPushButton:hover {"
      "    image: url(:/icons/info_h.png);"
      "}");
    button->setToolTip(tr("Show help"));

    connect(button, SIGNAL(clicked()), this, SLOT(showInfo()));

    layoutBar->addWidget(button, Qt::AlignLeft);
    infotitle = title;
    infomsg = text;
}

void Collapsible::showInfo()
{
    // windows plays an annoying sound when you use QMessageBox::information()
    QMessageBox mb(this);
    mb.setIcon(QMessageBox::Information);
    mb.setWindowTitle(infotitle);
    mb.setText(infomsg);
    mb.exec();
}


