#ifndef QUAD_H
#define QUAD_H

#include "settings.h"
#include "search.h"

#include <QRunnable>
#include <QImage>
#include <QPainter>
#include <QAtomicPointer>

#include "cubiomes/finders.h"


enum {
    D_NONE = -1,
    // generics
    D_GRID,
    D_SLIME,
    // structures
    D_DESERT,
    D_JUNGLE,
    D_IGLOO,
    D_HUT,
    D_VILLAGE,
    D_MANSION,
    D_MONUMENT,
    D_RUINS,
    D_SHIPWRECK,
    D_TREASURE,
    D_MINESHAFT,
    D_OUTPOST,
    D_PORTAL,
    D_PORTALN,
    D_FORTESS,
    D_BASTION,
    D_ENDCITY,
    D_GATEWAY,
    // non-recurring structures
    D_SPAWN,
    D_STRONGHOLD,
    STRUCT_NUM
};

inline const char *mapopt2str(int opt)
{
    switch (opt)
    {
    case D_GRID:        return "grid";
    case D_SLIME:       return "slime";
    case D_DESERT:      return "desert";
    case D_JUNGLE:      return "jungle";
    case D_IGLOO:       return "igloo";
    case D_HUT:         return "hut";
    case D_VILLAGE:     return "village";
    case D_MANSION:     return "mansion";
    case D_MONUMENT:    return "monument";
    case D_RUINS:       return "ruins";
    case D_SHIPWRECK:   return "shipwreck";
    case D_TREASURE:    return "treasure";
    case D_MINESHAFT:   return "mineshaft";
    case D_OUTPOST:     return "outpost";
    case D_PORTAL:      return "portal";
    case D_PORTALN:     return "portaln";
    case D_SPAWN:       return "spawn";
    case D_STRONGHOLD:  return "stronghold";
    case D_FORTESS:     return "fortress";
    case D_BASTION:     return "bastion";
    case D_ENDCITY:     return "endcity";
    case D_GATEWAY:     return "gateway";
    default:            return "";
    }
}

inline int str2mapopt(const char *s)
{
    if (!strcmp(s, "grid"))         return D_GRID;
    if (!strcmp(s, "slime"))        return D_SLIME;
    if (!strcmp(s, "desert"))       return D_DESERT;
    if (!strcmp(s, "jungle"))       return D_JUNGLE;
    if (!strcmp(s, "igloo"))        return D_IGLOO;
    if (!strcmp(s, "hut"))          return D_HUT;
    if (!strcmp(s, "village"))      return D_VILLAGE;
    if (!strcmp(s, "mansion"))      return D_MANSION;
    if (!strcmp(s, "monument"))     return D_MONUMENT;
    if (!strcmp(s, "ruins"))        return D_RUINS;
    if (!strcmp(s, "shipwreck"))    return D_SHIPWRECK;
    if (!strcmp(s, "treasure"))     return D_TREASURE;
    if (!strcmp(s, "mineshaft"))    return D_MINESHAFT;
    if (!strcmp(s, "outpost"))      return D_OUTPOST;
    if (!strcmp(s, "portal"))       return D_PORTAL;
    if (!strcmp(s, "portaln"))      return D_PORTALN;
    if (!strcmp(s, "spawn"))        return D_SPAWN;
    if (!strcmp(s, "stronghold"))   return D_STRONGHOLD;
    if (!strcmp(s, "fortress"))     return D_FORTESS;
    if (!strcmp(s, "bastion"))      return D_BASTION;
    if (!strcmp(s, "endcity"))      return D_ENDCITY;
    if (!strcmp(s, "gateway"))      return D_GATEWAY;
    return D_NONE;
}

inline int mapopt2stype(int opt)
{
    switch (opt)
    {
    case D_DESERT:      return Desert_Pyramid;
    case D_JUNGLE:      return Jungle_Pyramid;
    case D_IGLOO:       return Igloo;
    case D_HUT:         return Swamp_Hut;
    case D_VILLAGE:     return Village;
    case D_MANSION:     return Mansion;
    case D_MONUMENT:    return Monument;
    case D_RUINS:       return Ocean_Ruin;
    case D_SHIPWRECK:   return Shipwreck;
    case D_TREASURE:    return Treasure;
    case D_MINESHAFT:   return Mineshaft;
    case D_OUTPOST:     return Outpost;
    case D_PORTAL:      return Ruined_Portal;
    case D_PORTALN:     return Ruined_Portal_N;
    case D_FORTESS:     return Fortress;
    case D_BASTION:     return Bastion;
    case D_ENDCITY:     return End_City;
    case D_GATEWAY:     return End_Gateway;
    default:
        return -1;
    }
}

struct Level;

struct VarPos
{
    VarPos(Pos p) : p(p),sx(),sz(),variant() {}
    Pos p;
    int sx, sz;
    int variant;
};

void getStructs(std::vector<VarPos> *out, const StructureConfig sconf,
        WorldInfo wi, int dim, int x0, int z0, int x1, int z1);

class Quad : public QRunnable
{
public:
    Quad(const Level* l, int i, int j);
    ~Quad();

    void run();

    WorldInfo wi;
    int dim;
    const Generator *g;
    int scale;
    int ti, tj;
    int blocks;
    int pixs;
    int sopt;

    uchar *rgb;

    // img and spos act as an atomic gate (with NULL or non-NULL indicating available results)
    QAtomicPointer<QImage> img;
    QAtomicPointer<std::vector<VarPos>> spos;

    std::atomic_bool done; // indicates that no further processing will occur
    std::atomic_bool *isdel;

public:
    // externally managed (read/write in controller thread only)
    int prio;
    int stopped; // not done, and also not in processing queue
};

struct QWorld;
struct Level
{
    Level();
    ~Level();

    void init4map(QWorld *w, int dim, int pix, int layerscale);
    void init4struct(QWorld *w, int dim, int blocks, int sopt, int viewlv);

    void resizeLevel(std::vector<Quad*>& cache, int x, int z, int w, int h);
    void update(std::vector<Quad*>& cache, qreal bx0, qreal bz0, qreal bx1, qreal bz1);

    std::vector<Quad*> cells;
    Generator g;
    Layer *entry;
    WorldInfo wi;
    int dim;
    int tx, tz, tw, th;
    int scale;
    int blocks;
    int pixs;
    int sopt;
    int viewlv;
    std::atomic_bool *isdel;
};


struct QWorld
{
    QWorld(WorldInfo wi, int dim = 0);
    ~QWorld();

    void clearPool();

    void setDim(int dim);

    void cleancache(std::vector<Quad*>& cache, unsigned int maxsize);

    void draw(QPainter& painter, int vw, int vh, qreal focusx, qreal focusz, qreal blocks2pix);

    int getBiome(Pos p);

    WorldInfo wi;
    int dim;
    Generator g;

    // the visible area is managed in Quads of different scales (for biomes and structures),
    // which are managed in rectangular sections as levels
    std::vector<Level> lvb;     // levels for biomes
    std::vector<Level> lvs;     // levels for structures
    int activelv;               // currently visible level

    // processed Quads are cached until they are too far out of view
    std::vector<Quad*> cachedbiomes;
    std::vector<Quad*> cachedstruct;
    unsigned int cachesize;

    bool sshow[STRUCT_NUM];
    bool showBB;
    int gridspacing;

    // some features such as the world spawn and strongholds will be filled by
    // a designated worker thread once results are done
    QAtomicPointer<Pos> spawn;
    QAtomicPointer<std::vector<Pos>> strongholds;
    QAtomicPointer<QVector<QuadInfo>> qsinfo;
    // isdel is a flag for the worker thread to stop
    std::atomic_bool isdel;

    // slime overlay
    QImage slimeimg;
    long slimex, slimez;

    // structure selection from mouse position
    bool seldo;
    qreal selx, selz;
    int seltype;
    Pos selpos;
    int selvar;

    qreal qual; // quality, i.e. maximum pixels per 'block' at the current layer

    QPixmap icons[STRUCT_NUM];
    QPixmap iconzvil;
};



#endif // QUAD_H
