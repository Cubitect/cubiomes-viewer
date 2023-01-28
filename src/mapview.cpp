#include "mapview.h"
#include "gotodialog.h"
#include "cutil.h"

#include <QPainter>
#include <QThread>
#include <QApplication>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QThreadPool>
#include <QTime>
#include <QSettings>
#include <QMenu>
#include <QAction>
#include <QClipboard>

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
    QString s = bname + QString::asprintf(" [%d,%d]", pos.x, pos.z);
    QRect r = painter.fontMetrics()
            .boundingRect(0, 0, width(), height(), Qt::AlignRight | Qt::AlignTop, s);

    painter.fillRect(r, QBrush(QColor(0, 0, 0, 128), Qt::SolidPattern));
    painter.setPen(Qt::white);
    painter.drawText(r, s);
}

MapView::MapView(QWidget *parent)
    : QWidget(parent)
    , world()
    , decay(2.0)
    , blocks2pix(1.0/16)
    , focusx(),focusz()
    , prevx(),prevz()
    , velx(), velz()
    , mtime()
    , holding()
    , mstart(),mprev()
    , bstart()
    , measure()
    , updatecounter()
    , layeropt(LOPT_DEFAULT_1)
    , config()
{
    memset(sshow, 0, sizeof(sshow));

    QFont mono = *gp_font_mono;
    setFont(mono);

    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(pal);

    elapsed1.start();
    frameelapsed.start();
    actelapsed.start();

    overlay = new MapOverlay(this);
    overlay->setMouseTracking(true);
    overlay->setFont(mono);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

MapView::~MapView()
{
    delete world;
    delete overlay;
}

void MapView::deleteWorld()
{
    if (world)
    {
        disconnect(world);
        delete world;
        world = nullptr;
    }
}

void MapView::refresh()
{
    if (world)
    {
        WorldInfo wi = world->wi;
        int dim = world->dim;
        deleteWorld();
        world = new QWorld(wi, dim, layeropt);
        QObject::connect(world, &QWorld::update, this, &MapView::mapUpdate);
    }
}

void MapView::setSeed(WorldInfo wi, int dim, int lopt)
{
    prevx = focusx = getX();
    prevz = focusz = getZ();
    velx = velz = 0;
    if (lopt >= 0)
        layeropt = lopt;

    if (world == NULL || !wi.equals(world->wi) ||
        world->layeropt == LOPT_STRUCTS || layeropt == LOPT_STRUCTS)
    {
        deleteWorld();
        world = new QWorld(wi, dim, layeropt);
        QObject::connect(world, &QWorld::update, this, &MapView::mapUpdate);
    }
    else if (world->dim != dim || world->layeropt != layeropt)
    {
        world->setDim(dim, layeropt);
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

void MapView::animateView(qreal x_dst, qreal z_dst, qreal s_dst)
{
    if (s_dst <= 0)
        s_dst = 1.0 / blocks2pix;
    this->x_src = prevx = focusx = getX();
    this->z_src = prevz = focusz = getZ();
    this->s_src = 1.0 / blocks2pix;
    this->x_dst = x_dst;
    this->z_dst = z_dst;
    this->s_dst = s_dst;
    qreal dx = x_dst - x_src;
    qreal dz = z_dst - z_src;
    qreal ds = s_dst - s_src;
    qreal d = sqrt(dx*dx + dz*dz + ds*ds*1024);
    this->s_mul = d * 1e-3;
    this->atime = sqrt(d) * 5e7;
    anielapsed.start();
    update(2);
}

void MapView::setShow(int stype, bool v)
{
    sshow[stype] = v;
    settingsToWorld();
    update(2);
}

void MapView::setConfig(const Config& c)
{
    config = c;
    settingsToWorld();
    update(2);
}

void MapView::refreshBiomeColors()
{
    if (world)
        world->refreshBiomeColors();
}

void MapView::settingsToWorld()
{
    if (!world)
        return;
    for (int s = 0; s < STRUCT_NUM; s++)
        world->sshow[s] = sshow[s];
    world->showBB = config.showBBoxes;
    world->heightvis = config.heightVis;
    world->gridspacing = config.gridSpacing;
    world->gridmultiplier = config.gridMultiplier;
    world->memlimit = (uint64_t) config.mapCacheSize * 1024 * 1024;
    world->layeropt = layeropt;
}

static qreal smoothstep(qreal x)
{
    if (x < 0) return 0;
    if (x > 1) return 1;
    qreal v = x * x * x * (x * (x * 6 - 15) + 10);
    return v < 0 ? 0 : v > 1 ? 1 : v;
}

static qreal smoothstep_integral(qreal x)
{
    if (x < 0) return 0;
    if (x > 1) return 1;
    qreal x2 = x * x;
    qreal v = x2 * x2 * (x * (x - 3) + 2.5);
    return v < 0 ? 0 : v > 1 ? 1 : v;
}

static void smoothmotion(qreal *pos, qreal *vel, qreal x0, qreal x1, qreal t)
{
    // TODO: include the percieved travel distance due to scale changes
    qreal xm = (x0 + x1) / 2;
    qreal p, v;
    if (t < 0.5)
    {
        t = 2 * t;
        v = smoothstep(t);
        qreal u = 2 * smoothstep_integral(t);
        p = x0 + (xm - x0) * u;
    }
    else
    {
        t = 2 * (1 - t);
        v = smoothstep(t);
        qreal u = 1 - 2 * smoothstep_integral(t);
        p = xm + (x1 - xm) * u;
    }
    if (vel) *vel = v;
    if (pos) *pos = p;
}

qreal MapView::getX()
{
    if (anielapsed.isValid())
    {
        qreal t = anielapsed.nsecsElapsed() / atime;
        qreal u, v;
        smoothmotion(&u, 0, x_src, x_dst, t);
        smoothmotion(&v, 0, s_src, s_dst, t);
        blocks2pix = 1.0 / v;
        return focusx = u;
    }

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
    if (anielapsed.isValid())
    {
        qreal t = anielapsed.nsecsElapsed() / atime;
        qreal u, v;
        smoothmotion(&u, 0, z_src, z_dst, t);
        smoothmotion(&v, 0, s_src, s_dst, t);
        blocks2pix = 1.0 / v;
        return focusz = u;
    }

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
    if (world && world->selopt != D_NONE)
        p = world->selvp.p;
    return p;
}

void MapView::mapUpdate()
{
    update(1);
}

void MapView::showContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    menu.setFont(font());
    // this is a contextual temporary menu so shortcuts are only indicated here,
    // but will not function - see keyReleaseEvent() for shortcut implementation

    if (world)
    {
        mstart = pos;
        world->selx = mstart.x();
        world->selz = mstart.y();
        world->seldo = true;
        world->selopt = D_NONE;
        grab(); // invokes an immediate paint call
    }

    Pos p = getActivePos();
    QString seed   = world ? QString::asprintf("%" PRId64, (int64_t)world->wi.seed) : "";
    QString tp     = QString::asprintf("/tp @p %d ~ %d", p.x, p.z);
    QString coords = QString::asprintf("%d %d", p.x, p.z);
    QString chunk  = QString::asprintf("%d %d", p.x >> 4, p.z >> 4);

    menu.addAction(tr("Copy seed:  ")+seed, this, &MapView::copySeed, QKeySequence::Copy);
    menu.addAction(tr("Copy tp:    ")+tp, [=](){ this->copyText(tp); });
    menu.addAction(tr("Copy block: ")+coords, [=](){ this->copyText(coords); });
    menu.addAction(tr("Copy chunk: ")+chunk, [=](){ this->copyText(chunk); });
    menu.addAction(tr("Go to coordinates..."), this, &MapView::onGoto, QKeySequence(Qt::CTRL + Qt::Key_G));
   // menu.addAction(tr("Animation"), this, &MapView::runAni);
    menu.exec(mapToGlobal(pos));
}

void MapView::runAni()
{
}

void MapView::copySeed()
{
    if (world)
    {
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(QString::asprintf("%" PRId64, (int64_t)world->wi.seed));
    }
}

void MapView::copyText(QString txt)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(txt);
}

void MapView::onGoto()
{
    GotoDialog *dialog = new GotoDialog(this, getX(), getZ(), getScale());
    dialog->show();
}

void MapView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);

    qreal fx = getX();
    qreal fz = getZ();
    if (anielapsed.isValid() && anielapsed.nsecsElapsed() > atime)
    {
        anielapsed.invalidate();
        focusx = x_dst;
        focusz = z_dst;
        blocks2pix = 1.0 / s_dst;
    }
    if (fx < -INT_MAX) { focusx = fx = -INT_MAX; }
    if (fz < -INT_MAX) { focusz = fz = -INT_MAX; }
    if (fx > INT_MAX) { focusx = fx = INT_MAX; }
    if (fz > INT_MAX) { focusz = fz = INT_MAX; }

    if (world)
    {
        world->draw(painter, width(), height(), fx, fz, blocks2pix);

        QPoint cur = mapFromGlobal(QCursor::pos());
        qreal bx = (cur.x() -  width()/2.0) / blocks2pix + fx;
        qreal bz = (cur.y() - height()/2.0) / blocks2pix + fz;
        Pos p = {(int)bx, (int)bz};
        overlay->pos = p;
        overlay->bname = world->getBiomeName(p);

        if (measure)
        {
            qreal startx = (bstart.x - fx) * blocks2pix + width() / 2.0;
            qreal startz = (bstart.z - fz) * blocks2pix + height() / 2.0;
            QPointF start = {startx, startz};
            painter.setPen(QPen(QColor(0, 0, 0, 255), 2));
            painter.drawPoint(start);
            painter.setPen(QPen(QColor(255, 255, 128, 255), 1));
            painter.drawLine(start, cur);
            qreal dx = bstart.x - bx;
            qreal dz = bstart.z - bz;
            qreal r = sqrt(dx*dx + dz*dz);
            QString s = QString::asprintf("r=%g", floor(r));
            QRect textrec = painter.fontMetrics()
                    .boundingRect(0, 0, 0, 0, Qt::AlignLeft | Qt::AlignVCenter, s);
            textrec.translate(cur);
            painter.fillRect(textrec, QBrush(QColor(0, 0, 0, 128), Qt::SolidPattern));
            painter.setPen(QPen(QColor(255, 255, 128, 255), 1));
            painter.drawText(textrec, s);
        }

        bool active = world->isBusy();
        if (active || velx || velz || anielapsed.isValid())
            updatecounter = 2;
        if (updatecounter > 0)
        {
            updatecounter--;
            QWidget::update();

            if (active)
            {   // processing animation
                qreal cyc = actelapsed.nsecsElapsed() * 1e-9;
                qreal ang = 360 * (1.0 - (cyc - (int) cyc));
                int r = 20;
                QRect rec = QRect(r, height() - 2*r, r, r);

                QConicalGradient gradient;
                gradient.setCenter(rec.center());
                gradient.setAngle(ang);
                gradient.setColorAt(0, QColor(0, 0, 0, 192));
                gradient.setColorAt(1, QColor(255, 255, 255, 192));
                QPen pen(QBrush(gradient), 5, Qt::SolidLine, Qt::SquareCap);
                painter.setPen(pen);
                painter.drawRect(rec);
            }
        }
        else
        {
            actelapsed.start();
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
    qreal zoommin = 1.0 / 2048.0, zoommax = 128.0;
    const qreal ang = e->angleDelta().y() / 8; // e->delta() / 8;
    if (ang < 0 && blocks2pix < zoommin) return;
    if (ang > 0 && blocks2pix > zoommax) return;
    blocks2pix *= pow(2, ang/100);
    if (ang < 0 && blocks2pix < zoommin) blocks2pix = zoommin;
    if (ang > 0 && blocks2pix > zoommax) blocks2pix = zoommax;
    update();
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
        mtime = 0;
        elapsed1.start();
        frameelapsed.start();
        if (e->modifiers() & Qt::ShiftModifier)
        {
            if (!measure)
                bstart = overlay->pos;
            measure = true;
        }

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
        if (!config.smoothMotion)
        {   // i.e. without inertia
            velx = velz = 0;
        }

        if (!(e->modifiers() & Qt::ShiftModifier))
            measure = false;
        holding = false;
        mprev = e->pos();

        if (world && e->pos() == mstart)
        {
            world->selx = mstart.x();
            world->selz = mstart.y();
            world->seldo = true;
            world->selopt = D_NONE;
        }
    }
}

void MapView::keyReleaseEvent(QKeyEvent *e)
{
    if (e->matches(QKeySequence::Copy))
        copySeed();
    QWidget::keyReleaseEvent(e);
}

