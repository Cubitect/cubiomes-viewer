#include "mapview.h"

#include <QPainter>
#include <QTimer>
#include <QThread>
#include <QApplication>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QThreadPool>
#include <QTime>

#include <math.h>


static const char *biome2str(int id)
{
    switch (id)
    {
    case ocean: return "ocean";
    case plains: return "plains";
    case desert: return "desert";
    case mountains: return "mountains";
    case forest: return "forest";
    case taiga: return "taiga";
    case swamp: return "swamp";
    case river: return "river";
    case nether_wastes: return "nether_wastes";
    case the_end: return "the_end";
    // 10
    case frozen_ocean: return "frozen_ocean";
    case frozen_river: return "frozen_river";
    case snowy_tundra: return "snowy_tundra";
    case snowy_mountains: return "snowy_mountains";
    case mushroom_fields: return "mushroom_fields";
    case mushroom_field_shore: return "mushroom_field_shore";
    case beach: return "beach";
    case desert_hills: return "desert_hills";
    case wooded_hills: return "wooded_hills";
    case taiga_hills: return "taiga_hills";
    // 20
    case mountain_edge: return "mountain_edge";
    case jungle: return "jungle";
    case jungle_hills: return "jungle_hills";
    case jungle_edge: return "jungle_edge";
    case deep_ocean: return "deep_ocean";
    case stone_shore: return "stone_shore";
    case snowy_beach: return "snowy_beach";
    case birch_forest: return "birch_forest";
    case birch_forest_hills: return "birch_forest_hills";
    case dark_forest: return "dark_forest";
    // 30
    case snowy_taiga: return "snowy_taiga";
    case snowy_taiga_hills: return "snowy_taiga_hills";
    case giant_tree_taiga: return "giant_tree_taiga";
    case giant_tree_taiga_hills: return "giant_tree_taiga_hills";
    case wooded_mountains: return "wooded_mountains";
    case savanna: return "savanna";
    case savanna_plateau: return "savanna_plateau";
    case badlands: return "badlands";
    case wooded_badlands_plateau: return "wooded_badlands_plateau";
    case badlands_plateau: return "badlands_plateau";
    // 40  --  1.13
    case small_end_islands: return "small_end_islands";
    case end_midlands: return "end_midlands";
    case end_highlands: return "end_highlands";
    case end_barrens: return "end_barrens";
    case warm_ocean: return "warm_ocean";
    case lukewarm_ocean: return "lukewarm_ocean";
    case cold_ocean: return "cold_ocean";
    case deep_warm_ocean: return "deep_warm_ocean";
    case deep_lukewarm_ocean: return "deep_lukewarm_ocean";
    case deep_cold_ocean: return "deep_cold_ocean";
    // 50
    case deep_frozen_ocean: return "deep_frozen_ocean";

    case the_void: return "the_void";

    // mutated variants
    case sunflower_plains: return "sunflower_plains";
    case desert_lakes: return "desert_lakes";
    case gravelly_mountains: return "gravelly_mountains";
    case flower_forest: return "flower_forest";
    case taiga_mountains: return "taiga_mountains";
    case swamp_hills: return "swamp_hills";
    case ice_spikes: return "ice_spikes";
    case modified_jungle: return "modified_jungle";
    case modified_jungle_edge: return "modified_jungle_edge";
    case tall_birch_forest: return "tall_birch_forest";
    case tall_birch_hills: return "tall_birch_hills";
    case dark_forest_hills: return "dark_forest_hills";
    case snowy_taiga_mountains: return "snowy_taiga_mountains";
    case giant_spruce_taiga: return "giant_spruce_taiga";
    case giant_spruce_taiga_hills: return "giant_spruce_taiga_hills";
    case modified_gravelly_mountains: return "modified_gravelly_mountains";
    case shattered_savanna: return "shattered_savanna";
    case shattered_savanna_plateau: return "shattered_savanna_plateau";
    case eroded_badlands: return "eroded_badlands";
    case modified_wooded_badlands_plateau: return "modified_wooded_badlands_plateau";
    case modified_badlands_plateau: return "modified_badlands_plateau";
    // 1.14
    case bamboo_jungle: return "bamboo_jungle";
    case bamboo_jungle_hills: return "bamboo_jungle_hills";
    // 1.16
    case soul_sand_valley: return "soul_sand_valley";
    case crimson_forest: return "crimson_forest";
    case warped_forest: return "warped_forest";
    case basalt_deltas: return "basalt_deltas";
    }
    return NULL;
}

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
