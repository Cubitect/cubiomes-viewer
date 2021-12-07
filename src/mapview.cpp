#include "mapview.h"
#include "cutil.h"

#include <QPainter>
#include <QThread>
#include <QApplication>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QThreadPool>
#include <QTime>
#include <QSettings>

#include <math.h>

bool MapOverlay::event(QEvent *e)
{
    if (e->type() == QEvent::MouseMove)
        update();
    return QWidget::event(e);
}

void MapOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (bname)
    {
        QString s = QString::asprintf("%s [%d,%d]", bname, pos.x, pos.z);
        QRect r = painter.fontMetrics()
                .boundingRect(0, 0, width(), height(), Qt::AlignRight | Qt::AlignTop, s);

        painter.fillRect(r, QBrush(QColor(0, 0, 0, 128), Qt::SolidPattern));
        painter.setPen(Qt::white);
        painter.drawText(r, s);
    }
}

MapView::MapView(QWidget *parent)
: QWidget(parent)
, world()
, decay(2.0)
, blocks2pix(1.0/16)
, focusx(),focusz()
, prevx(),prevz()
, velx(), velz()
, holding()
, mstart(),mprev()
, updatecounter()
, sshow()
, hasinertia(true)
, gridspacing()
{
    memset(sshow, 0, sizeof(sshow));

    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);

    elapsed1.start();
    frameelapsed.start();

    overlay = new MapOverlay(this);
    overlay->setMouseTracking(true);

    setContextMenuPolicy(Qt::CustomContextMenu);
}

MapView::~MapView()
{
    delete world;
    delete overlay;
}

void MapView::deleteWorld()
{
    delete world;
    world = NULL;
}

void MapView::refresh()
{
    if (world)
    {
        WorldInfo wi = world->wi;
        int dim = world->dim;
        delete world;
        world = new QWorld(wi, dim);
    }
}

void MapView::setSeed(WorldInfo wi, int dim)
{
    prevx = focusx = getX();
    prevz = focusz = getZ();
    velx = velz = 0;
    if (world == NULL || !wi.equals(world->wi))
    {
        delete world;
        world = new QWorld(wi, dim);
    }
    else if (world->dim != dim)
    {
        world->setDim(dim);
    }
    settingsToWorld();
    update(2);
}

void MapView::setView(qreal x, qreal z, qreal scale)
{
    if (scale > 0)
    {
        blocks2pix = 1.0 / scale;
    }
    prevx = focusx = x;
    prevz = focusz = z;
    velx = velz = 0;
    update(2);
}

void MapView::setShow(int stype, bool v)
{
    sshow[stype] = v;
    settingsToWorld();
    update(2);
}

void MapView::setShowBB(bool show)
{
    showBB = show;
    settingsToWorld();
    update(2);
}

void MapView::setSmoothMotion(bool smooth)
{
    hasinertia = smooth;
}

void MapView::setSetGridSpacing(int spacing)
{
    gridspacing = spacing;
    settingsToWorld();
    update(2);
}

void MapView::settingsToWorld()
{
    if (!world)
        return;
    for (int s = 0; s < STRUCT_NUM; s++)
        world->sshow[s] = sshow[s];
    world->showBB = showBB;
    world->gridspacing = gridspacing;
}

qreal MapView::getX()
{
    qreal dt = frameelapsed.nsecsElapsed() * 1e-9;
    qreal fx = focusx;
    if (velx)
    {
        qreal df = 1.0 - exp(-decay*dt);
        fx += velx * df;
        if (df > 0.998)
        {
            focusx = fx;
            velx = 0;
        }
    }
    return fx;
}

qreal MapView::getZ()
{
    qreal dt = frameelapsed.nsecsElapsed() * 1e-9;
    qreal fz = focusz;
    if (velz)
    {
        qreal df = 1.0 - exp(-decay*dt);
        fz += velz * df;
        if (df > 0.998)
        {
            focusz = fz;
            velz = 0;
        }
    }
    return fz;
}

void MapView::update(int cnt)
{
    updatecounter = cnt;
    QWidget::update();
}

Pos MapView::getActivePos()
{
    Pos p = overlay->pos;
    if (world && world->seltype != D_NONE)
        p = world->selpos;
    return p;
}

void MapView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);

    qreal fx = getX();
    qreal fz = getZ();

    if (world)
    {
        world->draw(painter, width(), height(), fx, fz, blocks2pix);

        QPoint cur = mapFromGlobal(QCursor::pos());
        qreal bx = (cur.x() -  width()/2.0) / blocks2pix + fx;
        qreal bz = (cur.y() - height()/2.0) / blocks2pix + fz;
        Pos p = {(int)bx, (int)bz};
        overlay->pos = p;
        overlay->bname = biome2str(world->wi.mc, world->getBiome(p));

        if (QThreadPool::globalInstance()->activeThreadCount() > 0 || velx || velz)
            updatecounter = 2;
        if (updatecounter > 0)
        {
            updatecounter--;
            QWidget::update();
        }
    }
}

void MapView::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    overlay->resize(width(), height());
}

void MapView::wheelEvent(QWheelEvent *e)
{
    const qreal ang = e->angleDelta().y() / 8; // e->delta() / 8;
    blocks2pix *= pow(2, ang/100);
    qreal scalemin = 128.0, scalemax = 1.0 / 1024.0;
    if (blocks2pix > scalemin) blocks2pix = scalemin;
    if (blocks2pix < scalemax) blocks2pix = scalemax;
    update();//repaint();
}

void MapView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        mprev = mstart = e->pos();
        holding = true;
        prevx = focusx = getX();
        prevz = focusz = getZ();
        velx = 0;
        velz = 0;
        elapsed1.start();
        frameelapsed.start();

        if (world)
        {
            world->selx = mstart.x();
            world->selz = mstart.y();
            world->seldo = true;
            update();
        }
    }
}

void MapView::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::LeftButton) && holding)
    {
        QPoint d = e->pos() - mprev;
        qreal dt = elapsed1.nsecsElapsed() * 1e-9;
        if (dt > .005)
        {
            mtime = dt;
            prevx = focusx;
            prevz = focusz;
            focusx = focusx - d.x() / blocks2pix;
            focusz = focusz - d.y() / blocks2pix;
            elapsed1.start();
        }
        mprev = e->pos();
        update();//repaint();
    }
}

void MapView::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && holding)
    {
        mtime += elapsed1.nsecsElapsed() * 1e-9;
        if (mtime > .0)
        {
            elapsed1.start();
            QPoint d = e->pos() - mprev;
            focusx = focusx - d.x() / blocks2pix;
            focusz = focusz - d.y() / blocks2pix;
            velx = (focusx - prevx) / mtime;
            velz = (focusz - prevz) / mtime;
            frameelapsed.start();
            update();
        }
        if (!hasinertia)
        {
            velx = velz = 0;
        }

        holding = false;
        mprev = e->pos();

        if (world && e->pos() == mstart)
        {
            world->selx = mstart.x();
            world->selz = mstart.y();
            world->seldo = true;
            world->seltype = D_NONE;
        }
    }
}

void MapView::keyPressEvent(QKeyEvent *)
{
}
