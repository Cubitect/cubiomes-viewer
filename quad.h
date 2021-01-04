#ifndef QUAD_H
#define QUAD_H

#include <QRunnable>
#include <QImage>
#include <QPainter>
#include <QAtomicPointer>
#include <QMutex>

#include <random>

#include "cubiomes/finders.h"
#include "cubiomes/util.h"

extern unsigned char biomeColors[256][3];
extern unsigned char tempsColors[256][3];

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
    D_OUTPOST,
    D_PORTAL,
    D_SPAWN,
    D_STRONGHOLD,
    STRUCT_NUM
};

inline const char* mc2str(int mc)
{
    switch (mc)
    {
    case MC_1_7:  return "1.7"; break;
    case MC_1_8:  return "1.8"; break;
    case MC_1_9:  return "1.9"; break;
    case MC_1_10: return "1.10"; break;
    case MC_1_11: return "1.11"; break;
    case MC_1_12: return "1.12"; break;
    case MC_1_13: return "1.13"; break;
    case MC_1_14: return "1.14"; break;
    case MC_1_15: return "1.15"; break;
    case MC_1_16: return "1.16"; break;
    default: return NULL;
    }
}

inline int str2mc(const char *s)
{
    if (!strcmp(s, "1.16")) return MC_1_16;
    if (!strcmp(s, "1.15")) return MC_1_15;
    if (!strcmp(s, "1.14")) return MC_1_14;
    if (!strcmp(s, "1.13")) return MC_1_13;
    if (!strcmp(s, "1.12")) return MC_1_12;
    if (!strcmp(s, "1.11")) return MC_1_11;
    if (!strcmp(s, "1.10")) return MC_1_10;
    if (!strcmp(s, "1.9")) return MC_1_9;
    if (!strcmp(s, "1.8")) return MC_1_8;
    if (!strcmp(s, "1.7")) return MC_1_7;
    return -1;
}

// get a random 64-bit integer
static inline int64_t getRnd64()
{
    static QMutex mutex;
    static std::random_device rd;
    static std::mt19937_64 mt(rd());
    static uint64_t x = (uint64_t) time(0);
    int64_t ret = 0;
    mutex.lock();
    if (rd.entropy())
    {
        std::uniform_int_distribution<int64_t> d;
        ret = d(mt);
    }
    else
    {
        const uint64_t c = 0xd6e8feb86659fd93ULL;
        x ^= x >> 32;
        x *= c;
        x ^= x >> 32;
        x *= c;
        x ^= x >> 32;
        ret = (int64_t) x;
    }
    mutex.unlock();
    return ret;
}

enum { S_TEXT, S_NUMERIC, S_RANDOM };
inline int str2seed(const char *str, int64_t *out)
{
    int slen = strlen(str);
    char *p;
    if (slen == 0)
    {
        *out = getRnd64();
        return S_RANDOM;
    }

    *out = strtoll(str, &p, 10);
    if (str + slen == p)
        return S_NUMERIC;

    // String.hashCode();
    *out = 0;
    for (int i = 0; i < slen; i++)
        *out = 31*(*out) + str[i];
    *out &= (uint32_t)-1;
    return S_TEXT;
}


struct Level;

class Quad : public QRunnable
{
public:
    Quad(const Level* l, int i, int j);
    ~Quad();


    std::vector<Pos> *addStruct(const StructureConfig sconf, LayerStack *g);
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
    QAtomicPointer<std::vector<Pos>> spos;

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

    qreal qual; // quality, i.e. maximum pixels per 'block' at the current layer

    QPixmap icons[STRUCT_NUM];
};



#endif // QUAD_H
