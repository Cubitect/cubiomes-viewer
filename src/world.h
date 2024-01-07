#ifndef WORLD_H
#define WORLD_H

#include "config.h"
#include "search.h"

#include <QRunnable>
#include <QImage>
#include <QPainter>
#include <QAtomicPointer>
#include <QIcon>
#include <QMutex>
#include <QThread>

#include "cubiomes/quadbase.h"


struct Level;

struct VarPos
{
    VarPos(Pos p, int type) : p(p),type(type),v(),pieces() {}
    Pos p;
    int type;
    StructureVariant v;
    std::vector<Piece> pieces;

    QStringList detail() const;
};

const QPixmap& getMapIcon(int opt, VarPos *variation = 0);
QIcon getBiomeIcon(int id, bool warn = false);

void getStructs(std::vector<VarPos> *out, const StructureConfig sconf,
        WorldInfo wi, int dim, int x0, int z0, int x1, int z1, bool nogen = false);

enum {
    HV_GRAYSCALE        = 0,
    HV_SHADING          = 1,
    HV_CONTOURS         = 2,
    HV_CONTOURS_SHADING = 3,
};
void applyHeightShading(unsigned char *rgb, Range r,
        const Generator *g, const SurfaceNoise *sn, int stepbits, int mode,
        bool bicubic, const std::atomic_bool *abort);

struct Scheduled : public QRunnable
{
    Scheduled() : prio(),stopped(),next(),done(),isdel() {}
    // externally managed (read/write in controller thread only)
    float prio;
    int stopped; // not done, and also not in processing queue
    Scheduled *next;
    std::atomic_bool done; // indicates that no further processing will occur
    std::atomic_bool *isdel;
};

struct Quad : public Scheduled
{
    Quad(const Level* l, int64_t i, int64_t j);
    ~Quad();

    void run();

    WorldInfo wi;
    int dim;
    LayerOpt lopt;
    const Generator *g;
    const SurfaceNoise *sn;
    int hd;
    int scale;
    int ti, tj;
    int blocks;
    int pixs;
    int sopt;

    int *biomes;
    uchar *rgb;

    // img and spos act as an atomic gate (with NULL or non-NULL indicating available results)
    QAtomicPointer<QImage> img;
    QAtomicPointer<std::vector<VarPos>> spos;
};

struct QWorld;
struct Level
{
    Level();
    ~Level();

    void init4map(QWorld *world, int pix, int layerscale);
    void init4struct(QWorld *world, int sopt);

    void resizeLevel(std::vector<Quad*>& cache, int64_t x, int64_t z, int64_t w, int64_t h);
    void update(std::vector<Quad*>& cache, qreal bx0, qreal bz0, qreal bx1, qreal bz1);
    void setInactive(std::vector<Quad*>& cache);

    QWorld *world;
    std::vector<Quad*> cells;
    Generator g;
    SurfaceNoise sn;
    Layer *entry;
    LayerOpt lopt;
    WorldInfo wi;
    int dim;
    int tx, tz, tw, th;
    int hd;
    int scale;
    int blocks;
    int pixs;
    int sopt;
    double vis;
    std::atomic_bool *isdel;
};

struct PosElement
{
    PosElement(Pos p_) : next(), p(p_) {}
    ~PosElement() { delete next; next = nullptr; }
    QAtomicPointer<PosElement> next;
    Pos p;
};

struct Shape
{
    enum { RECT, LINE, CIRCLE } type;
    int dim;
    int r;
    Pos p1, p2;
};

class MapWorker : public QThread
{
    Q_OBJECT
public:
    MapWorker() : QThread(), world() {}
    virtual ~MapWorker() {}
    virtual void run() override;
signals:
    void quadDone();
public:
    QWorld *world;
};

struct QWorld : public QObject
{
    Q_OBJECT
public:
    QWorld(WorldInfo wi, int dim = 0, LayerOpt lopt = LayerOpt());
    virtual ~QWorld();

    void setDim(int dim, LayerOpt lopt);

    void cleancache(std::vector<Quad*>& cache, unsigned int maxsize);

    void draw(QPainter& painter, int vw, int vh, qreal focusx, qreal focusz, qreal blocks2pix);

    void setSelectPos(QPoint pos);

    int getBiome(Pos p);
    QString getBiomeName(Pos p);
    int estimateSurface(Pos p);

    void refreshBiomeColors();

    void startWorkers();
    void waitForIdle();
    bool isBusy();
    void clear();
    void add(Scheduled *q);
    Scheduled *take(Scheduled *q);
    Scheduled *requestQuad();

signals:
    void update();

public:
    WorldInfo wi;
    int dim;
    MapConfig mconfig;
    LayerOpt lopt;
    Generator g;
    SurfaceNoise sn;

    // the visible area is managed in Quads of different scales (for biomes and structures),
    // which are managed in rectangular sections as levels
    std::vector<Level> lvb;     // levels for biomes
    std::vector<Level> lvs;     // levels for structures
    int activelv;

    // processed Quads are cached until they are too far out of view
    std::vector<Quad*> cachedbiomes;
    std::vector<Quad*> cachedstruct;
    uint64_t memlimit;
    QMutex mutex;
    Scheduled *queue;
    std::vector<MapWorker> workers;
    int threadlimit;

    bool sshow[D_STRUCT_NUM];
    bool showBB;
    int gridspacing;
    int gridmultiplier;

    // some features such as the world spawn and strongholds will be filled by
    // a designated worker thread once results are done
    QAtomicPointer<Pos> spawn;
    QAtomicPointer<PosElement> strongholds;
    QAtomicPointer<QVector<QuadInfo>> qsinfo;
    // isdel is a flag for the worker thread to stop
    std::atomic_bool isdel;

    // slime overlay
    QImage slimeimg;
    long slimex, slimez;

    // shapes to overlay
    std::vector<Shape> shapes;

    // structure selection from mouse position
    bool seldo;
    qreal selx, selz;
    int selopt;
    VarPos selvp;

    qreal qual; // quality, i.e. maximum pixels per 'block' at the current layer
};


#endif // WORLD_H
