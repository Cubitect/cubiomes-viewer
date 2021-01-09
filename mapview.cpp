#include "mapview.h"
#include "cutil.h"

#include <QPainter>
#include <QTimer>
#include <QThread>
#include <QApplication>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QThreadPool>
#include <QTime>

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

    const char *bname = biome2str(id);
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
, blocks2pix(1.0/12)
, focusx(),focusz()
, prevx(),prevz()
, velx(), velz()
, holding()
, mstart(),mprev()
, updatecounter()
, sshow()
{
    memset(sshow, 0, sizeof(sshow));

    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&MapView::timeout));
    timer->start(50);

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

void MapView::setSeed(int mc, int64_t s)
{
    prevx = focusx;
    prevz = focusz;
    velx = velz = 0;
    if (world == NULL || world->mc != mc || world->seed != s)
    {
        delete world;
        world = new QWorld(mc, s);
    }
    for (int i = 0; i < STRUCT_NUM; i++)
        world->sshow[i] = sshow[i];
    update(2);
}

void MapView::setShow(int stype, bool v)
{
    sshow[stype] = v;
    if (world)
        for (int s = 0; s < STRUCT_NUM; s++)
            world->sshow[s] = sshow[s];
    update(2);
}

void MapView::setView(qreal x, qreal z)
{
    prevx = focusx = x;
    prevz = focusz = z;
    velx = velz = 0;
    update(2);
}

void MapView::timeout()
{
    qreal dt = 1e-3 * timer->interval(); //elapsed1.nsecsElapsed() * 1e-9;
    //elapsed1.start();

    frameelapsed.start();

    if (!holding)
    {
        qreal m = 1.0 - 3*dt; if (m < 0) m = 0;
        velx *= m;
        velz *= m;
        focusx += velx * dt;
        focusz += velz * dt;
    }
    else
    {
        velx = (focusx - prevx) / dt;
        velz = (focusz - prevz) / dt;
        prevx = focusx;
        prevz = focusz;
    }

    if ((velx*velx + velz*velz) * (blocks2pix*blocks2pix) < 100)
    {
        velx = 0;
        velz = 0;
    }
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

    qreal dt = frameelapsed.nsecsElapsed() * 1e-9;
    qreal fx = focusx;
    qreal fz = focusz;

    if (!holding)
    {
        fx += velx * dt;
        fz += velz * dt;
    }

    if (world)
    {
        world->draw(painter, width(), height(), fx, fz, blocks2pix);

        QPoint cur = mapFromGlobal(QCursor::pos());
        qreal bx = (cur.x() -  width()/2) / blocks2pix + fx;
        qreal bz = (cur.y() - height()/2) / blocks2pix + fz;
        Pos p = {(int)bx, (int)bz};
        overlay->pos = p;
        overlay->id = getBiomeAtPos(&world->g, p);

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
    update();//repaint();
}

void MapView::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        mprev = mstart = e->pos();
        holding = true;
        prevx = focusx;
        prevz = focusz;
        velx = 0;
        velz = 0;

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
        focusx = focusx - d.x() / blocks2pix;
        focusz = focusz - d.y() / blocks2pix;
        mprev = e->pos();
        update();//repaint();
    }
}

void MapView::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && holding)
    {
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
