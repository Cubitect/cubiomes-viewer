#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "quad.h"

#include <QWidget>
#include <QElapsedTimer>

class MapOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit MapOverlay(QWidget *parent = nullptr)
        : QWidget(parent),pos{},bname(nullptr) {}
    ~MapOverlay() {}

public slots:
    bool event(QEvent *) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

public:
    Pos pos;
    const char *bname;
};

class MapView : public QWidget
{
    Q_OBJECT

public:
    explicit MapView(QWidget *parent = nullptr);
    ~MapView();

    qreal getX();
    qreal getZ();
    qreal getScale() const { return 1.0 / blocks2pix; }

    void deleteWorld();
    void refresh();
    void setSeed(WorldInfo wi, int dim);
    void setView(qreal x, qreal z, qreal scale = 0);

    bool getShow(int stype) { return stype >= 0 && stype < STRUCT_NUM ? sshow[stype] : false; }
    void setShow(int stype, bool v);
    void setShowBB(bool show);
    void setSmoothMotion(bool smooth);
    void setSetGridSpacing(int spacing);

    void timeout();

    void update(int cnt = 1);

    Pos getActivePos();

private:
    void settingsToWorld();

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

    QElapsedTimer elapsed1;
    QElapsedTimer frameelapsed;
    qreal decay;

    MapOverlay *overlay;

private:
    qreal blocks2pix;
    qreal focusx, focusz;
    qreal prevx, prevz;
    qreal velx, velz;
    qreal mtime;

    bool holding;
    QPoint mstart, mprev;
    int updatecounter;

    bool sshow[STRUCT_NUM];
    bool showBB;
    bool hasinertia;
    int gridspacing;
};

#endif // MAPVIEW_H
