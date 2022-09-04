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
    , updatecounter()
    , layeropt(LOPT_DEFAULT_1)
    , config()
{
    memset(sshow, 0, sizeof(sshow));

    QFont mono = QFont("Monospace", 9);
    mono.setStyleHint(QFont::TypeWriter);
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
        world = new QWorld(wi, dim, layeropt);
    }
}

void MapView::setSeed(WorldInfo wi, int dim, int lopt)
{
    prevx = focusx = getX();
    prevz = focusz = getZ();
    velx = velz = 0;
    if (lopt >= 0)
        layeropt = lopt;

    if (world == NULL || !wi.equals(world->wi))
    {
        delete world;
        world = new QWorld(wi, dim, layeropt);
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
    world->gridspacing = config.gridSpacing;
    world->memlimit = (uint64_t) config.mapCacheSize * 1024 * 1024;
    world->layeropt = layeropt;
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
        p = world->selvp.p;
    return p;
}

void MapView::showContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    // this is a contextual temporary menu so shortcuts are only indicated here,
    // but will not function - see keyReleaseEvent() for shortcut implementation

    menu.addAction(tr("Copy seed"), this, &MapView::copySeed, QKeySequence::Copy);
    menu.addAction(tr("Copy coordinates"), this, &MapView::copyCoord);
    menu.addAction(tr("Copy teleport command"), this, &MapView::copyTeleportCommand);
    menu.addAction(tr("Go to coordinates..."), this, &MapView::onGoto);
    menu.exec(mapToGlobal(pos));
}

void MapView::copySeed()
{
    if (world)
    {
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(QString::asprintf("%" PRId64, (int64_t)world->wi.seed));
    }
}

void MapView::copyCoord()
{
    Pos p = getActivePos();
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(QString::asprintf("%d, %d", p.x, p.z));
}

void MapView::copyTeleportCommand()
{
    Pos p = getActivePos();
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(QString::asprintf("/tp @p %d ~ %d", p.x, p.z));
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

    if (world)
    {
        world->draw(painter, width(), height(), fx, fz, blocks2pix);

        QPoint cur = mapFromGlobal(QCursor::pos());
        qreal bx = (cur.x() -  width()/2.0) / blocks2pix + fx;
        qreal bz = (cur.y() - height()/2.0) / blocks2pix + fz;
        Pos p = {(int)bx, (int)bz};
        overlay->pos = p;
        overlay->bname = world->getBiomeName(p);

        bool active = QThreadPool::globalInstance()->activeThreadCount() > 0;
        if (active || velx || velz)
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
        mtime = 0;
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
        if (!config.smoothMotion)
        {   // i.e. without inertia
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

void MapView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Copy))
        copySeed();
    QWidget::keyReleaseEvent(event);
}

