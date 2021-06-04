#ifndef COLLAPSABLE_H
#define COLLAPSABLE_H

#include <QWidget>
#include <QParallelAnimationGroup>
#include <QToolButton>
#include <QFrame>
#include <QHBoxLayout>

class Collapsible : public QWidget
{
    Q_OBJECT
public:
    explicit Collapsible(QWidget *parent = nullptr);
    void init(const QString& title, QWidget *widget, bool collapsed);
    void setInfo(const QString& title, const QString& text);

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

#endif // COLLAPSABLE_H
