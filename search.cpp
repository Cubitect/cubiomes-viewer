#include "search.h"
#include "mainwindow.h"

#include <QThread>

#include <unistd.h>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

extern MainWindow *gMainWindowInstance;

// Quad monument bases are too expensive to generate on the fly and there are
// so few of them that they can be hard coded, rather than loading from a file.
const int64_t g_qm_90[] = {
    35624347962,
    775379617447,
    3752024106001,
    6745614706047,
    8462955635640,
    9735132392160,
    10800300288310,
    11692770796600,
    15412118464919,
    17507532114595,
    20824644731942,
    22102701941684,
    23762458057008,
    25706119531719,
    30282993829760,
    35236447787959,
    36751564646809,
    36982453953202,
    40642997855160,
    40847762196737,
    42617133245824,
    43070154706705,
    45369094039004,
    46388611271005,
    49815551084927,
    55209383814200,
    60038067905464,
    62013253870977,
    64801897210897,
    64967864749064,
    65164403480601,
    69458416339619,
    69968827844832,
    73925647436179,
    75345272448242,
    75897465569912,
    75947388701440,
    77139057518714,
    80473739688725,
    80869452536592,
    85241154759688,
    85336458606471,
    85712023677536,
    88230710669198,
    89435894990993,
    91999529042303,
    96363285020895,
    96666161703135,
    97326727082256,
    108818298610594,
    110070513256351,
    110929712933903,
    113209235869070,
    117558655912178,
    121197807897998,
    141209504516769,
    141849611674552,
    143737598577672,
    152637000035722,
    157050650953070,
    170156303860785,
    177801550039713,
    183906212826120,
    184103110528026,
    185417005583496,
    195760072598584,
    197672667717871,
    201305960334702,
    206145718978215,
    208212272645282,
    210644031421853,
    211691056285867,
    211760277005049,
    214621983270272,
    215210457001278,
    215223265529230,
    218746494888768,
    220178916199595,
    220411714922367,
    222407756997991,
    222506979025848,
    223366717375839,
    226043527056401,
    226089475358070,
    226837059463777,
    228023673284850,
    230531729507551,
    233072888622088,
    233864988591288,
    235857097051144,
    236329863308326,
    240806176474748,
    241664440380224,
    244715397179172,
    248444967740433,
    249746457285392,
    252133682596189,
    254891649599679,
    256867214419776,
    257374503348631,
    257985200458337,
    258999802520935,
    260070444629216,
    260286378141952,
    261039947696903,
    264768533253187,
    265956688913983,
};

const int64_t g_qm_95[] = {
    775379617447,
    40642997855160,
    75345272448242,
    85241154759688,
    143737598577672,
    201305960334702,
    206145718978215,
    220178916199595,
    226043527056401,
};


__attribute__((const, used))
static int qhutQual(int low20)
{
    switch (low20)
    {
        case 0x1272d: return F_QH_BARELY;
        case 0x17908: return F_QH_BARELY;
        case 0x367b9: return F_QH_BARELY;
        case 0x43f18: return F_QH_IDEAL;
        case 0x487c9: return F_QH_BARELY;
        case 0x487ce: return F_QH_BARELY;
        case 0x50aa7: return F_QH_BARELY;
        case 0x647b5: return F_QH_NORMAL;

        case 0x65118: return F_QH_BARELY;
        case 0x75618: return F_QH_NORMAL;
        case 0x79a0a: return F_QH_IDEAL;
        case 0x89718: return F_QH_NORMAL;
        case 0x9371a: return F_QH_NORMAL;
        case 0x967ec: return F_QH_BARELY;
        case 0xa3d0a: return F_QH_BARELY;
        case 0xa5918: return F_QH_BARELY;

        case 0xa591d: return F_QH_BARELY;
        case 0xa5a08: return F_QH_NORMAL;
        case 0xb5e18: return F_QH_NORMAL;
        case 0xc6749: return F_QH_BARELY;
        case 0xc6d9a: return F_QH_BARELY;
        case 0xc751a: return F_QH_CLASSIC;
        case 0xd7108: return F_QH_BARELY;
        case 0xd717a: return F_QH_BARELY;

        case 0xe2739: return F_QH_BARELY;
        case 0xe9918: return F_QH_BARELY;
        case 0xee1c4: return F_QH_BARELY;
        case 0xf520a: return F_QH_IDEAL;

        default: return 0;
    }
}

// returns for a >90% quadmonument the number of blocks, by area, in spawn range
__attribute__((const, used))
static int qmonumentQual(int64_t s48)
{
    switch ((s48) & ((1LL<<48)-1))
    {
        case 35624347962LL:     return 12409;
        case 775379617447LL:    return 12796;
        case 3752024106001LL:   return 12583;
        case 6745614706047LL:   return 12470;
        case 8462955635640LL:   return 12190;
        case 9735132392160LL:   return 12234;
        case 10800300288310LL:  return 12443;
        case 11692770796600LL:  return 12748;
        case 15412118464919LL:  return 12463;
        case 17507532114595LL:  return 12272;
        case 20824644731942LL:  return 12470;
        case 22102701941684LL:  return 12227;
        case 23762458057008LL:  return 12165;
        case 25706119531719LL:  return 12163;
        case 30282993829760LL:  return 12236;
        case 35236447787959LL:  return 12338;
        case 36751564646809LL:  return 12459;
        case 36982453953202LL:  return 12499;
        case 40642997855160LL:  return 12983;
        case 40847762196737LL:  return 12296;
        case 42617133245824LL:  return 12234;
        case 43070154706705LL:  return 12627;
        case 45369094039004LL:  return 12190;
        case 46388611271005LL:  return 12299;
        case 49815551084927LL:  return 12296;
        case 55209383814200LL:  return 12632;
        case 60038067905464LL:  return 12227;
        case 62013253870977LL:  return 12470;
        case 64801897210897LL:  return 12780;
        case 64967864749064LL:  return 12376;
        case 65164403480601LL:  return 12125;
        case 69458416339619LL:  return 12610;
        case 69968827844832LL:  return 12236;
        case 73925647436179LL:  return 12168;
        case 75345272448242LL:  return 12836;
        case 75897465569912LL:  return 12343;
        case 75947388701440LL:  return 12234;
        case 77139057518714LL:  return 12155;
        case 80473739688725LL:  return 12155;
        case 80869452536592LL:  return 12165;
        case 85241154759688LL:  return 12799;
        case 85336458606471LL:  return 12651;
        case 85712023677536LL:  return 12212;
        case 88230710669198LL:  return 12499;
        case 89435894990993LL:  return 12115;
        case 91999529042303LL:  return 12253;
        case 96363285020895LL:  return 12253;
        case 96666161703135LL:  return 12470;
        case 97326727082256LL:  return 12165;
        case 108818298610594LL: return 12150;
        case 110070513256351LL: return 12400;
        case 110929712933903LL: return 12348;
        case 113209235869070LL: return 12130;
        case 117558655912178LL: return 12687;
        case 121197807897998LL: return 12130;
        case 141209504516769LL: return 12147;
        case 141849611674552LL: return 12630;
        case 143737598577672LL: return 12938;
        case 152637000035722LL: return 12130;
        case 157050650953070LL: return 12588;
        case 170156303860785LL: return 12348;
        case 177801550039713LL: return 12389;
        case 183906212826120LL: return 12358;
        case 184103110528026LL: return 12630;
        case 185417005583496LL: return 12186;
        case 195760072598584LL: return 12118;
        case 197672667717871LL: return 12553;
        case 201305960334702LL: return 12948;
        case 206145718978215LL: return 12796;
        case 208212272645282LL: return 12317;
        case 210644031421853LL: return 12261;
        case 211691056285867LL: return 12478;
        case 211760277005049LL: return 12539;
        case 214621983270272LL: return 12236;
        case 215210457001278LL: return 12372;
        case 215223265529230LL: return 12499;
        case 218746494888768LL: return 12234;
        case 220178916199595LL: return 12848;
        case 220411714922367LL: return 12470;
        case 222407756997991LL: return 12458;
        case 222506979025848LL: return 12632;
        case 223366717375839LL: return 12296;
        case 226043527056401LL: return 13028; // best
        case 226089475358070LL: return 12285;
        case 226837059463777LL: return 12305;
        case 228023673284850LL: return 12742;
        case 230531729507551LL: return 12296;
        case 233072888622088LL: return 12376;
        case 233864988591288LL: return 12376;
        case 235857097051144LL: return 12632;
        case 236329863308326LL: return 12396;
        case 240806176474748LL: return 12190;
        case 241664440380224LL: return 12118;
        case 244715397179172LL: return 12300;
        case 248444967740433LL: return 12780;
        case 249746457285392LL: return 12391;
        case 252133682596189LL: return 12299;
        case 254891649599679LL: return 12296;
        case 256867214419776LL: return 12234;
        case 257374503348631LL: return 12391;
        case 257985200458337LL: return 12118;
        case 258999802520935LL: return 12290;
        case 260070444629216LL: return 12168;
        case 260286378141952LL: return 12234;
        case 261039947696903LL: return 12168;
        case 264768533253187LL: return 12242;
        case 265956688913983LL: return 12118;

        default: return 0;
    }
}

int check(int64_t s48, void *data)
{
    const StructureConfig sconf = *(const StructureConfig*) data;
    return isQuadBase(sconf, s48 - sconf.salt, 128);
}

/* Loads a seed list for a filter type from disk, or generates it if neccessary.
 * @mc      mincreaft version
 * @ftyp    filter type
 * @qb      output seed base list
 * @qbn     output length of seed base list
 * @dyn     list was dynamically allocated and requires a free
 * @sconf   structure configuration used for the bases
 */
static void genSeedBases(int mc, int ftyp, const int64_t **qb, int64_t *qbn,
                         int *dyn, StructureConfig *sconf)
{
    char fnam[128];

    const char *lbstr = NULL;
    const int64_t *lbset = NULL;
    int64_t lbcnt = 0;
    int64_t *dqb = NULL;

    *qb = NULL;
    *qbn = 0;
    *dyn = 0;

    switch (ftyp)
    {
    case F_QH_IDEAL:
        lbstr = "ideal";
        lbset = low20QuadIdeal;
        lbcnt = sizeof(low20QuadIdeal) / sizeof(int64_t);
        goto L_QH_ANY;
    case F_QH_CLASSIC:
        lbstr = "cassic";
        lbset = low20QuadClassic;
        lbcnt = sizeof(low20QuadClassic) / sizeof(int64_t);
        goto L_QH_ANY;
    case F_QH_NORMAL:
        lbstr = "normal";
        lbset = low20QuadHutNormal;
        lbcnt = sizeof(low20QuadHutNormal) / sizeof(int64_t);
        goto L_QH_ANY;
    case F_QH_BARELY:
        lbstr = "barely";
        lbset = low20QuadHutBarely;
        lbcnt = sizeof(low20QuadHutBarely) / sizeof(int64_t);
        goto L_QH_ANY;
L_QH_ANY:
        snprintf(fnam, sizeof(fnam), "protobases/quad_%s.txt", lbstr);
        *sconf = mc <= MC_1_12 ? SWAMP_HUT_CONFIG_112 : SWAMP_HUT_CONFIG;

        if ((dqb = loadSavedSeeds(fnam, qbn)) == NULL)
        {
            QMetaObject::invokeMethod(gMainWindowInstance, "openProtobaseMsg", Qt::QueuedConnection, Q_ARG(QString, QString(fnam)));

            int threads = QThread::idealThreadCount();
            int err = searchAll48(&dqb, qbn, fnam, threads, lbset, lbcnt, 20, check, sconf);

            QMetaObject::invokeMethod(gMainWindowInstance, "closeProtobaseMsg", Qt::BlockingQueuedConnection);

            if (err)
            {
                QMetaObject::invokeMethod(
                        gMainWindowInstance, "warning", Qt::BlockingQueuedConnection,
                        Q_ARG(QString, QString("Warning")),
                        Q_ARG(QString, QString("Failed to generate protobases.")));
                return;
            }
        }
        if (dqb)
        {
            // convert protobases to proper bases by subtracting the salt
            for (int64_t i = 0; i < (*qbn); i++)
                dqb[i] -= sconf->salt;
            *qb = (const int64_t*)dqb;
            *dyn = 1;
        }
        break;

    case F_QM_95:
        *qb = g_qm_95;
        *qbn = sizeof(g_qm_95) / sizeof(int64_t);
        *sconf = MONUMENT_CONFIG;
        *dyn = 0;
        break;
    case F_QM_90:
        *qb = g_qm_90;
        *qbn = sizeof(g_qm_90) / sizeof(int64_t);
        *sconf = MONUMENT_CONFIG;
        *dyn = 0;
        break;
    }
}

static int cmp_baseitem(const void *a, const void *b)
{
    return *(int64_t*)a > *(int64_t*)b;
}

/* Produces a list of seed bases from precomputed lists, provided all candidates
 * fit into a buffer.
 *
 * @param mc        mincraft version
 * @param cond      conditions
 * @param ccnt      number of conditions
 * @param bufmax    maximum allowed buffer size
 */
CandidateList getCandidates(int mc, const Condition *cond, int ccnt, int64_t bufmax)
{
    int ci;
    CandidateList clist = {};

    for (ci = 0; ci < ccnt; ci++)
    {
        int64_t qbn = 0;
        const int64_t *qb = NULL;
        StructureConfig sconf;
        int dyn;

        if (cond[ci].relative == 0)
        {
            genSeedBases(mc, cond[ci].type, &qb, &qbn, &dyn, &sconf);

            if (qb)
            {
                int x = cond[ci].x1;
                int z = cond[ci].z1;
                int w = cond[ci].x2 - x + 1;
                int h = cond[ci].z2 - z + 1;

                // does the set of candidates for this condition fit in memory?
                if (qbn * w*h < bufmax * 4 * (int64_t)sizeof(*clist.items->spos))
                {
                    if (clist.items == NULL)
                    {
                        clist.bcnt = qbn * w*h;
                        clist.scnt = 4;
                        clist.isiz = sizeof(*clist.items) + clist.scnt * sizeof(*clist.items->spos);
                        clist.mem = (char*) calloc(clist.bcnt, clist.isiz);

                        Candidate *item = (Candidate*)(clist.mem);

                        int i, j;
                        int64_t q;
                        for (j = 0; j < h; j++)
                        {
                            for (i = 0; i < w; i++)
                            {
                                for (q = 0; q < qbn; q++)
                                {
                                    item->seed = moveStructure(qb[q], x+i, z+j);
                                    item->spos[0].sconf = sconf;
                                    item->spos[0].x = x+i;
                                    item->spos[0].z = z+j;
                                    item->spos[1].sconf = sconf;
                                    item->spos[1].x = x+i+1;
                                    item->spos[1].z = z+j;
                                    item->spos[2].sconf = sconf;
                                    item->spos[2].x = x+i;
                                    item->spos[2].z = z+j+1;
                                    item->spos[3].sconf = sconf;
                                    item->spos[3].x = x+i+1;
                                    item->spos[3].z = z+j+1;
                                    item = (Candidate*)((char*)item + clist.isiz);
                                }
                            }
                        }
                    }
                    else
                    {
                        // TODO:
                        // merge, i.e. filter out seeds and add new structure condition to item->spos
                    }
                }

                if (dyn)
                    free((void*)qb);
            }
        }
    }

    if (clist.items)
    {
        qsort(clist.items, clist.bcnt, clist.isiz, cmp_baseitem);
    }

    return clist;
}


static bool intersectLineLine(double ax1, double az1, double ax2, double az2, double bx1, double bz1, double bx2, double bz2)
{
    double ax = ax2 - ax1, az = az2 - az1;
    double bx = bx2 - bx1, bz = bz2 - bz1;
    double adotb = ax * bz - az * bx;
    if (adotb == 0)
        return false; // parallel

    double cx = bx1 - ax1, cz = bz1 - az1;
    double t;
    t = (cx * az - cz * ax) / adotb;
    if (t < 0 || t > 1)
        return false;
    t = (cx * bz - cz * bx) / adotb;
    if (t < 0 || t > 1)
        return false;

    return true;
}

// does the line segment l1->l2 intersect the rectangle r1->r2
static bool intersectRectLine(double rx1, double rz1, double rx2, double rz2, double lx1, double lz1, double lx2, double lz2)
{
    if (lx1 >= rx1 && lx1 <= rx2 && lz1 >= rz1 && lz1 <= rz2) return true;
    if (lx2 >= rx1 && lx2 <= rx2 && lz2 >= rz1 && lz2 <= rz2) return true;
    if (intersectLineLine(lx1, lz1, lx2, lz2, rx1, rz1, rx1, rz2)) return true;
    if (intersectLineLine(lx1, lz1, lx2, lz2, rx1, rz2, rx2, rz2)) return true;
    if (intersectLineLine(lx1, lz1, lx2, lz2, rx2, rz2, rx2, rz1)) return true;
    if (intersectLineLine(lx1, lz1, lx2, lz2, rx2, rz1, rx1, rz1)) return true;
    return false;
}

static bool isInnerRingOk(int mc, int64_t seed, int x1, int z1, int x2, int z2, int r1, int r2)
{
    StrongholdIter sh;
    Pos p = initFirstStronghold(&sh, mc, seed);

    if (p.x >= x1 && p.x <= x2 && p.z >= z1 && p.z <= z2)
        return true;
    // Do a ray cast analysis, checking if any of the generation angles intersect the area.
    double c, s;
    c = cos(sh.angle + M_PI*2/3);
    s = sin(sh.angle + M_PI*2/3);
    if (intersectRectLine(x1-112, z1-112, x2+112, z2+112, c*r1, s*r1, c*r2, s*r2))
        return true;
    c = cos(sh.angle + M_PI*4/3);
    s = sin(sh.angle + M_PI*4/3);
    if (intersectRectLine(x1-112, z1-112, x2+112, z2+112, c*r1, s*r1, c*r2, s*r2))
        return true;

    return false;
}

int testCond(StructPos *spos, int64_t seed, const Condition *cond, int mc, LayerStack *g, volatile bool *abort)
{
    int x1, x2, z1, z2;
    int rx1, rx2, rz1, rz2, rx, rz;
    Pos pc;
    StructureConfig sconf;
    int qual, valid;
    int xt, zt;
    int64_t s, r, rmin, rmax;
    Pos p[128];

    StructPos *sout = spos + cond->save;

    switch (cond->type)
    {
    case F_QH_IDEAL:
    case F_QH_CLASSIC:
    case F_QH_NORMAL:
    case F_QH_BARELY:
        sconf = mc <= MC_1_12 ? SWAMP_HUT_CONFIG_112 : SWAMP_HUT_CONFIG;
        qual = cond->type;

        if (cond->relative)
        {
            rx1 = ((cond->x1 << 9) + spos[cond->relative].cx) >> 9;
            rz1 = ((cond->z1 << 9) + spos[cond->relative].cz) >> 9;
            rx2 = ((cond->x2 << 9) + spos[cond->relative].cx) >> 9;
            rz2 = ((cond->z2 << 9) + spos[cond->relative].cz) >> 9;
        }
        else
        {
            rx1 = cond->x1;
            rz1 = cond->z1;
            rx2 = cond->x2;
            rz2 = cond->z2;
        }

        for (rz = rz1; rz <= rz2 && !*abort; rz++)
        {
            for (rx = rx1; rx <= rx2; rx++)
            {
                s = moveStructure(seed, -rx, -rz);
                if ( U(qhutQual((s + sconf.salt) & 0xfffff) >= qual) &&
                     U(isQuadBaseFeature24(sconf, s, 7,7,9)) )
                {
                    sout->sconf = sconf;
                    p[0] = getStructurePos(sconf, seed, rx+0, rz+0, 0);
                    p[1] = getStructurePos(sconf, seed, rx+0, rz+1, 0);
                    p[2] = getStructurePos(sconf, seed, rx+1, rz+0, 0);
                    p[3] = getStructurePos(sconf, seed, rx+1, rz+1, 0);
                    pc = getOptimalAfk(p, 7,7,9, 0);
                    sout->cx = pc.x;
                    sout->cz = pc.z;
                    return 1;
                }
            }
        }
        return 0;

    case F_QM_95:   qual = 58*58*4 * 95 / 100;  goto L_qm_any;
    case F_QM_90:   qual = 58*58*4 * 90 / 100;
L_qm_any:
        sconf = MONUMENT_CONFIG;

        if (cond->relative)
        {
            rx1 = ((cond->x1 << 9) + spos[cond->relative].cx) >> 9;
            rz1 = ((cond->z1 << 9) + spos[cond->relative].cz) >> 9;
            rx2 = ((cond->x2 << 9) + spos[cond->relative].cx) >> 9;
            rz2 = ((cond->z2 << 9) + spos[cond->relative].cz) >> 9;
        }
        else
        {
            rx1 = cond->x1;
            rz1 = cond->z1;
            rx2 = cond->x2;
            rz2 = cond->z2;
        }

        for (rz = rz1; rz <= rz2 && !*abort; rz++)
        {
            for (rx = rx1; rx <= rx2; rx++)
            {
                s = moveStructure(seed, -rx, -rz);
                if (qmonumentQual(s) >= qual)
                {
                    sout->sconf = sconf;
                    p[0] = getStructurePos(sconf, seed, rx+0, rz+0, 0);
                    p[1] = getStructurePos(sconf, seed, rx+0, rz+1, 0);
                    p[2] = getStructurePos(sconf, seed, rx+1, rz+0, 0);
                    p[3] = getStructurePos(sconf, seed, rx+1, rz+1, 0);
                    pc = getOptimalAfk(p, 58,23,58, 0);
                    sout->cx = pc.x;
                    sout->cz = pc.z;
                    return 1;
                }
            }
        }
        return 0;


    case F_DESERT:
        sconf = mc <= MC_1_12 ? DESERT_PYRAMID_CONFIG_112 : DESERT_PYRAMID_CONFIG;
        goto L_struct_any;
    case F_HUT:
        sconf = mc <= MC_1_12 ? SWAMP_HUT_CONFIG_112 : SWAMP_HUT_CONFIG;
        goto L_struct_any;
    case F_JUNGLE:
        sconf = mc <= MC_1_12 ? JUNGLE_PYRAMID_CONFIG_112 : JUNGLE_PYRAMID_CONFIG;
        goto L_struct_any;
    case F_IGLOO:
        sconf = mc <= MC_1_12 ? IGLOO_CONFIG_112 : IGLOO_CONFIG;
        goto L_struct_any;
    case F_MONUMENT:    sconf = MONUMENT_CONFIG;    goto L_struct_any;
    case F_VILLAGE:     sconf = VILLAGE_CONFIG;     goto L_struct_any;
    case F_OUTPOST:     sconf = OUTPOST_CONFIG;     goto L_struct_any;

L_struct_any:
        x1 = cond->x1;
        z1 = cond->z1;
        x2 = cond->x2;
        z2 = cond->z2;
        if (cond->relative)
        {
            x1 += spos[cond->relative].cx;
            z1 += spos[cond->relative].cz;
            x2 += spos[cond->relative].cx;
            z2 += spos[cond->relative].cz;
        }

        rx1 = x1 >> 9;
        rz1 = z1 >> 9;
        rx2 = x2 >> 9;
        rz2 = z2 >> 9;


        // TODO: warn if multistructure clusters are used as a positional
        // dependency (the centre can change based on biomes)

        sout->cx = xt = 0;
        sout->cz = zt = 0;
        qual = 0;

        // Note "<="
        for (rz = rz1; rz <= rz2 && !*abort; rz++)
        {
            for (rx = rx1; rx <= rx2; rx++)
            {
                pc = getStructurePos(sconf, seed, rx, rz, &valid);
                if (valid && pc.x >= x1 && pc.x <= x2 && pc.z >= z1 && pc.z <= z2)
                {
                    if (g && !isViableStructurePos(sconf.structType, mc, g, seed, pc.x, pc.z))
                        continue;

                    xt += pc.x;
                    zt += pc.z;

                    if (++qual >= cond->count)
                    {
                        sout->sconf = sconf;
                        sout->cx = xt / qual;
                        sout->cz = zt / qual;
                        return 1;
                    }
                }
            }
        }
        return 0;

    case F_SPAWN:
        // TODO: warn if spawn is used for relative positioning
        sout->cx = 0;
        sout->cz = 0;
        if (!g)
            return 1;

        x1 = cond->x1;
        z1 = cond->z1;
        x2 = cond->x2;
        z2 = cond->z2;
        if (cond->relative)
        {
            x1 += spos[cond->relative].cx;
            z1 += spos[cond->relative].cz;
            x2 += spos[cond->relative].cx;
            z2 += spos[cond->relative].cz;
        }
        applySeed(g, seed);
        if (*abort) return 0;
        pc = getSpawn(mc, g, NULL, seed);
        if (pc.x >= x1 && pc.x <= x2 && pc.z >= z1 && pc.z <= z2)
        {
            sout->cx = pc.x;
            sout->cz = pc.z;
            return 1;
        }
        return 0;

    case F_STRONGHOLD:
        x1 = cond->x1;
        z1 = cond->z1;
        x2 = cond->x2;
        z2 = cond->z2;
        if (cond->relative)
        {
            x1 += spos[cond->relative].cx;
            z1 += spos[cond->relative].cz;
            x2 += spos[cond->relative].cx;
            z2 += spos[cond->relative].cz;
        }
        // TODO: add option for looking for the first stronghold only (which would be faster)

        rx1 = abs(x1); rx2 = abs(x2);
        rz1 = abs(z1); rz2 = abs(z2);
        if (x1 <= 112 && x2 >= -112 && z1 <= 112 && z2 >= -112)
        {
            rmin = 0;
        }
        else
        {
            xt = (rx1 < rx2 ? rx1 : rx2) - 112;
            zt = (rz1 < rz2 ? rz1 : rz2) - 112;
            rmin = xt*xt + zt*zt;
        }
        xt = (rx1 > rx2 ? rx1 : rx2) + 112;
        zt = (rz1 > rz2 ? rz1 : rz2) + 112;
        rmax = xt*xt + zt*zt;

        // -MC_1_8 formula:
        // r = 640 + [0,1]*512 (+/-112)
        // MC_1_9+ formula:
        // r = 1408 + 3072*n + 1280*[0,1] (+/-112)

        if (mc < MC_1_9)
        {
            if (rmax < 640*640 || rmin > 1152*1152)
                return 0;
            r = 0;
            rmin = 640;
            rmax = 1152;
        }
        else
        {   // check if the area is entirely outside the radii ranges in which strongholds can generate
            if (rmax < 1408*1408)
                return 0;
            rmin = sqrt(rmin);
            rmax = sqrt(rmax);
            r = (rmax - 1408) / 3072;       // maximum relevant ring number
            if (rmax - rmin < 3072-1280)    // area does not span more than one ring
            {
                if (rmin > 1408+1280+3072*r)
                    return 0;               // area is between rings
            }
            rmin = 1408;
            rmax = 1408+1280;
        }
        // if we are only looking at the inner ring, we can check if the generation angles are suitable
        if (r == 0 && !isInnerRingOk(mc, seed, x1, z1, x2, z2, rmin, rmax))
            return 0;

        // pre-biome-checks complete, the area appears to line up with possible generation positions
        if (!g)
        {
            // TODO: warn if strongholds are used for relative positioning
            sout->cx = 0;
            sout->cz = 0;
            return 1;
        }
        else
        {
            StrongholdIter sh;
            initFirstStronghold(&sh, mc, seed);
            applySeed(g, seed);
            qual = 0;
            while (nextStronghold(&sh, g, NULL) > 0)
            {
                if (*abort || sh.ringnum > r)
                    break;

                if (sh.pos.x >= x1 && sh.pos.x <= x2 && sh.pos.z >= z1 && sh.pos.z <= z2)
                {
                    if (++qual >= cond->count)
                    {
                        sout->cx = sh.pos.x;
                        sout->cz = sh.pos.z;
                        return 1;
                    }
                }

                if (sh.ringnum == r && sh.ringidx+1 == sh.ringmax)
                    break;
            }
        }
        return 0;

        // TODO: burried treasure

    case F_TEMP:
        if (cond->relative)
        {
            rx1 = ((cond->x1 << 10) + spos[cond->relative].cx) >> 10;
            rz1 = ((cond->z1 << 10) + spos[cond->relative].cz) >> 10;
        }
        else
        {
            rx1 = cond->x1;
            rz1 = cond->z1;
        }
        sout->cx = rx1 << 10;
        sout->cz = rz1 << 10;
        if (!g) return 1;
        return hasAllTemps(g, seed, rx1, rz1);

    case F_BIOME:           s = 0; qual = L_VORONOI_ZOOM_1; goto L_biome_filter_any;
    case F_BIOME_4_RIVER:   s = 2; qual = L_RIVER_MIX_4;    goto L_biome_filter_any;
    case F_BIOME_16_SHORE:  s = 4; qual = L_SHORE_16;       goto L_biome_filter_any;
    case F_BIOME_64_RARE:   s = 6; qual = L_RARE_BIOME_64;  goto L_biome_filter_any;
    case F_BIOME_256_BIOME: s = 8; qual = L_BIOME_256;      goto L_biome_filter_any;

L_biome_filter_any:
        if (cond->relative)
        {
            rx1 = ((cond->x1 << s) + spos[cond->relative].cx) >> s;
            rz1 = ((cond->z1 << s) + spos[cond->relative].cz) >> s;
            rx2 = ((cond->x2 << s) + spos[cond->relative].cx) >> s;
            rz2 = ((cond->z2 << s) + spos[cond->relative].cz) >> s;
        }
        else
        {
            rx1 = cond->x1;
            rz1 = cond->z1;
            rx2 = cond->x2;
            rz2 = cond->z2;
        }
        sout->cx = ((rx1 + rx2) << s) >> 1;
        sout->cz = ((rx1 + rx2) << s) >> 1;
        if (!g) return 1;
        if (rx2 < rx1 || rz2 < rz1 || *abort) return 0;
        return checkForBiomes(g, qual, NULL, seed, rx1, rz1, rx2-rx1+1, rz2-rz1+1, cond->bfilter, 0) > 0;

    default:
        break;
    }

    return 1;
}



int64_t searchFamily(int64_t seedbuf[], int64_t s, int scnt, int mc,
        LayerStack *g, const Condition cond[], int ccnt, StructPos *spos, volatile bool *abort)
{
    const Condition *c, *ce = cond + ccnt;

    if (*abort)
        return 0;

    for (c = cond; c != ce; c++)
        if (!testCond(spos, s, c, mc, NULL, abort))
            return 0;

    for (c = cond; c != ce; c++)
        if (g_filterinfo.list[c->type].cat != CAT_48)
            break;

    int n = 0;
    while (scnt--)
    {
        for (const Condition *ct = c; ct != ce; ct++)
            if (!testCond(spos, s, ct, mc, g, abort))
                goto L_NEXT_SEED;
        seedbuf[n++] = s;
L_NEXT_SEED:;
        if (*abort)
            break;
        s += (1LL << 48);
    }

    return n;
}
