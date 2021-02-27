#ifndef QUAD_H
#define QUAD_H

#include <QRunnable>
#include <QImage>
#include <QPainter>
#include <QAtomicPointer>

#include "cubiomes/finders.h"

enum {
    D_NONE = -1,
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
    D_OUTPOST,
    D_PORTAL,
    D_SPAWN,
    D_STRONGHOLD,
    STRUCT_NUM
};


struct Level;

struct VarPos
{
    Pos p;
    int variant;
};

class Quad : public QRunnable
{
public:
    Quad(const Level* l, int i, int j);
    ~Quad();


    std::vector<VarPos> *addStruct(const StructureConfig sconf, LayerStack *g);
    std::vector<VarPos> *addTreasure(LayerStack *g);
    void run();

    int mc;
    const Layer *entry;
    int64_t seed;
    int ti, tj;
    int blocks;
    int pixs;
    int stype;

    uchar *rgb;

    // img and spos act as an atomic gate (with NULL or non-NULL indicating available results)
    QAtomicPointer<QImage> img;
    QAtomicPointer<std::vector<VarPos>> spos;

    std::atomic_bool done; // indicates that no further processing will occur

public:
    // externally managed (read/write in controller thread only)
    int prio;
    int stopped; // not done, and also not in processing queue
};


struct Level
{
    Level();
    ~Level();

    void init4map(int mc, int64_t ws, int pix, int layerscale);
    void init4struct(int mc, int64_t ws, int blocks, int stype);

    void resizeLevel(std::vector<Quad*>& cache, int x, int z, int w, int h);
    void update(std::vector<Quad*>& cache, qreal bx0, qreal bz0, qreal bx1, qreal bz1);

    std::vector<Quad*> cells;
    LayerStack g;
    Layer *entry;
    int64_t seed;
    int mc;
    int tx, tz, tw, th;
    int scale;
    int blocks;
    int pixs;
    int stype;
};


struct QWorld
{
    QWorld(int mc, int64_t seed);
    ~QWorld();

    void cleancache(std::vector<Quad*>& cache, unsigned int maxsize);

    void draw(QPainter& painter, int vw, int vh, qreal focusx, qreal focusz, qreal blocks2pix);


    int mc;
    int64_t seed;
    LayerStack g;

    // the visible area is managed in Quads of different scales (for biomes and structures),
    // which are managed in rectangular sections as levels
    std::vector<Level> lv;  // levels for biomes
    std::vector<Level> lvs; // levels for structures
    int activelv;           // currently visible level
    int structlv;           // currently visible structure level

    // processed Quads are cached until they are too far out of view
    std::vector<Quad*> cached;
    std::vector<Quad*> cachedstruct;
    unsigned int cachesize;

    bool sshow[STRUCT_NUM];

    // spawn and strongholds will be filled by a designated worker thread once results are done
    QAtomicPointer<Pos> spawn;
    QAtomicPointer<std::vector<Pos>> strongholds;
    // isdel is a flag for the worker thread to stop
    std::atomic_bool isdel;

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
