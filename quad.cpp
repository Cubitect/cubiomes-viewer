#include "quad.h"

#include "cutil.h"

#include <QThreadPool>

#include <cmath>
#include <algorithm>


Quad::Quad(const Level* l, int i, int j)
    : mc(l->mc),entry(l->entry),seed(l->seed)
    , ti(i),tj(j),blocks(l->blocks),pixs(l->pixs),stype(l->stype)
    , rgb(),img(),spos()
    , done()
    , prio(),stopped()
{
    setAutoDelete(false);
}

Quad::~Quad()
{
    delete [] rgb;
    delete img;
    delete spos;
}

void getStructs(std::vector<VarPos> *out, const StructureConfig sconf,
    LayerStack *g, int mc, int64_t seed, int x0, int z0, int x1, int z1)
{
    int si0 = (int)floor(x0 / (qreal)(sconf.regionSize * 16));
    int sj0 = (int)floor(z0 / (qreal)(sconf.regionSize * 16));
    int si1 = (int)floor((x1-1) / (qreal)(sconf.regionSize * 16));
    int sj1 = (int)floor((z1-1) / (qreal)(sconf.regionSize * 16));

    for (int i = si0; i <= si1; i++)
    {
        for (int j = sj0; j <= sj1; j++)
        {
            Pos p;
            if (!getStructurePos(sconf.structType, mc, seed, i, j, &p))
                continue;

            if (p.x >= x0 && p.x < x1 && p.z >= z0 && p.z < z1)
            {
                int id = isViableStructurePos(sconf.structType, mc, g, seed, p.x, p.z);
                if (id)
                {
                    VarPos vp = { p, 0 };
                    if (sconf.structType == Village)
                    {
                        VillageType vt = getVillageType(mc, seed, p.x, p.z, id);
                        vp.variant = vt.abandoned;
                    }
                    out->push_back(vp);
                }
            }
        }
    }
}


void Quad::run()
{
    if (done)
        return;

    if (pixs > 0)
    {
        int *b = allocCache(entry, pixs, pixs);

        genArea(entry, b, ti*pixs, tj*pixs, pixs, pixs);

        rgb = new uchar[pixs*pixs * 3];
        biomesToImage(rgb, biomeColors, b, pixs, pixs, 1, 1);
        img = new QImage(rgb, pixs, pixs, QImage::Format_RGB888);
        free(b);
    }
    else
    {
        int structureType = -1;

        switch (stype)
        {
        case D_DESERT:      structureType = Desert_Pyramid; break;
        case D_JUNGLE:      structureType = Jungle_Pyramid; break;
        case D_IGLOO:       structureType = Igloo; break;
        case D_HUT:         structureType = Swamp_Hut; break;
        case D_VILLAGE:     structureType = Village; break;
        case D_MANSION:     structureType = Mansion; break;
        case D_MONUMENT:    structureType = Monument; break;
        case D_RUINS:       structureType = Ocean_Ruin; break;
        case D_SHIPWRECK:   structureType = Shipwreck; break;
        case D_TREASURE:    structureType = Treasure; break;
        case D_OUTPOST:     structureType = Outpost; break;
        case D_PORTAL:      structureType = Ruined_Portal; break;
        }
        if (structureType >= 0)
        {
            LayerStack g;
            setupGenerator(&g, mc);
            int x0 = ti*blocks, x1 = (ti+1)*blocks;
            int z0 = tj*blocks, z1 = (tj+1)*blocks;

            std::vector<VarPos>* st = new std::vector<VarPos>();
            StructureConfig sconf;
            if (getConfig(structureType, mc, &sconf))
                getStructs(st, sconf, &g, mc, seed, x0, z0, x1, z1);
            spos = st;
        }
    }
    done = true;
}


Level::Level()
    : cells(),g(),entry(),seed(),mc()
    , tx(),tz(),tw(),th()
    , scale(),blocks(),pixs()
    , stype()
{
}

Level::~Level()
{
    QThreadPool::globalInstance()->waitForDone();
    for (Quad *q : cells)
        delete q;
}

int mapOceanMixMod(const Layer * l, int * out, int x, int z, int w, int h)
{
    int *otyp;
    int i, j;
    l->p2->getMap(l->p2, out, x, z, w, h);

    otyp = (int *) malloc(w*h*sizeof(int));
    memcpy(otyp, out, w*h*sizeof(int));

    l->p->getMap(l->p, out, x, z, w, h);


    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            int landID, oceanID;

            landID = out[i + j*w];

            if (!isOceanic(landID))
                continue;

            oceanID = otyp[i + j*w];

            if (landID == deep_ocean)
            {
                switch (oceanID)
                {
                case lukewarm_ocean:
                    oceanID = deep_lukewarm_ocean;
                    break;
                case ocean:
                    oceanID = deep_ocean;
                    break;
                case cold_ocean:
                    oceanID = deep_cold_ocean;
                    break;
                case frozen_ocean:
                    oceanID = deep_frozen_ocean;
                    break;
                }
            }

            out[i + j*w] = oceanID;
        }
    }

    free(otyp);

    return 0;
}

void Level::init4map(int mcversion, int64_t ws, int pix, int layerscale)
{
    mc = mcversion;
    seed = ws;
    setupGenerator(&g, mc);

    tx = tz = tw = th = 0;
    scale = layerscale;
    pixs = pix;
    blocks = pix * layerscale;
    stype = D_NONE;

    switch (scale)
    {
    case 1:
        entry = g.entry_1;
        break;
    case 4:
        entry = g.entry_4;
        break;
    case 16:
        if (mc >= MC_1_13) {
            entry = setupLayer(&g, L_VORONOI_1, &mapOceanMixMod, mc, 1, 0, 0, &g.layers[L_SHORE_16], &g.layers[L_ZOOM_16_OCEAN]);
        } else {
            entry = g.entry_16;
        }
        break;
    case 64:
        if (mc >= MC_1_13) {
            entry = setupLayer(&g, L_VORONOI_1, &mapOceanMixMod, mc, 1, 0, 0, &g.layers[L_SUNFLOWER_64], &g.layers[L_ZOOM_64_OCEAN]);
        } else {
            entry = g.entry_64;
        }
        break;
    case 256:
        if (mc >= MC_1_13) {
            int layerid = mc >= MC_1_14 ? L_BAMBOO_256 : L_BIOME_256;
            entry = setupLayer(&g, L_VORONOI_1, &mapOceanMixMod, mc, 1, 0, 0, &g.layers[layerid], &g.layers[L_OCEAN_TEMP_256]);
        } else {
            entry = g.entry_256;
        }
        break;
    default:
        printf("Bad scale (%d) for level\n", scale);
        exit(1);
    }

    setLayerSeed(entry, seed);
}

void Level::init4struct(int mcversion, int64_t ws, int b, int structtype, int lv)
{
    mc = mcversion;
    seed = ws;
    blocks = b;
    pixs = -1;
    scale = -1;
    stype = structtype;
    viewlv = lv;
}

static int sqdist(int x, int z) { return x*x + z*z; }

void Level::resizeLevel(std::vector<Quad*>& cache, int x, int z, int w, int h)
{
    // move the cells from the old grid to the new grid
    // or to the cached queue if they are not inside the new grid
    std::vector<Quad*> grid(w*h);
    std::vector<Quad*> togen;

    for (Quad *q : cells)
    {
        int gx = q->ti - x;
        int gz = q->tj - z;
        if (gx >= 0 && gx < w && gz >= 0 && gz < h)
            grid[gz*w + gx] = q;
        else
            cache.push_back(q);
    }

    // look through the cached queue for reusable quads
    std::vector<Quad*> newcache;
    for (Quad *c : cache)
    {
        int gx = c->ti - x;
        int gz = c->tj - z;

        if (c->blocks == blocks && c->stype == stype)
        {
            // remove outside quads from schedule
            if (QThreadPool::globalInstance()->tryTake(c))
            {
                c->stopped = true;
            }

            if (gx >= 0 && gx < w && gz >= 0 && gz < h)
            {
                Quad *& g = grid[gz*w + gx];
                if (g == NULL)
                {
                    g = c;
                    continue;
                }
            }
        }
        newcache.push_back(c);
    }
    cache.swap(newcache);

    // collect which quads need generation and add any that are missing
    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            Quad *& g = grid[j*w + i];
            if (g == NULL)
            {
                g = new Quad(this, x+i, z+j);
                g->prio = sqdist(i-w/2, j-h/2);
                togen.push_back(g);
            }
            else if (g->stopped || QThreadPool::globalInstance()->tryTake(g))
            {
                if (!g->done)
                {
                    g->stopped = false;
                    g->prio = sqdist(i-w/2, j-h/2);
                    togen.push_back(g);
                }
            }
        }
    }

    // start the quad processing
    std::sort(togen.begin(), togen.end(),
              [](Quad* a, Quad* b) { return a->prio < b->prio; });
    for (Quad *q : togen)
        QThreadPool::globalInstance()->start(q, scale);

    cells.swap(grid);
    tx = x;
    tz = z;
    tw = w;
    th = h;
}

void Level::update(std::vector<Quad*>& cache, qreal bx0, qreal bz0, qreal bx1, qreal bz1)
{
    int nti0 = (int) std::floor(bx0 / blocks);
    int ntj0 = (int) std::floor(bz0 / blocks);
    int nti1 = (int) std::floor(bx1 / blocks) + 1;
    int ntj1 = (int) std::floor(bz1 / blocks) + 1;

    // resize if the new area is much smaller or in an unprocessed range
    if ((nti1-nti0)*2 < tw || nti0 < tx || nti1 > tx+tw || ntj0 < tz || ntj1 > tz+th)
    {
        qreal padf = 0.2 * (bx1 - bx0);
        nti0 = (int) std::floor((bx0-padf) / blocks);
        ntj0 = (int) std::floor((bz0-padf) / blocks);
        nti1 = (int) std::floor((bx1+padf) / blocks) + 1;
        ntj1 = (int) std::floor((bz1+padf) / blocks) + 1;

        resizeLevel(cache, nti0, ntj0, nti1-nti0, ntj1-ntj0);
    }
}


QWorld::QWorld(int mc, int64_t seed)
    : mc(mc)
    , seed(seed)
    , lv()
    , lvs()
    , activelv()
    , cached()
    , cachedstruct()
    , cachesize()
    , spawn()
    , strongholds()
    , isdel()
    , slimeimg()
    , slimex()
    , slimez()
    , seldo()
    , selx()
    , selz()
    , seltype(-1)
    , selpos()
    , selvar()
    , qual()
{
    setupGenerator(&g, mc);
    applySeed(&g, seed);

    activelv = 0;

    int pixs = 512;
    lvs.resize(D_SPAWN);
    lvs[D_DESERT].init4struct(mc, seed, 2048, D_DESERT, 2);
    lvs[D_JUNGLE].init4struct(mc, seed, 2048, D_JUNGLE, 2);
    lvs[D_IGLOO].init4struct(mc, seed, 2048, D_IGLOO, 2);
    lvs[D_HUT].init4struct(mc, seed, 2048, D_HUT, 2);
    lvs[D_VILLAGE].init4struct(mc, seed, 2048, D_VILLAGE, 2);
    lvs[D_MANSION].init4struct(mc, seed, 2048, D_MANSION, 3);
    lvs[D_MONUMENT].init4struct(mc, seed, 2048, D_MONUMENT, 2);
    lvs[D_RUINS].init4struct(mc, seed, 2048, D_RUINS, 1);
    lvs[D_SHIPWRECK].init4struct(mc, seed, 2048, D_SHIPWRECK, 1);
    lvs[D_TREASURE].init4struct(mc, seed, 2048, D_TREASURE, 1);
    lvs[D_OUTPOST].init4struct(mc, seed, 2048, D_OUTPOST, 2);
    lvs[D_PORTAL].init4struct(mc, seed, 2048, D_PORTAL, 1);
    lv.resize(5);
    lv[0].init4map(mc, seed, pixs, 1);
    lv[1].init4map(mc, seed, pixs, 4);
    lv[2].init4map(mc, seed, pixs, 16);
    lv[3].init4map(mc, seed, pixs, 64);
    lv[4].init4map(mc, seed, pixs, 256);
    cachesize = 100;
    qual = 1.0;

    memset(sshow, 0, sizeof(sshow));

    icons[D_DESERT]     = QPixmap(":/icons/desert.png");
    icons[D_JUNGLE]     = QPixmap(":/icons/jungle.png");
    icons[D_IGLOO]      = QPixmap(":/icons/igloo.png");
    icons[D_HUT]        = QPixmap(":/icons/hut.png");
    icons[D_VILLAGE]    = QPixmap(":/icons/village.png");
    icons[D_MANSION]    = QPixmap(":/icons/mansion.png");
    icons[D_MONUMENT]   = QPixmap(":/icons/monument.png");
    icons[D_RUINS]      = QPixmap(":/icons/ruins.png");
    icons[D_SHIPWRECK]  = QPixmap(":/icons/shipwreck.png");
    icons[D_TREASURE]   = QPixmap(":/icons/treasure.png");
    icons[D_OUTPOST]    = QPixmap(":/icons/outpost.png");
    icons[D_PORTAL]     = QPixmap(":/icons/portal.png");
    icons[D_SPAWN]      = QPixmap(":/icons/spawn.png");
    icons[D_STRONGHOLD] = QPixmap(":/icons/stronghold.png");

    iconzvil = QPixmap(":/icons/zombie.png");
}

QWorld::~QWorld()
{
    isdel = true;
    QThreadPool::globalInstance()->clear();
    QThreadPool::globalInstance()->waitForDone();
    for (Quad *q : cached)
        delete q;
    for (Quad *q : cachedstruct)
        delete q;
    if (spawn && spawn != (Pos*)-1)
    {
        delete spawn;
        delete strongholds;
    }
}

void QWorld::cleancache(std::vector<Quad*>& cache, unsigned int maxsize)
{
    // try to delete the oldest entries in the cache
    if (cache.size() > maxsize)
    {
        std::vector<Quad*> newcache;
        int i;
        for (i = cache.size()-1; i >= 0; --i)
        {
            Quad *q = cache[i];
            if (newcache.size() + i < maxsize * 0.8)
            {
                newcache.push_back(q);
            }
            else
            {
                if (q->done || q->stopped || QThreadPool::globalInstance()->tryTake(q))
                    delete q;
                else
                    newcache.push_back(q);
            }
        }

        cache.resize(newcache.size());
        std::copy(newcache.rbegin(), newcache.rend(), cache.begin());
    }
}


struct SpawnStronghold : public QRunnable
{
    QWorld *world;
    int mc;
    int64_t seed;

    SpawnStronghold(QWorld *world, int mc, int64_t seed) :
        world(world),mc(mc),seed(seed) {}

    void run()
    {
        LayerStack g;
        setupGenerator(&g, mc);
        applySeed(&g, seed);

        Pos *p = new Pos;
        *p = getSpawn(mc, &g, NULL, seed);
        world->spawn = p;
        if (world->isdel) return;

        StrongholdIter sh;
        initFirstStronghold(&sh, mc, seed);

        std::vector<Pos> *shp = new std::vector<Pos>;
        shp->reserve(mc >= MC_1_9 ? 128 : 3);

        while (nextStronghold(&sh, &g, NULL) > 0)
        {
            if (world->isdel)
            {
                delete shp;
                return;
            }
            shp->push_back(sh.pos);
        }

        world->strongholds = shp;
    }
};


void QWorld::draw(QPainter& painter, int vw, int vh, qreal focusx, qreal focusz, qreal blocks2pix)
{
    qreal uiw = vw / blocks2pix;
    qreal uih = vh / blocks2pix;

    qreal bx0 = focusx - uiw/2;
    qreal bz0 = focusz - uih/2;
    qreal bx1 = focusx + uiw/2;
    qreal bz1 = focusz + uih/2;

    if      (blocks2pix >= qual) activelv = -1;
    else if (blocks2pix >= qual/4) activelv = 0;
    else if (blocks2pix >= qual/16) activelv = 1;
    else if (blocks2pix >= qual/64) activelv = 2;
    else if (blocks2pix >= qual/256) activelv = 3;
    else activelv = lv.size()-1;

    for (int li = activelv+1; li >= activelv; --li)
    {
        if (li < 0 || li >= (int)lv.size())
            continue;
        Level& l = lv[li];
        for (Quad *q : l.cells)
        {
            if (!q->img)
                continue;
            // q was processed in another thread and is now done
            qreal ps = q->blocks * blocks2pix;
            qreal px = vw/2 + (q->ti) * ps - focusx * blocks2pix;
            qreal pz = vh/2 + (q->tj) * ps - focusz * blocks2pix;

            QRect rec(px,pz,ps,ps);
            painter.drawImage(rec, *q->img);

            if (sshow[D_GRID])
            {
                QString s = QString::asprintf("%d,%d", q->ti*q->blocks, q->tj*q->blocks);
                QRect textrec = painter.fontMetrics()
                        .boundingRect(rec, Qt::AlignLeft | Qt::AlignTop, s);

                painter.fillRect(textrec, QBrush(QColor(0, 0, 0, 128), Qt::SolidPattern));

                painter.setPen(QColor(255, 255, 255));
                painter.drawText(textrec, s);

                painter.setPen(QPen(QColor(0, 0, 0, 96), 1));
                painter.drawRect(rec);
            }
        }
    }

    if (sshow[D_SLIME] && blocks2pix*16 > 2.0)
    {
        int x = floor(bx0 / 16), w = floor(bx1 / 16) - x + 1;
        int z = floor(bz0 / 16), h = floor(bz1 / 16) - z + 1;

        // conditions when the slime overlay should be updated
        if (x < slimex || z < slimez ||
            x+w >= slimex+slimeimg.width() || z+h >= slimez+slimeimg.height() ||
            w*h*4 >= slimeimg.width()*slimeimg.height())
        {
            int pad = 64;
            x -= pad;
            z -= pad;
            w += 2*pad;
            h += 2*pad;
            slimeimg = QImage(w, h, QImage::Format_Indexed8);
            slimeimg.setColor(0, qRgba(0, 0, 0, 64));
            slimeimg.setColor(1, qRgba(0, 255, 0, 64));
            slimex = x;
            slimez = z;

            for (int j = 0; j < h; j++)
            {
                for (int i = 0; i < w; i++)
                {
                    int isslime = isSlimeChunk(seed, i+x, j+z);
                    slimeimg.setPixel(i, j, isslime);
                }
            }
        }

        qreal ps = 16 * blocks2pix;
        qreal px = vw/2 + slimex * ps - focusx * blocks2pix;
        qreal pz = vh/2 + slimez * ps - focusz * blocks2pix;

        QRect rec(px, pz, ps*slimeimg.width(), ps*slimeimg.height());
        // match partial pixels to X11
        //painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        painter.drawImage(rec, slimeimg);
    }


    for (int stype = D_DESERT; stype < D_SPAWN; stype++)
    {
        Level& l = lvs[stype];
        if (!sshow[stype] || activelv > l.viewlv)
            continue;

        std::vector<QPainter::PixmapFragment> frags;

        for (Quad *q : l.cells)
        {
            if (!q->spos)
                continue;
            // q was processed in another thread and is now done
            for (VarPos& vp : *q->spos)
            {
                qreal x = vw/2 + (vp.p.x - focusx) * blocks2pix;
                qreal y = vh/2 + (vp.p.z - focusz) * blocks2pix;
                if (x < 0 || x >= vw || y < 0 || y >= vh)
                    continue;

                QPointF d = QPointF(x, y);
                QRectF r = icons[stype].rect();
                if (stype == D_VILLAGE)
                {
                    if (vp.variant) {
                        painter.drawPixmap(x-r.width()/2, y-r.height()/2, iconzvil);
                    } else {
                        frags.push_back(QPainter::PixmapFragment::create(d, r));
                    }
                }
                else
                {
                    frags.push_back(QPainter::PixmapFragment::create(d, r));
                }

                if (seldo)
                {
                    r.moveCenter(d);
                    if (r.contains(selx, selz))
                    {
                        seltype = stype;
                        selpos = vp.p;
                        selvar = vp.variant;
                    }
                }
            }
        }

        painter.drawPixmapFragments(frags.data(), frags.size(), icons[stype]);
    }

    Pos* sp = spawn; // atomic fetch
    if (sp && sp != (Pos*)-1 && sshow[D_SPAWN])
    {
        qreal x = vw/2 + (sp->x - focusx) * blocks2pix;
        qreal y = vh/2 + (sp->z - focusz) * blocks2pix;

        QPointF d = QPointF(x, y);
        QRectF r = icons[D_SPAWN].rect();
        painter.drawPixmap(d, icons[D_SPAWN]);

        if (seldo)
        {
            r.moveCenter(d);
            if (r.contains(selx, selz))
            {
                seltype = D_SPAWN;
                selpos = *sp;
                selvar = 0;
            }
        }
    }

    std::vector<Pos>* shs = strongholds; // atomic fetch
    if (shs && sshow[D_STRONGHOLD])
    {
        std::vector<QPainter::PixmapFragment> frags;
        frags.reserve(shs->size());
        for (Pos p : *shs)
        {
            qreal x = vw/2 + (p.x - focusx) * blocks2pix;
            qreal y = vh/2 + (p.z - focusz) * blocks2pix;
            QPointF d = QPointF(x, y);
            QRectF r = icons[D_STRONGHOLD].rect();
            frags.push_back(QPainter::PixmapFragment::create(d, r));

            if (seldo)
            {
                r.moveCenter(d);
                if (r.contains(selx, selz))
                {
                    seltype = D_STRONGHOLD;
                    selpos = p;
                    selvar = 0;
                }
            }
        }
        painter.drawPixmapFragments(frags.data(), frags.size(), icons[D_STRONGHOLD]);
    }

    for (int li = lv.size()-1; li >= 0; --li)
    {
        if (li == activelv || li == activelv+1)
            lv[li].update(cached, bx0, bz0, bx1, bz1);
        else
            lv[li].update(cached, 0, 0, 0, 0);
    }
    for (int stype = D_DESERT; stype < D_SPAWN; stype++)
    {
        Level& l = lvs[stype];
        if (activelv <= l.viewlv && sshow[stype])
            l.update(cachedstruct, bx0, bz0, bx1, bz1);
        else if (activelv > l.viewlv+1)
            l.update(cachedstruct, 0, 0, 0, 0);
    }

    // start the spawn and stronghold worker thread if this is the first run
    if (spawn == NULL && (sshow[D_SPAWN] || sshow[D_STRONGHOLD]))
    {
        spawn = (Pos*) -1;
        QThreadPool::globalInstance()->start(new SpawnStronghold(this, mc, seed));
    }

    if (seldo)
    {
        seldo = false;
    }

    if (seltype != D_NONE)
    {
        QPixmap *icon = &icons[seltype];
        if (selvar)
        {
            if (seltype == D_VILLAGE)
                icon = &iconzvil;
        }
        qreal x = vw/2 + (selpos.x - focusx) * blocks2pix;
        qreal y = vh/2 + (selpos.z - focusz) * blocks2pix;
        QRect iconrec = icon->rect();
        qreal w = iconrec.width() * 1.5;
        qreal h = iconrec.height() * 1.5;
        painter.drawPixmap(x-w/2, y-h/2, w, h, *icon);

        QFont f = QFont();
        f.setBold(true);
        painter.setFont(f);

        QString s = QString::asprintf(" %d,%d", selpos.x, selpos.z);
        int pad = 5;
        QRect textrec = painter.fontMetrics()
                .boundingRect(0, 0, vw, vh, Qt::AlignLeft | Qt::AlignTop, s);

        if (textrec.height() < iconrec.height())
            textrec.setHeight(iconrec.height());

        textrec.translate(pad+iconrec.width(), pad);

        painter.fillRect(textrec.marginsAdded(QMargins(pad+iconrec.width(),pad,pad,pad)),
                         QBrush(QColor(0, 0, 0, 128), Qt::SolidPattern));

        painter.setPen(QPen(QColor(255, 255, 255), 2));
        painter.drawText(textrec, s, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

        painter.drawPixmap(iconrec.translated(pad,pad), *icon);
    }

    cleancache(cached, cachesize);
    cleancache(cachedstruct, cachesize);
}


