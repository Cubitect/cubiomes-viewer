#include "world.h"
#include "mapview.h" // for translations

#include "cutil.h"

#include <QThreadPool>
#include <QSettings>

#include <cmath>
#include <algorithm>


void loadStructVis(std::map<int, double>& structvis)
{
    QSettings settings("cubiomes-viewer", "cubiomes-viewer");

    for (int opt = D_DESERT; opt < D_SPAWN; opt++)
    {
        double defval = 32;
        const char *name = mapopt2str(opt);
        double scale = settings.value(QString("structscale/") + name, defval).toDouble();
        structvis[opt] = scale;
    }
}

void saveStructVis(std::map<int, double>& structvis)
{
    QSettings settings("cubiomes-viewer", "cubiomes-viewer");

    for (auto it : structvis)
    {
        const char *name = mapopt2str(it.first);
        settings.setValue(QString("structscale/") + name, it.second);
    }
}

const QPixmap& getMapIcon(int opt, VarPos *vp)
{
    static QPixmap icons[STRUCT_NUM];
    static QPixmap iconzvil;
    static QPixmap icongiant;
    static QPixmap iconship;
    static bool init = false;

    if (!init)
    {
        init = true;
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
        icons[D_MINESHAFT]  = QPixmap(":/icons/mineshaft.png");
        icons[D_OUTPOST]    = QPixmap(":/icons/outpost.png");
        icons[D_ANCIENTCITY]= QPixmap(":/icons/ancient_city.png");
        icons[D_PORTAL]     = QPixmap(":/icons/portal.png");
        icons[D_PORTALN]    = QPixmap(":/icons/portal.png");
        icons[D_SPAWN]      = QPixmap(":/icons/spawn.png");
        icons[D_STRONGHOLD] = QPixmap(":/icons/stronghold.png");
        icons[D_FORTESS]    = QPixmap(":/icons/fortress.png");
        icons[D_BASTION]    = QPixmap(":/icons/bastion.png");
        icons[D_ENDCITY]    = QPixmap(":/icons/endcity.png");
        icons[D_GATEWAY]    = QPixmap(":/icons/gateway.png");
        iconzvil            = QPixmap(":/icons/zombie.png");
        icongiant           = QPixmap(":/icons/portal_giant.png");
        iconship            = QPixmap(":/icons/end_ship.png");
    }
    if (!vp)
        return icons[opt];
    if (opt == D_VILLAGE && vp->v.abandoned)
        return iconzvil;
    if ((opt == D_PORTAL || opt == D_PORTALN) && vp->v.giant)
        return icongiant;
    if (opt == D_ENDCITY)
    {
        for (Piece& p : vp->pieces)
            if (p.type == END_SHIP)
                return iconship;
    }
    return icons[opt];
}

QStringList VarPos::detail() const
{
    QStringList sinfo;
    QString s;
    if (type == Village)
    {
        if (v.abandoned)
            sinfo.append("abandoned");
        s = getStartPieceName(Village, &v);
        if (!s.isEmpty())
            sinfo.append(s);
    }
    else if (type == Bastion)
    {
        s = getStartPieceName(Bastion, &v);
        if (!s.isEmpty())
            sinfo.append(s);
    }
    else if (type == Ruined_Portal || type == Ruined_Portal_N)
    {
        switch (v.biome)
        {
        case plains:    sinfo.append("standard"); break;
        case desert:    sinfo.append("desert"); break;
        case jungle:    sinfo.append("jungle"); break;
        case swamp:     sinfo.append("swamp"); break;
        case mountains: sinfo.append("mountain"); break;
        case ocean:     sinfo.append("ocean"); break;
        default:        sinfo.append("nether"); break;
        }
        s = getStartPieceName(Ruined_Portal, &v);
        if (!s.isEmpty())
            sinfo.append(s);
        if (v.underground)
            sinfo.append("underground");
        if (v.airpocket)
            sinfo.append("airpocket");
    }
    else if (type == End_City)
    {
        sinfo.append(QString::asprintf("size=%zu", pieces.size()));
        for (const Piece& p : pieces)
        {
            if (p.type == END_SHIP)
            {
                sinfo.append("ship");
                break;
            }
        }
    }
    else if (type == Fortress)
    {
        sinfo.append(QString::asprintf("size=%zu", pieces.size()));
        int spawner = 0, wart = 0;
        for (const Piece& p : pieces)
        {
            spawner += p.type == BRIDGE_SPAWNER;
            wart += p.type == CORRIDOR_NETHER_WART;
        }
        if (spawner)
            sinfo.append(QString::asprintf("spawners=%d", spawner));
        if (wart)
            sinfo.append(QString::asprintf("nether_wart=%d", wart));
    }
    return sinfo;
}


Quad::Quad(const Level* l, int i, int j)
    : wi(l->wi),dim(l->dim),g(&l->g),scale(l->scale)
    , ti(i),tj(j),blocks(l->blocks),pixs(l->pixs),sopt(l->sopt),lopt(l->lopt)
    , biomes(),rgb(),img(),spos()
    , done(),isdel(l->isdel)
    , prio(),stopped()
{
    setAutoDelete(false);
}

Quad::~Quad()
{
    if (biomes) free(biomes);
    delete img;
    delete spos;
    delete [] rgb;
}

void getStructs(std::vector<VarPos> *out, const StructureConfig sconf,
        WorldInfo wi, int dim, int x0, int z0, int x1, int z1)
{
    int si0 = (int)floor(x0 / (qreal)(sconf.regionSize * 16));
    int sj0 = (int)floor(z0 / (qreal)(sconf.regionSize * 16));
    int si1 = (int)floor((x1-1) / (qreal)(sconf.regionSize * 16));
    int sj1 = (int)floor((z1-1) / (qreal)(sconf.regionSize * 16));

    // TODO: move generator to arguments?
    //       isViableStructurePos would have to be const (due to threading)
    Generator g;
    setupGenerator(&g, wi.mc, wi.large);
    applySeed(&g, dim, wi.seed);

    for (int i = si0; i <= si1; i++)
    {
        for (int j = sj0; j <= sj1; j++)
        {
            Pos p;
            int ok = getStructurePos(sconf.structType, wi.mc, wi.seed, i, j, &p);
            if (!ok)
                continue;

            if (p.x >= x0 && p.x < x1 && p.z >= z0 && p.z < z1)
            {
                int id = isViableStructurePos(sconf.structType, &g, p.x, p.z, 0);
                if (!id)
                    continue;
                VarPos vp = VarPos(p, sconf.structType);
                Piece pieces[1024];

                if (sconf.structType == End_City)
                {
                    SurfaceNoise sn;
                    initSurfaceNoiseEnd(&sn, wi.seed);
                    id = isViableEndCityTerrain(&g.en, &sn, p.x, p.z);
                    if (!id)
                        continue;
                    int n = getEndCityPieces(pieces, wi.seed, p.x >> 4, p.z >> 4);
                    vp.pieces.assign(pieces, pieces+n);
                }
                else if (sconf.structType == Ruined_Portal || sconf.structType == Ruined_Portal_N)
                {
                    id = getBiomeAt(&g, 4, (p.x >> 2) + 2, 0, (p.z >> 2) + 2);
                }
                else if (sconf.structType == Fortress)
                {
                    int n = getFortressPieces(pieces, sizeof(pieces)/sizeof(pieces[0]),
                        wi.mc, wi.seed, p.x >> 4, p.z >> 4);
                    vp.pieces.assign(pieces, pieces+n);
                }
                else if (g.mc >= MC_1_18)
                {
                    if (g_extgen.estimateTerrain &&
                        !isViableStructureTerrain(sconf.structType, &g, p.x, p.z))
                    {
                        continue;
                    }
                }

                getVariant(&vp.v, sconf.structType, wi.mc, wi.seed, p.x, p.z, id);
                out->push_back(vp);
            }
        }
    }
}

static QMutex g_mutex;

void Quad::run()
{
    if (done || *isdel)
        return;

    if (pixs > 0)
    {
        int seam_buf = 0; //pixs / 128;
        int y = (scale > 1) ? wi.y >> 2 : wi.y;
        int x = ti*pixs, z = tj*pixs, w = pixs+seam_buf, h = pixs+seam_buf;
        Range r = {scale, x, z, w, h, y, 1};
        biomes = allocCache(g, r);
        if (!biomes) return;

        int err = genBiomes(g, biomes, r);
        if (err)
        {
            fprintf(
                stderr,
                "Failed to generate tile - "
                "MC:%s seed:%" PRId64 " dim:%d @ [%d %d] (%d %d) 1:%d\n",
                mc2str(g->mc), g->seed, g->dim,
                x, z, w, h, scale);
            for (int i = 0; i < w*h; i++)
                biomes[i] = -1;
        }

        rgb = new uchar[w*h * 3];
        if (lopt <= LOPT_OCEAN_256 || g->mc < MC_1_18 || dim != 0 || g->bn.nptype < 0)
        {
            // sync biomeColors
            g_mutex.lock();
            g_mutex.unlock();
            biomesToImage(rgb, g_biomeColors, biomes, w, h, 1, 1);
        }
        else // climate parameter
        {
            const int *extremes = getBiomeParaExtremes(g->mc);
            int cmin = extremes[g->bn.nptype*2 + 0];
            int cmax = extremes[g->bn.nptype*2 + 1];
            for (int i = 0; i < w*h; i++)
            {
                double p = (biomes[i] - cmin) / (double) (cmax - cmin);
                uchar col = (p <= 0) ? 0 : (p >= 1.0) ? 0xff : (uchar)(0xff * p);
                rgb[3*i+0] = rgb[3*i+1] = rgb[3*i+2] = col;
            }
        }
        img = new QImage(rgb, w, h, 3*w, QImage::Format_RGB888);
    }
    else if (pixs < 0)
    {
        int structureType = mapopt2stype(sopt);
        if (structureType >= 0)
        {
            int x0 = ti*blocks, x1 = (ti+1)*blocks;
            int z0 = tj*blocks, z1 = (tj+1)*blocks;
            std::vector<VarPos>* st = new std::vector<VarPos>();
            StructureConfig sconf;
            if (getStructureConfig_override(structureType, wi.mc, &sconf))
                getStructs(st, sconf, wi, dim, x0, z0, x1, z1);
            spos = st;
        }
    }
    done = true;
}


Level::Level()
    : cells(),g(),entry(),wi(),dim()
    , tx(),tz(),tw(),th()
    , scale(),blocks(),pixs()
    , sopt(),lopt()
{
}

Level::~Level()
{
    QThreadPool::globalInstance()->waitForDone();
    for (Quad *q : cells)
        delete q;
}


void Level::init4map(QWorld *w, int pix, int layerscale)
{
    this->wi = w->wi;
    this->dim = w->dim;

    tx = tz = tw = th = 0;

    scale = layerscale;
    pixs = pix;
    blocks = pix * layerscale;

    int optlscale = 1;
    switch (w->layeropt)
    {
    case LOPT_NOISE_T_4:
    case LOPT_NOISE_H_4:
    case LOPT_NOISE_C_4:
    case LOPT_NOISE_E_4:
    case LOPT_NOISE_D_4:
    case LOPT_NOISE_W_4:
    case LOPT_RIVER_4:
    case LOPT_DEFAULT_4:
        optlscale = 4;
        break;
    case LOPT_DEFAULT_16:
        optlscale = 16;
        break;
    case LOPT_DEFAULT_64:
        optlscale = 64;
        break;
    case LOPT_OCEAN_256:
    case LOPT_DEFAULT_256:
        optlscale = 256;
        break;
    }
    if (layerscale < optlscale)
    {
        int f = optlscale / layerscale;
        scale *= f;
        pixs /= f;
        if (pixs == 0)
            pixs = 1;
    }

    if (w->layeropt == LOPT_RIVER_4 && wi.mc >= MC_1_13 && wi.mc <= MC_1_17)
    {
        setupGenerator(&g, wi.mc, wi.large);
        g.ls.entry_4 = &g.ls.layers[L_RIVER_MIX_4];
    }
    else if (w->layeropt == LOPT_OCEAN_256 && wi.mc >= MC_1_13 && wi.mc <= MC_1_17)
    {
        setupGenerator(&g, wi.mc, wi.large);
        g.ls.entry_256 = &g.ls.layers[L_OCEAN_TEMP_256];
    }
    else
    {
        setupGenerator(&g, wi.mc, wi.large | FORCE_OCEAN_VARIANTS);
    }

    applySeed(&g, dim, wi.seed);
    this->isdel = &w->isdel;
    sopt = D_NONE;
    lopt = 0;
    if (dim == 0 && wi.mc >= MC_1_18)
    {
        lopt = w->layeropt;
        switch (w->layeropt)
        {
        case LOPT_NOISE_T_4: g.bn.nptype = NP_TEMPERATURE; break;
        case LOPT_NOISE_H_4: g.bn.nptype = NP_HUMIDITY; break;
        case LOPT_NOISE_C_4: g.bn.nptype = NP_CONTINENTALNESS; break;
        case LOPT_NOISE_E_4: g.bn.nptype = NP_EROSION; break;
        case LOPT_NOISE_D_4: g.bn.nptype = NP_DEPTH; break;
        case LOPT_NOISE_W_4: g.bn.nptype = NP_WEIRDNESS; break;
        }
    }
}

void Level::init4struct(QWorld *w, int dim, int blocks, double vis, int sopt)
{
    this->wi = w->wi;
    this->dim = dim;
    this->blocks = blocks;
    this->pixs = -1;
    this->scale = -1;
    this->sopt = sopt;
    this->lopt = 0;
    this->vis = vis;
    this->isdel = &w->isdel;
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

        if (c->blocks == blocks && c->sopt == sopt && c->dim == dim)
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
                    if (g->dim != dim)
                        g->prio += 1000000;
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


QWorld::QWorld(WorldInfo wi, int dim, int layeropt)
    : wi(wi)
    , dim(dim)
    , layeropt(layeropt)
    , lvb()
    , lvs()
    , activelv()
    , cachedbiomes()
    , cachedstruct()
    , memlimit()
    , showBB()
    , gridspacing()
    , spawn()
    , strongholds()
    , qsinfo()
    , isdel()
    , slimeimg()
    , slimex()
    , slimez()
    , seldo()
    , selx()
    , selz()
    , selopt(-1)
    , selvp(Pos{}, -1)
    , qual()
{
    setupGenerator(&g, wi.mc,  wi.large);

    memlimit = 256ULL * 1024*1024;

    setDim(dim, layeropt);

    std::map<int, double> svis;
    loadStructVis(svis);

    lvs.resize(D_SPAWN);
    for (int opt = D_DESERT; opt < D_SPAWN; opt++)
    {
        int sdim = 0, qsiz = 512*16;
        switch (opt) {
        case D_PORTALN: sdim = -1; break;
        case D_FORTESS: sdim = -1; break;
        case D_BASTION: sdim = -1; break;
        case D_ENDCITY: sdim = +1; break;
        case D_GATEWAY: sdim = +1; break;
        case D_MANSION: qsiz = 1280*16; break;
        }
        double vis = 1.0 / svis[opt];
        lvs[opt].init4struct(this, sdim, qsiz, vis, opt);
    }
    memset(sshow, 0, sizeof(sshow));
}

QWorld::~QWorld()
{
    clearPool();
    for (Quad *q : cachedbiomes)
        delete q;
    for (Quad *q : cachedstruct)
        delete q;
    if (spawn && spawn != (Pos*)-1)
    {
        delete spawn;
        delete strongholds;
        delete qsinfo;
    }
}

void QWorld::clearPool()
{
    isdel = true;
    QThreadPool::globalInstance()->clear();
    QThreadPool::globalInstance()->waitForDone();
    isdel = false;
}

void QWorld::setDim(int dim, int layeropt)
{
    clearPool();
    if (this->layeropt != layeropt)
    {
        for (Level& l : lvb)
            l.resizeLevel(cachedbiomes, 0, 0, 0, 0);
        cleancache(cachedbiomes, 0);
    }

    this->dim = dim;
    this->layeropt = layeropt;
    applySeed(&g, dim, wi.seed);

    // cache existing quads
    for (Level& l : lvb)
    {
        std::vector<Quad*> todel;
        for (Quad *q : l.cells)
        {
            if (q->done)
                cachedbiomes.push_back(q);
            else
                todel.push_back(q);
        }
        l.cells.swap(todel);
    }

    int pixs, lcnt;
    if (g.mc >= MC_1_18 || dim != DIM_OVERWORLD)
    {
        pixs = 128;
        lcnt = 6;
        qual = 2.0;
    }
    else
    {
        pixs = 512;
        lcnt = 5;
        qual = 1.0;
    }

    activelv = -1;
    lvb.clear();
    lvb.resize(lcnt);
    for (int i = 0, scale = 1; i < lcnt; i++, scale *= 4)
        lvb[i].init4map(this, pixs, scale);
}


int QWorld::getBiome(Pos p)
{
    Generator *g = &this->g;
    int scale = 1;
    int y = wi.y;

    if (activelv >= 0 && activelv < (int)lvb.size())
    {
        g = &lvb[activelv].g;
        scale = lvb[activelv].blocks / lvb[activelv].pixs;
        p.x = p.x / scale - (p.x < 0);
        p.z = p.z / scale - (p.z < 0);
        if (scale != 1)
            y /= 4;
    }

    int id = getBiomeAt(g, scale, p.x, y, p.z);
    return id;
}

QString QWorld::getBiomeName(Pos p)
{
    int id = getBiome(p);
    if (wi.mc >= MC_1_18 && dim == 0 && layeropt >= LOPT_NOISE_T_4 && layeropt <= LOPT_NOISE_W_4)
    {
        QString c;
        switch (layeropt)
        {
            case LOPT_NOISE_T_4: c = "T="; break;
            case LOPT_NOISE_H_4: c = "H="; break;
            case LOPT_NOISE_C_4: c = "C="; break;
            case LOPT_NOISE_E_4: c = "E="; break;
            case LOPT_NOISE_D_4: c = "D="; break;
            case LOPT_NOISE_W_4: c = "W="; break;
        }
        return c + QString::number(id);
    }
    const char *s = biome2str(wi.mc, id);
    return s ? s : "";
}

static void refreshQuadColor(Quad *q)
{
    QImage *img = q->img;
    if (!img)
        return;
    if (q->lopt <= LOPT_OCEAN_256 || q->g->mc < MC_1_18 || q->dim != 0 || q->g->bn.nptype < 0)
        biomesToImage(q->rgb, g_biomeColors, q->biomes, img->width(), img->height(), 1, 1);
}

void QWorld::refreshBiomeColors()
{
    g_mutex.lock();
    for (Level& l : lvb)
    {
        for (Quad *q : l.cells)
            refreshQuadColor(q);
    }
    for (Quad *q : cachedbiomes)
        refreshQuadColor(q);
    g_mutex.unlock();
}


void QWorld::cleancache(std::vector<Quad*>& cache, unsigned int maxsize)
{
    size_t n = cache.size();
    if (n < maxsize)
        return;

    size_t targetsize = 4 * maxsize / 5;

    std::vector<Quad*> newcache;
    for (size_t i = 0; i < n; i++)
    {
        Quad *q = cache[i];
        if (q->stopped)
        {
            delete q;
            continue;
        }
        if (n - i >= targetsize)
        {
            if (q->done || QThreadPool::globalInstance()->tryTake(q))
            {
                delete q;
                continue;
            }
        }
        newcache.push_back(q);
    }
    //printf("%zu -> %zu [%u]\n", n, newcache.size(), maxsize);
    cache.swap(newcache);
}

struct SpawnStronghold : public QRunnable
{
    QWorld *world;
    WorldInfo wi;

    SpawnStronghold(QWorld *world, WorldInfo wi) :
        world(world),wi(wi) {}

    void run()
    {
        Generator g;
        setupGenerator(&g, wi.mc, wi.large);
        applySeed(&g, 0, wi.seed);

        Pos *p = new Pos;
        *p = getSpawn(&g);
        world->spawn = p;
        if (world->isdel) return;

        StrongholdIter sh;
        initFirstStronghold(&sh, wi.mc, wi.seed);

        // note: pointer to atomic pointer
        QAtomicPointer<PosElement> *shpp = &world->strongholds;

        while (nextStronghold(&sh, &g) > 0)
        {
            if (world->isdel)
                return;
            PosElement *shp;
            (*shpp) = shp = new PosElement(sh.pos);
            shpp = &shp->next;
        }

        QVector<QuadInfo> *qsinfo = new QVector<QuadInfo>;

        if (!world->isdel)
            findQuadStructs(Swamp_Hut, &g, qsinfo);
        if (!world->isdel)
            findQuadStructs(Monument, &g, qsinfo);

        world->qsinfo = qsinfo;
    }
};

static bool draw_grid_rec(QPainter& painter, QRect &rec, qreal pix, int x, int z)
{
    painter.setPen(QPen(QColor(0, 0, 0, 96), 1));
    painter.drawRect(rec);

    QFont font = painter.font();
    if (pix < 100)
    {
        if (pix < 50)
            return false;
        QFont smallfont = font;
        smallfont.setPointSize(8);
        painter.setFont(smallfont);
    }

    QString s = QString::asprintf("%d,%d", x, z);
    QRect textrec = painter.fontMetrics()
            .boundingRect(rec, Qt::AlignLeft | Qt::AlignTop, s);

    painter.fillRect(textrec, QBrush(QColor(0, 0, 0, 128), Qt::SolidPattern));

    painter.setPen(QColor(255, 255, 255));
    painter.drawText(textrec, s);

    painter.setFont(font);
    return true;
}

void QWorld::draw(QPainter& painter, int vw, int vh, qreal focusx, qreal focusz, qreal blocks2pix)
{
    qreal uiw = vw / blocks2pix;
    qreal uih = vh / blocks2pix;

    qreal bx0 = focusx - uiw/2;
    qreal bz0 = focusz - uih/2;
    qreal bx1 = focusx + uiw/2;
    qreal bz1 = focusz + uih/2;

    // determine the active level, which represents a scale resolution of:
    // [0] 1:1, [1] 1:4, [2] 1:16, [3] 1:64, [4] 1:256
    qreal imgres = qual;
    for (activelv = 0; activelv < (int)lvb.size(); activelv++, imgres /= 4)
        if (blocks2pix > imgres)
            break;
    activelv--;

    for (int li = activelv+1; li >= activelv; --li)
    {
        if (li < 0 || li >= (int)lvb.size())
            continue;
        Level& l = lvb[li];
        for (Quad *q : l.cells)
        {
            if (!q->img)
                continue;
            // q was processed in another thread and is now done
            qreal ps = q->blocks * blocks2pix;
            qreal px = vw/2.0 + (q->ti) * ps - focusx * blocks2pix;
            qreal pz = vh/2.0 + (q->tj) * ps - focusz * blocks2pix;
            // account for the seam buffer pixels
            //ps += ((q->pixs / 128) * q->blocks / (qreal)q->pixs) * blocks2pix;
            QRect rec(floor(px),floor(pz), ceil(ps),ceil(ps));
            painter.drawImage(rec, *q->img);

            if (sshow[D_GRID] && !gridspacing)
            {
                draw_grid_rec(painter, rec, ps, q->ti*q->blocks, q->tj*q->blocks);
            }
        }
    }

    if (sshow[D_GRID] && gridspacing)
    {
        long x = floor(bx0 / gridspacing), w = floor(bx1 / gridspacing) - x + 1;
        long z = floor(bz0 / gridspacing), h = floor(bz1 / gridspacing) - z + 1;
        qreal ps = gridspacing * blocks2pix;

        if (ps > 50)
        {
            for (int j = 0; j < h; j++)
            {
                for (int i = 0; i < w; i++)
                {
                    qreal px = vw/2.0 + (x+i) * ps - focusx * blocks2pix;
                    qreal pz = vh/2.0 + (z+j) * ps - focusz * blocks2pix;
                    QRect rec(px, pz, ps, ps);
                    draw_grid_rec(painter, rec, ps, (x+i)*gridspacing, (z+j)*gridspacing);
                }
            }
        }
    }

    if (sshow[D_SLIME] && dim == 0 && blocks2pix*16 > 0.5)
    {
        long x = floor(bx0 / 16), w = floor(bx1 / 16) - x + 1;
        long z = floor(bz0 / 16), h = floor(bz1 / 16) - z + 1;

        // conditions when the slime overlay should be updated
        if (x < slimex || z < slimez ||
            x+w >= slimex+slimeimg.width() || z+h >= slimez+slimeimg.height() ||
            w*h*4 >= slimeimg.width()*slimeimg.height())
        {
            int pad = (int)(20 / blocks2pix); // 20*16 pixels movement before recalc
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
                    int isslime = isSlimeChunk(wi.seed, i+x, j+z);
                    slimeimg.setPixel(i, j, isslime);
                }
            }
        }

        qreal ps = 16 * blocks2pix;
        qreal px = vw/2.0 + slimex * ps - focusx * blocks2pix;
        qreal pz = vh/2.0 + slimez * ps - focusz * blocks2pix;

        QRect rec(round(px), round(pz), round(ps*slimeimg.width()), round(ps*slimeimg.height()));
        painter.drawImage(rec, slimeimg);
    }


    if (showBB && blocks2pix >= 1.0 && qsinfo && dim == 0)
    {
        for (QuadInfo qi : qAsConst(*qsinfo))
        {
            if (qi.typ == Swamp_Hut && !sshow[D_HUT])
                continue;
            if (qi.typ == Monument && !sshow[D_MONUMENT])
                continue;

            qreal x = vw/2.0 + (qi.afk.x - focusx) * blocks2pix;
            qreal y = vh/2.0 + (qi.afk.z - focusz) * blocks2pix;
            qreal r = 128.0 * blocks2pix;
            painter.setPen(QPen(QColor(192, 0, 0, 160), 1));
            painter.drawEllipse(QRectF(x-r, y-r, 2*r, 2*r));
            r = 16;
            painter.drawLine(QPointF(x-r,y), QPointF(x+r,y));
            painter.drawLine(QPointF(x,y-r), QPointF(x,y+r));
        }
    }

    for (int sopt = D_DESERT; sopt < D_SPAWN; sopt++)
    {
        Level& l = lvs[sopt];
        if (!sshow[sopt] || dim != l.dim || l.vis > blocks2pix)
            continue;

        std::map<const QPixmap*, std::vector<QPainter::PixmapFragment>> frags;

        for (Quad *q : l.cells)
        {
            if (!q->spos)
                continue;
            // q was processed in another thread and is now done
            for (VarPos& vp : *q->spos)
            {
                qreal x = vw/2.0 + (vp.p.x - focusx) * blocks2pix;
                qreal y = vh/2.0 + (vp.p.z - focusz) * blocks2pix;

                if (x < 0 || x >= vw || y < 0 || y >= vh)
                    continue;

                if (showBB && blocks2pix > 1.0)
                {
                    if (vp.v.sx && vp.v.sz)
                    {   // draw bounding box and move icon to its center
                        x += vp.v.x * blocks2pix;
                        y += vp.v.z * blocks2pix;
                        qreal dx = vp.v.sx * blocks2pix;
                        qreal dy = vp.v.sz * blocks2pix;
                        painter.setPen(QPen(QColor(192, 0, 0, 160), 1));
                        painter.drawRect(QRect(x, y, dx, dy));
                        // center icon on bb
                        x += dx / 2;
                        y += dy / 2;
                    }
                    painter.setPen(QPen(QColor(192, 0, 0, 128), 0));
                    for (Piece& p : vp.pieces)
                    {
                        qreal px = vw/2.0 + (p.bb0.x - focusx) * blocks2pix;
                        qreal py = vh/2.0 + (p.bb0.z - focusz) * blocks2pix;
                        qreal dx = (p.bb1.x - p.bb0.x + 1) * blocks2pix;
                        qreal dy = (p.bb1.z - p.bb0.z + 1) * blocks2pix;
                        painter.drawRect(QRect(px, py, dx, dy));
                    }
                }

                QPointF d = QPointF(x, y);

                Pos spos = {
                    vp.p.x ,//+ (vp.v.x + vp.v.sx / 2),
                    vp.p.z //+ (vp.v.z + vp.v.sz / 2)
                };

                const QPixmap& icon = getMapIcon(sopt, &vp);
                QRectF rec = icon.rect();
                if (seldo)
                {   // check for structure selection
                    QRectF r = rec;
                    r.moveCenter(d);
                    if (r.contains(selx, selz))
                    {
                        selopt = sopt;
                        selvp = vp;
                    }
                }
                if (selopt == sopt && selvp.p.x == spos.x && selvp.p.z == spos.z)
                {   // don't draw selected structure
                    continue;
                }

                frags[&icon].push_back(QPainter::PixmapFragment::create(d, rec));
            }
        }

        for (auto& it : frags)
            painter.drawPixmapFragments(it.second.data(), it.second.size(), *it.first);
    }

    Pos* sp = spawn; // atomic fetch
    if (sp && sp != (Pos*)-1 && sshow[D_SPAWN] && dim == 0)
    {
        qreal x = vw/2.0 + (sp->x - focusx) * blocks2pix;
        qreal y = vh/2.0 + (sp->z - focusz) * blocks2pix;

        QPointF d = QPointF(x, y);
        QRectF r = getMapIcon(D_SPAWN).rect();
        painter.drawPixmap(x-r.width()/2, y-r.height()/2, getMapIcon(D_SPAWN));

        if (seldo)
        {
            r.moveCenter(d);
            if (r.contains(selx, selz))
            {
                selopt = D_SPAWN;
                selvp = VarPos(*sp, -1);
            }
        }
    }

    QAtomicPointer<PosElement> shs = strongholds;
    if (shs && sshow[D_STRONGHOLD] && dim == 0)
    {
        std::vector<QPainter::PixmapFragment> frags;
        do
        {
            Pos p = (*shs).p;
            qreal x = vw/2.0 + (p.x - focusx) * blocks2pix;
            qreal y = vh/2.0 + (p.z - focusz) * blocks2pix;
            QPointF d = QPointF(x, y);
            QRectF r = getMapIcon(D_STRONGHOLD).rect();
            frags.push_back(QPainter::PixmapFragment::create(d, r));

            if (seldo)
            {
                r.moveCenter(d);
                if (r.contains(selx, selz))
                {
                    selopt = D_STRONGHOLD;
                    selvp = VarPos(p, -1);
                }
            }
        }
        while ((shs = (*shs).next));
        painter.drawPixmapFragments(frags.data(), frags.size(), getMapIcon(D_STRONGHOLD));
    }

    for (int sopt = D_DESERT; sopt < D_SPAWN; sopt++)
    {
        Level& l = lvs[sopt];
        if (l.vis <= blocks2pix && sshow[sopt] && dim == l.dim)
            l.update(cachedstruct, bx0, bz0, bx1, bz1);
        else if (l.vis * 4 > blocks2pix)
            l.update(cachedstruct, 0, 0, 0, 0);
    }
    for (int li = lvb.size()-1; li >= 0; --li)
    {
        Level& l = lvb[li];
        if (li == activelv || li == activelv+1)
            l.update(cachedbiomes, bx0, bz0, bx1, bz1);
        else
            l.update(cachedbiomes, 0, 0, 0, 0);
    }

    if (spawn == NULL)
    {   // start the spawn and stronghold worker thread if this is the first run
        if (sshow[D_SPAWN] || sshow[D_STRONGHOLD] || (showBB && blocks2pix >= 1.0))
        {
            spawn = (Pos*) -1;
            QThreadPool::globalInstance()->start(new SpawnStronghold(this, wi));
        }
    }

    if (seldo)
    {
        seldo = false;
    }

    if (selopt != D_NONE)
    {   // draw selection overlay
        const QPixmap& icon = getMapIcon(selopt, &selvp);
        qreal x = vw/2.0 + (selvp.p.x - focusx) * blocks2pix;
        qreal y = vh/2.0 + (selvp.p.z - focusz) * blocks2pix;
        QRect iconrec = icon.rect();
        qreal w = iconrec.width() * 1.5;
        qreal h = iconrec.height() * 1.5;
        painter.drawPixmap(x-w/2, y-h/2, w, h, icon);

        QFont f = QFont();
        f.setBold(true);
        painter.setFont(f);

        QString s = QString::asprintf(" %d,%d", selvp.p.x, selvp.p.z);
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

        iconrec = iconrec.translated(pad,pad);
        painter.drawPixmap(iconrec, icon);

        QStringList sinfo = selvp.detail();
        if (!sinfo.empty())
        {
            f = QFont();
            f.setPointSize(8);
            painter.setFont(f);

            s = sinfo.join(":");
            int xpos = textrec.right() + 3;
            textrec = painter.fontMetrics().boundingRect(0, 0, vw, vh, Qt::AlignLeft | Qt::AlignTop, s);
            textrec = textrec.translated(xpos+pad, 0);
            painter.fillRect(textrec.marginsAdded(QMargins(2,0,2,0)), QBrush(QColor(0,0,0,128), Qt::SolidPattern));
            painter.drawText(textrec, s, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
        }
    }

    if (activelv < 0)
        activelv = 0;
    if (activelv >= (int)lvb.size()-1)
        activelv = lvb.size() - 1;
    int pixs = lvb[activelv].pixs;
    unsigned int cachesize = memlimit / pixs / pixs / (3+4); // sizeof(RGB) + sizeof(biome_id)

    cleancache(cachedbiomes, cachesize);
    cleancache(cachedstruct, cachesize);

    if (0)
    {   // debug outline
        qreal rx0 = vw/2.0 + (bx0 - focusx) * blocks2pix;
        qreal rz0 = vh/2.0 + (bz0 - focusz) * blocks2pix;
        qreal rx1 = vw/2.0 + (bx1 - focusx) * blocks2pix;
        qreal rz1 = vh/2.0 + (bz1 - focusz) * blocks2pix;
        painter.setPen(QPen(QColor(255, 0, 0, 255), 1));
        painter.drawRect(QRect(rx0, rz0, rx1-rx0, rz1-rz0));
    }
}


