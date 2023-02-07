#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "world.h"

#include <QDockWidget>
#include <QElapsedTimer>

class MapOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit MapOverlay(QWidget *parent = nullptr)
        : QWidget(parent),pos{0,0},bname() {}
    ~MapOverlay() {}

public slots:
    bool event(QEvent *) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

public:
    Pos pos;
    QString bname;
};

class MapView : public QWidget
{
    Q_OBJECT

public:
    explicit MapView(QWidget *parent = nullptr);
    virtual ~MapView();

    qreal getX();
    qreal getZ();
    qreal getScale() const { return 1.0 / blocks2pix; }

    void deleteWorld();
    void refresh();
    void setSeed(WorldInfo wi, int dim, LayerOpt lopt);
    void setView(qreal x, qreal z, qreal scale = 0);
    void animateView(qreal x, qreal z, qreal scale);
    qreal x_src, z_src, s_src;
    qreal x_dst, z_dst, s_dst;
    qreal s_mul;
    qreal atime;
    QElapsedTimer anielapsed;

    bool getShow(int stype) { return stype >= 0 && stype < STRUCT_NUM ? sshow[stype] : false; }
    void setShow(int stype, bool v);
    void setConfig(const Config& config);
    void refreshBiomeColors();

    void timeout();

    void update(int cnt = 1);

    Pos getActivePos();

    QPixmap screenshot();

private:
    void settingsToWorld();
    void runAni();

signals:
    void layerChange(int mode, int disp);

public slots:
    void mapUpdate();
    void showContextMenu(const QPoint &pos);
    void copySeed();
    void copyText(QString txt);
    void onGoto();

    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *) Q_DECL_OVERRIDE;

    void wheelEvent(QWheelEvent *) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;

    void keyReleaseEvent(QKeyEvent *) Q_DECL_OVERRIDE;

public:
    QWorld *world;

    QElapsedTimer elapsed1;
    QElapsedTimer frameelapsed;
    QElapsedTimer actelapsed;
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
    Pos bstart;
    bool measure;
    int updatecounter;

    bool sshow[STRUCT_NUM];
    LayerOpt lopt;
    Config config;
};

#endif // MAPVIEW_H
