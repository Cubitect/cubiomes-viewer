#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "quad.h"

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>


class MapOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit MapOverlay(QWidget *parent = nullptr)
        : QWidget(parent),pos{},id(-1) {}
    ~MapOverlay() {}

public slots:
    bool event(QEvent *) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

public:
    Pos pos;
    int id;
};

class MapView : public QWidget
{
    Q_OBJECT

public:
    explicit MapView(QWidget *parent = nullptr);
    ~MapView();

    qreal getX() const { return focusx; }
    qreal getZ() const { return focusz; }
    qreal getScale() const { return 1.0 / blocks2pix; }

    void setSeed(int mc, int64_t s);
    void setShow(int stype, bool v);
    void setView(qreal x, qreal z, qreal scale = 0);

    void timeout();

    void update(int cnt = 1);

    Pos getActivePos();

signals:

public slots:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;

    void wheelEvent(QWheelEvent *) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;

    void keyPressEvent(QKeyEvent *) Q_DECL_OVERRIDE;

public:
    QWorld *world;

    QTimer *timer;
    QElapsedTimer elapsed1;
    QElapsedTimer frameelapsed;

    MapOverlay *overlay;

private:
    qreal blocks2pix;
    qreal focusx, focusz;
    qreal prevx, prevz;
    qreal velx, velz;

    bool holding;
    QPoint mstart, mprev;
    int updatecounter;

    bool sshow[STRUCT_NUM];
};

#endif // MAPVIEW_H
