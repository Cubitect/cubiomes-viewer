#include "search.h"
#include "seedtables.h"
#include "mainwindow.h"
#include "cutil.h"

#include "cubiomes/quadbase.h"
#include "cubiomes/finders.h"

#include <QThread>
#include <QByteArray>
#include <QApplication>

#include <algorithm>

#define MULTIPLY_CHAR QChar(0xD7)

QString Condition::summary() const
{
    const FilterInfo& ft = g_filterinfo.list[type];
    QString s;
    if (meta & Condition::DISABLED)
        s = QString("#%1#").arg(save, 2, 10, QChar('0'));
    else
        s = QString("[%1]").arg(save, 2, 10, QChar('0'));

    if (type == 0)
    {
        s += " " + QApplication::translate("Filter", "Conditions");
        return s;
    }

    QString cnts = "";
    if (ft.count)
        cnts += MULTIPLY_CHAR + QString::number(count);
    if (skipref)
        cnts += "*";
    QString txts = "";
    if (text[0])
    {
        QByteArray txta = QByteArray(text, sizeof(text));
        txts = QString::fromLocal8Bit(txta);
    }
    else
    {
        txts = QApplication::translate("Filter", ft.name);
    }

    s += QString(" %2%3").arg(txts, -28, ' ').arg(cnts, -4, QChar(' '));

    if (relative)
        s += QString::asprintf("[%02d]+", relative);
    else
        s += "     ";

    if (rmax > 0)
    {
        s += QString::asprintf("r<%d", rmax-1);
    }
    else
    {
        if (ft.coord)
            s += QString::asprintf("(%d,%d)", x1*ft.step, z1*ft.step);
        if (ft.area)
            s += QString::asprintf(",(%d,%d)", (x2+1)*ft.step-1, (z2+1)*ft.step-1);
    }
    return s;
}

bool Condition::versionUpgrade()
{
    if (version == VER_LEGACY)
    {
        uint64_t oceanToFind;
        memcpy(&oceanToFind, &biomeId, sizeof(oceanToFind));
        biomeToFind &= ~((1ULL << ocean) | (1ULL << deep_ocean));
        biomeToFind |= oceanToFind;
        skipref = 0;
        memset(pad0, 0, sizeof(pad0));
        memset(text, 0, sizeof(text));
        memset(pad1, 0, sizeof(pad1));
        memset(pad2, 0, sizeof(pad2));
        memset(pad3, 0, sizeof(pad3));
        biomeId = biomeSize = tol = 0;
        varflags = varbiome = varstart = 0;
    }
    else if (version == VER_2_3_0)
    {
        varflags = varbiome = varstart = 0;
    }
    version = VER_CURRENT;
    return true;
}

bool Condition::apply(WorldInfo wi)
{
    int in[256] = {}, inlen = 0, ex[256] = {}, exlen = 0;
    for (int i = 0; i < 64; i++)
    {
        if (biomeToFind & (1ULL << i))
            in[inlen++] = i;
        if (biomeToFindM & (1ULL << i))
            in[inlen++] = i + 128;
        if (biomeToExcl & (1ULL << i))
            ex[exlen++] = i;
        if (biomeToExclM & (1ULL << i))
            ex[exlen++] = i + 128;
    }
    uint32_t bfflags = BF_FORCED_OCEAN;
    if (flags & APPROX)
        bfflags |= BF_APPROX;
    if (flags & MATCH_ANY)
        setupBiomeFilter(&bf, wi.mc, bfflags, 0, 0, ex, exlen, in, inlen);
    else
        setupBiomeFilter(&bf, wi.mc, bfflags, in, inlen, ex, exlen, 0, 0);
    return true;
}

QString Condition::toHex() const
{
    size_t savsize = offsetof(Condition, generated_start);
    return QByteArray((const char*) this, savsize).toHex();
}

bool Condition::readHex(const QString& hex)
{
    if ((size_t)hex.length()/2 < offsetof(Condition, count))
        return false;
    QByteArray ba = QByteArray::fromHex(QByteArray(hex.toLocal8Bit().data()));
    size_t minsize = (size_t)ba.size();
    size_t savsize = offsetof(Condition, generated_start);
    if (savsize < minsize)
        minsize = savsize;
    memset(this, 0, sizeof(Condition));
    memcpy(this, ba.data(), minsize);
    bool ok = (size_t)ba.size() >= minsize &&
            save >= 0 && save < 100 &&
            type >= 0 && type < FILTER_MAX;
    if (ok)
        ok = versionUpgrade();
    return ok;
}

void ConditionTree::set(const QVector<Condition>& cv, WorldInfo wi)
{
    int cmax = 0;
    for (const Condition& c : cv)
        if (!(c.meta & Condition::DISABLED) && c.save > cmax)
            cmax = c.save;
    condvec.clear();
    condvec.resize(cmax + 1);
    references.clear();
    references.resize(cmax + 1);
    for (const Condition& c : cv)
    {
        if (c.meta & Condition::DISABLED)
            continue;
        condvec[c.save] = c;
        condvec[c.save].apply(wi);
        if (c.relative <= cmax)
            references[c.relative].push_back(c.save);
    }
}

static
int testTreeAt(
    Pos                         at,
    ConditionTree             * tree,
    int                         node,
    int                         pass,
    WorldGen                  * gen,
    std::atomic_bool          * abort,
    Pos                       * path
)
{
    Condition& c = tree->condvec[node];
    const std::vector<char>& branches = tree->references[c.save];
    int st;
    int rx1, rz1, rx2, rz2;
    int sref;
    Pos pos;
    Pos inst[MAX_INSTANCES];
    const FilterInfo *finfo;

    switch (c.type)
    {
    case F_REFERENCE_1:     sref = 0;  goto L_ref_pow2;
    case F_REFERENCE_4:     sref = 2;  goto L_ref_pow2;
    case F_REFERENCE_16:    sref = 4;  goto L_ref_pow2;
    case F_REFERENCE_64:    sref = 6;  goto L_ref_pow2;
    case F_REFERENCE_256:   sref = 8;  goto L_ref_pow2;
    case F_REFERENCE_512:   sref = 9;  goto L_ref_pow2;
    case F_REFERENCE_1024:  sref = 10; goto L_ref_pow2;
    L_ref_pow2:
        rx1 = ((c.x1 << sref) + at.x) >> sref;
        rz1 = ((c.z1 << sref) + at.z) >> sref;
        rx2 = ((c.x2 << sref) + at.x) >> sref;
        rz2 = ((c.z2 << sref) + at.z) >> sref;
        st = COND_FAILED;
        for (int z = rz1; z <= rz2; z++)
        {
            for (int x = rx1; x <= rx2; x++)
            {
                pos.x = (x << sref);
                pos.z = (z << sref);

                // children are combined via AND
                int sta = COND_OK;
                for (int b : branches)
                {
                    int stb = testTreeAt(
                        pos,
                        tree,
                        b,
                        pass,
                        gen,
                        abort,
                        path
                    );
                    if (*abort)
                        return COND_FAILED;
                    if (stb < sta)
                        sta = stb;
                    if (sta == COND_FAILED)
                        break;
                }

                if (sta > st)
                    st = sta;
                if (st == COND_OK)
                {
                    if (path)
                        path[c.save] = pos;
                    return COND_OK;
                }
            }
        }
        return st;


    case F_SCALE_TO_NETHER:
        pos.x = at.x / 8;
        pos.z = at.z / 8;
        goto L_scaled_to_dim;
    case F_SCALE_TO_OVERWORLD:
        pos.x = at.x * 8;
        pos.z = at.z * 8;
        goto L_scaled_to_dim;

    L_scaled_to_dim:
        st = COND_OK;
        for (int b : branches)
        {
            int sta = testTreeAt(
                pos,
                tree,
                b,
                pass,
                gen,
                abort,
                path
            );
            if (*abort)
                return COND_FAILED;
            if (sta < st)
                st = sta;
            if (st == COND_FAILED)
                break;
        }
        if (path && st == COND_OK)
            path[c.save] = pos;
        return st;


    case F_LOGIC_OR:
        if (branches.empty())
        {
            if (path)
                path[c.save].x = path[c.save].z = -1;
            return COND_OK; // empty ORs are ignored
        }
        else
        {
            int bok = 0;
            st = COND_FAILED;
            for (int b : branches)
            {
                int sta = testTreeAt(
                    at,
                    tree,
                    b,
                    pass,
                    gen,
                    abort,
                    path
                );
                if (*abort)
                    return COND_FAILED;
                if (sta > st)
                    st = sta;
                if (st == COND_OK) {
                    bok = b;
                    break;
                }
            }
            if (path && st == COND_OK)
            {
                path[c.save] = at;
                for (int b : branches)
                {   // invalidate the other branches
                    if (b == bok)
                        continue;
                    Pos *p = path + tree->condvec[b].save;
                    p->x = p->z = -1;
                }
            }
        }
        return st;


    case F_LOGIC_NOT:
        st = COND_OK;
        for (int b : branches)
        {
            int sta = testTreeAt(
                at,
                tree,
                b,
                pass,
                gen,
                abort,
                path
            );
            if (*abort)
                return COND_FAILED;
            if      (sta == COND_OK) { st = COND_FAILED; break; }
            else if (sta == COND_FAILED) { st = COND_OK; break; }
            else if (sta > st) st = sta;
        }
        return st;

    default:

        if (branches.empty())
        {   // this is a leaf node => check only for presence of instances
            int icnt = c.count;
            st = testCondAt(at, inst, &icnt, &c, pass, gen, abort);
            if (path && st == COND_OK)
            {
                if (icnt == 1)
                    path[c.save] = *inst;
                else
                    path[c.save].x = path[c.save].z = -1;
            }
            return st;
        }
        finfo = g_filterinfo.list + c.type;
        if (c.count == 1 && (finfo->count || finfo->cat == CAT_QUAD))
        {   // condition has exactly one required instance so we can check each
            // of the found instances individually, i.e. this branch splits the
            // instances into independent subbranches (combined via OR)
            // quad conditions are also processed here since we want to
            // examine all instances without support for averaging
            int icnt = MAX_INSTANCES;
            st = testCondAt(at, inst, &icnt, &c, pass, gen, abort);
            if (st == COND_FAILED || st == COND_MAYBE_POS_INVAL)
                return st;
            int sta = COND_FAILED;
            int iok = 0;
            for (int i = 0; i < icnt; i++) // OR instance subbranches
            {
                int stb = COND_OK;
                pos = inst[i];
                for (int b : branches) // AND dependent conditions
                {
                    int stc = testTreeAt(
                        pos,
                        tree,
                        b,
                        pass,
                        gen,
                        abort,
                        path
                    );
                    if (*abort)
                        return COND_FAILED;
                    // worst branch dictates status for instance
                    if (stc < stb)
                        stb = stc;
                    if (stb == COND_FAILED)
                        break;
                }
                // best instance dictates status
                if (stb > sta) {
                    sta = stb;
                    if (sta == COND_OK)
                        iok = i; // save position with ok path
                }
                if (sta > st)
                    break;
            }
            // status cannot be better than it was for this condition
            if (sta < st)
                st = sta;
            if (path && st == COND_OK)
                path[c.save] = inst[iok];
            return st;
        }
        else
        {   // this condition cannot branch, position of multiple instances
            // will be averaged to a center point
            if (c.type == 0)
            {   // this is the root condition
                st = COND_OK;
                pos = at;
            }
            else
            {
                st = testCondAt(at, inst, NULL, &c, pass, gen, abort);
                if (st == COND_FAILED || st == COND_MAYBE_POS_INVAL)
                    return st;
                pos = inst[0]; // center point of instances
            }
            for (char b : branches)
            {
                if (st == COND_FAILED)
                    break;
                int sta = testTreeAt(
                    pos,
                    tree,
                    b,
                    pass,
                    gen,
                    abort,
                    path
                );
                if (*abort)
                    return COND_FAILED;
                if (sta < st)
                    st = sta;
            }
            if (path && st == COND_OK)
                path[c.save] = pos;
            return st;
        }
    }
}

int testTreeAt(
    Pos                         at,
    ConditionTree             * tree,
    int                         pass,
    WorldGen                  * gen,
    std::atomic_bool          * abort,
    Pos                       * path
)
{
    if (pass != PASS_FAST_48)
    {   // do a fast check first
        int st = testTreeAt(at, tree, 0, PASS_FAST_48, gen, abort, NULL);
        if (st == COND_FAILED)
            return st;
    }
    return testTreeAt(at, tree, 0, pass, gen, abort, path);
}


static const int g_qh_c_n = sizeof(low20QuadHutBarely) / sizeof(uint64_t);
static QuadInfo qh_constellations[g_qh_c_n];

// initialize global tables
void _init(void) __attribute__((constructor));
void _init(void)
{
    int st = Swamp_Hut;
    StructureConfig sc;
    getStructureConfig(st, MC_NEWEST, &sc);
    sc.salt = 0; // ignore version dependent salt offsets

    for (int i = 0; i < g_qh_c_n; i++)
    {
        uint64_t b = low20QuadHutBarely[i];
        for (uint64_t s = b;; s += 0x100000)
        {
            QuadInfo *qi = &qh_constellations[i];
            Pos pc;
            if (scanForQuads(sc, 128, s, low20QuadHutBarely, g_qh_c_n,
                    20, 0, 0, 0, 1, 1, &pc, 1) < 1)
                continue;
            if ( !(qi->rad = isQuadBase(sc, s, 160)) )
                continue;

            qi->c = b;
            qi->p[0] = getFeaturePos(sc, s, 0, 0);
            qi->p[1] = getFeaturePos(sc, s, 0, 1);
            qi->p[2] = getFeaturePos(sc, s, 1, 0);
            qi->p[3] = getFeaturePos(sc, s, 1, 1);
            qi->afk = getOptimalAfk(qi->p, 7,7,9, &qi->spcnt);
            qi->typ = st;

            qi->flt = F_QH_BARELY;
            int j, n;
            n = sizeof(low20QuadHutNormal) / sizeof(uint64_t);
            for (j = 0; j < n; j++) {
                if (low20QuadHutNormal[j] == b) {
                    qi->flt = F_QH_NORMAL;
                    break;
                }
            }
            n = sizeof(low20QuadClassic) / sizeof(uint64_t);
            for (j = 0; j < n; j++) {
                if (low20QuadClassic[j] == b) {
                    qi->flt = F_QH_CLASSIC;
                    break;
                }
            }
            n = sizeof(low20QuadIdeal) / sizeof(uint64_t);
            for (j = 0; j < n; j++) {
                if (low20QuadIdeal[j] == b) {
                    qi->flt = F_QH_IDEAL;
                    break;
                }
            }
            break;
        }
    }
}

static bool isVariantOk(const Condition *c, WorldGen *g, int stype, int varbiome, Pos *pos)
{
    StructureVariant sv;

    if (stype == Village)
    {
        if (g->mc < MC_1_10) return true;
        getVariant(&sv, stype, g->mc, g->seed, pos->x, pos->z, varbiome);
        if (c->varflags & Condition::VAR_ABANODONED)
        {
            if ((c->varflags & Condition::VAR_NOT) && sv.abandoned)
                return false;
            if (!(c->varflags & Condition::VAR_NOT) && !sv.abandoned)
                return false;
        }
        if (!(c->varflags & Condition::VAR_WITH_START) || g->mc < MC_1_14) return true;
    }
    else if (stype == Bastion)
    {
        if (g->mc < MC_1_16) return true;
        getVariant(&sv, stype, g->mc, g->seed, pos->x, pos->z, -1);
        if (!(c->varflags & Condition::VAR_WITH_START)) return true;
    }
    else if (stype == Ruined_Portal || stype == Ruined_Portal_N)
    {
        if (g->mc < MC_1_16) return true;
        g->init4Dim(stype == Ruined_Portal ? DIM_OVERWORLD : DIM_NETHER);
        varbiome = getBiomeAt(&g->g, 4, (pos->x >> 2) + 2, 0, (pos->z >> 2) + 2);
        getVariant(&sv, stype, g->mc, g->seed, pos->x, pos->z, varbiome);
        if (!(c->varflags & Condition::VAR_WITH_START)) return true;
    }
    else if (stype == End_City)
    {
        if (!(c->varflags & Condition::VAR_ENDSHIP)) return true;
        Piece pieces[END_CITY_PIECES_MAX];
        int i, n = getEndCityPieces(pieces, g->seed, pos->x >> 4, pos->z >> 4);
        bool withship = !(c->varflags & Condition::VAR_NOT);
        for (i = 0; i < n; i++)
            if (pieces[i].type == END_SHIP)
                return withship;
        return !withship;
    }
    else if (stype == Fortress)
    {
        if (!(c->varflags & Condition::VAR_DENSE_BB)) return true;
        enum { FP_MAX = 400 };
        Piece p[FP_MAX];
        int i, n, b;
        n = getFortressPieces(p, FP_MAX, g->mc, g->seed, pos->x >> 4, pos->z >> 4);
        for (b = i = 0; i < n; i++)
            if (p[i].type == FORTRESS_START || p[i].type == BRIDGE_CROSSING)
                p[b++] = p[i];
        if (b < 4) return false;
        for (i = 0; i < b; i++)
        {
            int j, adj = 0;
            for (j = 0; j < b; j++)
            {
                if (p[i].bb0.y != p[j].bb0.y) continue;
                if (p[i].bb1.x != p[j].bb1.x && p[i].bb1.x+1 != p[j].bb0.x) continue;
                if (p[i].bb1.z != p[j].bb1.z && p[i].bb1.z+1 != p[j].bb0.z) continue;
                adj++;
            }
            if (adj >= 4)
            {
                pos->x = p[i].bb1.x;
                pos->z = p[i].bb1.z;
                return true;
            }
        }
        return false;
    }
    else
    {
        return true;
    }

    // check start piece
    uint64_t idxbits = c->varstart;
    while (idxbits)
    {
        int idx = __builtin_ctzll(idxbits);
        idxbits &= idxbits - 1;
        if ((size_t)idx < sizeof(g_start_pieces) / sizeof(g_start_pieces[0]))
        {
            const StartPiece *sp = g_start_pieces + idx;
            if (sp->stype == stype && sp->start == sv.start)
            {
                if (sp->biome != -1 && sp->biome != sv.biome)
                    continue;
                if (sp->giant != -1 && sp->giant != sv.giant)
                    continue;
                return true;
            }
        }
    }
    return false;
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

static bool isInnerRingOk(int mc, uint64_t seed, int x1, int z1, int x2, int z2, int r1, int r2)
{
    StrongholdIter sh;
    Pos p = initFirstStronghold(&sh, mc, seed);

    if (p.x >= x1 && p.x <= x2 && p.z >= z1 && p.z <= z2)
        return true;
    // Do a ray cast analysis, checking if any of the generation angles intersect the area.
    double c, s;
    c = cos(sh.angle + M_PI*2/3);
    s = sin(sh.angle + M_PI*2/3);
    if (intersectRectLine(x1, z1, x2, z2, c*r1, s*r1, c*r2, s*r2))
        return true;
    c = cos(sh.angle + M_PI*4/3);
    s = sin(sh.angle + M_PI*4/3);
    if (intersectRectLine(x1, z1, x2, z2, c*r1, s*r1, c*r2, s*r2))
        return true;

    return false;
}

static int f_confine(void *data, int x, int z, double p)
{
    (void) x; (void) z;
    double *lim = (double*) data;
    return p < lim[0] || p > lim[1];
}


/* Tests if a condition is satisfied with 'at' as origin for a search pass.
 * If sufficiently satisfied (check return value) then:
 * when 'imax' is NULL, the center position is written to 'cent[0]'
 * otherwise a maximum number of '*imax' instance positions are stored in 'cent'
 * and '*imax' is overwritten with the number of found instances.
 * ('*imax' should be at most MAX_INSTANCES)
 */
int
testCondAt(
    Pos                 at,     // relative origin
    Pos               * cent,   // output center position(s)
    int               * imax,   // max instances (NULL for avg)
    Condition         * cond,   // condition to check
    int                 pass,
    WorldGen          * gen,
    std::atomic_bool  * abort
    )
{
    int x1, x2, z1, z2;
    int rx1, rx2, rz1, rz2, rx, rz;
    Pos pc;
    StructureConfig sconf;
    int qual, valid;
    int xt, zt;
    int st;
    int i, j, n, icnt;
    int64_t s, r, rmin, rmax;
    const uint64_t *seeds;
    Pos p[MAX_INSTANCES];

    const FilterInfo& finfo = g_filterinfo.list[cond->type];

    if ((st = finfo.stype) > 0)
    {
        if (!getStructureConfig_override(finfo.stype, gen->mc, &sconf))
            return COND_FAILED;
    }

    switch (cond->type)
    {
    case F_REFERENCE_1:
    case F_REFERENCE_4:
    case F_REFERENCE_16:
    case F_REFERENCE_64:
    case F_REFERENCE_256:
    case F_REFERENCE_512:
    case F_REFERENCE_1024:
    case F_SCALE_TO_NETHER:
    case F_SCALE_TO_OVERWORLD:
    case F_LOGIC_OR:
        // helper conditions should not reach here
        //exit(1);
        return COND_OK;

    case F_QH_IDEAL:
        seeds = low20QuadIdeal;
        n = sizeof(low20QuadIdeal) / sizeof(uint64_t);
        goto L_qh_any;
    case F_QH_CLASSIC:
        seeds = low20QuadClassic;
        n = sizeof(low20QuadClassic) / sizeof(uint64_t);
        goto L_qh_any;
    case F_QH_NORMAL:
        seeds = low20QuadHutNormal;
        n = sizeof(low20QuadHutNormal) / sizeof(uint64_t);
        goto L_qh_any;
    case F_QH_BARELY:
        seeds = low20QuadHutBarely;
        n = sizeof(low20QuadHutBarely) / sizeof(uint64_t);

L_qh_any:
        rx1 = ((cond->x1 << 9) + at.x) >> 9;
        rz1 = ((cond->z1 << 9) + at.z) >> 9;
        rx2 = ((cond->x2 << 9) + at.x) >> 9;
        rz2 = ((cond->z2 << 9) + at.z) >> 9;

        n = scanForQuads(
                sconf, 128, (gen->seed) & MASK48, seeds, n, 20, sconf.salt,
                rx1, rz1, rx2 - rx1 + 1, rz2 - rz1 + 1, p, MAX_INSTANCES);
        if (n < 1)
            return COND_FAILED;
        icnt = 0;
        for (i = 0; i < n; i++)
        {
            pc = p[i];
            s = moveStructure(gen->seed, -pc.x, -pc.z);

            // find the constellation info
            uint64_t cst = (s + sconf.salt) & 0xfffff;
            QuadInfo *qi = NULL;
            for (j = 0; j < g_qh_c_n; j++)
            {
                if (qh_constellations[j].c == cst)
                {
                    qi = &qh_constellations[j];
                    break;
                }
            }
            if (!qi || qi->flt > cond->type)
                continue;
            // we don't support finding the center of multiple
            // quad huts, instead we just return the first one
            // (unless we are looking for all instances)
            cent[icnt].x = (pc.x << 9) + qi->afk.x;
            cent[icnt].z = (pc.z << 9) + qi->afk.z;
            icnt++;
            if (imax && icnt >= *imax)
                return COND_OK;
            if (imax == NULL)
                break;
        }
        if (imax)
            *imax = icnt;
        if (icnt > 0)
            return COND_OK;
        return COND_FAILED;

    case F_QM_95:   qual = 58*58*4 * 95 / 100;  goto L_qm_any;
    case F_QM_90:   qual = 58*58*4 * 90 / 100;
L_qm_any:

        rx1 = ((cond->x1 << 9) + at.x) >> 9;
        rz1 = ((cond->z1 << 9) + at.z) >> 9;
        rx2 = ((cond->x2 << 9) + at.x) >> 9;
        rz2 = ((cond->z2 << 9) + at.z) >> 9;
        // we don't really need to check for more than one instance here
        n = scanForQuads(
                sconf, 160, (gen->seed) & MASK48, g_qm_90,
                sizeof(g_qm_90) / sizeof(uint64_t), 48, sconf.salt,
                rx1, rz1, rx2 - rx1 + 1, rz2 - rz1 + 1, p, 1);
        if (n < 1)
            return COND_FAILED;
        icnt = 0;
        for (i = 0; i < n; i++)
        {
            rx = p[i].x; rz = p[i].z;
            s = moveStructure(gen->seed, -rx, -rz);
            if (qmonumentQual(s + sconf.salt) >= qual)
            {
                getStructurePos(st, gen->mc, gen->seed, rx+0, rz+0, p+0);
                getStructurePos(st, gen->mc, gen->seed, rx+0, rz+1, p+1);
                getStructurePos(st, gen->mc, gen->seed, rx+1, rz+0, p+2);
                getStructurePos(st, gen->mc, gen->seed, rx+1, rz+1, p+3);
                pc = getOptimalAfk(p, 58,23,58, 0);
                pc.x -= 29; // monument is centered
                pc.z -= 29;
                cent[icnt] = pc;
                icnt++;
                if (imax && icnt >= *imax)
                    return COND_OK;
                if (imax == NULL)
                    break;
            }
        }
        if (imax)
            *imax = icnt;
        if (icnt > 0)
            return COND_OK;
        return COND_FAILED;


    case F_DESERT:
    case F_HUT:
    case F_JUNGLE:
    case F_IGLOO:
    case F_MONUMENT:
    case F_VILLAGE:
    case F_OUTPOST:
    case F_MANSION:
    case F_RUINS:
    case F_SHIPWRECK:
    case F_TREASURE:
    case F_PORTAL:
    case F_PORTALN:
    case F_ANCIENT_CITY:

    case F_FORTRESS:
    case F_BASTION:

    case F_ENDCITY:
    case F_GATEWAY:

        if (cond->rmax > 0)
        {
            rmax = (cond->rmax-1) * (cond->rmax-1) + 1;
            x1 = at.x - cond->rmax;
            z1 = at.z - cond->rmax;
            x2 = at.x + cond->rmax;
            z2 = at.z + cond->rmax;
        }
        else
        {
            rmax = 0;
            x1 = cond->x1 + at.x;
            z1 = cond->z1 + at.z;
            x2 = cond->x2 + at.x;
            z2 = cond->z2 + at.z;
        }

        if (sconf.regionSize == 32)
        {
            rx1 = x1 >> 9;
            rz1 = z1 >> 9;
            rx2 = x2 >> 9;
            rz2 = z2 >> 9;
        }
        else if (sconf.regionSize == 1)
        {
            rx1 = x1 >> 4;
            rz1 = z1 >> 4;
            rx2 = x2 >> 4;
            rz2 = z2 >> 4;
        }
        else
        {
            rx1 = (x1 / (sconf.regionSize << 4)) - (x1 < 0);
            rz1 = (z1 / (sconf.regionSize << 4)) - (z1 < 0);
            rx2 = (x2 / (sconf.regionSize << 4)) - (x2 < 0);
            rz2 = (z2 / (sconf.regionSize << 4)) - (z2 < 0);
        }

        cent->x = xt = 0;
        cent->z = zt = 0;
        icnt = 0;

        // Note "<="
        for (rz = rz1; rz <= rz2 && !*abort; rz++)
        {
            for (rx = rx1; rx <= rx2; rx++)
            {
                if (!getStructurePos(st, gen->mc, gen->seed, rx+0, rz+0, &pc))
                    continue;
                if (cond->skipref && pc.x == at.x && pc.z == at.z)
                    continue;
                if (rmax)
                {
                    int dx = pc.x - at.x;
                    int dz = pc.z - at.z;
                    int64_t rsq = dx*(int64_t)dx + dz*(int64_t)dz;
                    if (rsq >= rmax)
                        continue;
                }
                else if (pc.x < x1 || pc.x > x2 || pc.z < z1 || pc.z > z2)
                {
                    continue;
                }
                if (pass == PASS_FULL_64 || (pass == PASS_FULL_48 && !finfo.dep64))
                {
                    if (*abort) return COND_FAILED;

                    if (st == Village && cond->varflags)
                    {   // we can test for abandoned villages before the
                        // biome checks by trying each suitable biome
                        int vv[] = {
                            plains, desert, savanna, taiga, snowy_tundra,
                            // plains village variant covers meadows
                        };
                        int vn = gen->mc <= MC_1_13 ? 1 : sizeof(vv) / sizeof(int);
                        int i;
                        for (i = 0; i < vn; i++)
                            if (isVariantOk(cond, gen, st, vv[i], &pc))
                                break;
                        if (i >= vn) // no suitable village variants here
                            continue;
                    }

                    gen->init4Dim(finfo.dim);
                    int id = isViableStructurePos(st, &gen->g, pc.x, pc.z, 0);
                    if (!id)
                        continue;
                    if (st == End_City)
                    {
                        gen->setSurfaceNoise();
                        if (!isViableEndCityTerrain(
                            &gen->g.en, &gen->sn, pc.x, pc.z))
                            continue;
                    }
                    if (cond->varflags)
                    {
                        if (!isVariantOk(cond, gen, st, id, &pc))
                            continue;
                    }
                    if (gen->mc >= MC_1_18)
                    {
                        if (g_extgen.estimateTerrain &&
                            !isViableStructureTerrain(st, &gen->g, pc.x, pc.z))
                        {
                            continue;
                        }
                    }
                }

                icnt++;
                if (imax == NULL)
                {
                    xt += pc.x;
                    zt += pc.z;
                }
                else if (*imax)
                {
                    cent[icnt-1] = pc;
                    if (icnt >= *imax)
                        return COND_OK;
                }
                else
                {
                    goto L_struct_failed_exclusion;
                }
            }
        }
        if (cond->count == 0)
        {   // structure exclusion filter
    L_struct_failed_exclusion:
            cent->x = (x1 + x2) >> 1;
            cent->z = (z1 + z2) >> 1;
            if (imax) *imax = 1;
            if (icnt == 0)
                return COND_OK;
            else
            {
                if (pass == PASS_FULL_64)
                    return COND_FAILED;
                if (pass == PASS_FULL_48 && !finfo.dep64)
                    return COND_FAILED;
                return COND_MAYBE_POS_VALID;
            }
        }
        else if (icnt >= cond->count)
        {
            if (imax)
            {
                *imax = icnt;
                return COND_OK;
            }
            else
            {
                cent->x = xt / icnt;
                cent->z = zt / icnt;
            }

            if (pass == PASS_FULL_64)
                return COND_OK;
            if (pass == PASS_FULL_48 && !finfo.dep64)
                return COND_OK;
            // some non-exhaustive structure clusters do not
            // have known center positions with 48-bit seeds
            if (cond->count != (1+rx2-rx1) * (1+rz2-rz1))
                return COND_MAYBE_POS_INVAL;
            return COND_MAYBE_POS_VALID;
        }
        return COND_FAILED;


    case F_MINESHAFT:

        if (cond->rmax > 0)
        {
            rmax = (cond->rmax-1) * (cond->rmax-1) + 1;
            x1 = at.x - cond->rmax;
            z1 = at.z - cond->rmax;
            x2 = at.x + cond->rmax;
            z2 = at.z + cond->rmax;
        }
        else
        {
            rmax = 0;
            x1 = cond->x1 + at.x;
            z1 = cond->z1 + at.z;
            x2 = cond->x2 + at.x;
            z2 = cond->z2 + at.z;
        }
        rx1 = x1 >> 4;
        rz1 = z1 >> 4;
        rx2 = x2 >> 4;
        rz2 = z2 >> 4;

        if (cond->count == 0)
        {   // exclusion
            icnt = getMineshafts(gen->mc, gen->seed, rx1, rz1, rx2, rz2, cent, 1);
            if (icnt == 1 && cond->skipref && cent->x == at.x && cent->z == at.z)
                icnt = 0;
            cent->x = (x1 + x2) >> 1;
            cent->z = (z1 + z2) >> 1;
            if (icnt == 0)
            {
                if (imax) *imax = 1;
                return COND_OK;
            }
        }
        else if (imax)
        {   // just check there are at least *inst (== cond->count) instances
            *imax = icnt =
                getMineshafts(gen->mc, gen->seed, rx1, rz1, rx2, rz2, cent, *imax);
            if (rmax)
            {   // filter out the instances that are outside the radius
                int j = 0;
                for (int i = 0; i < icnt; i++)
                {
                    int dx = cent[i].x - at.x;
                    int dz = cent[i].z - at.z;
                    int64_t rsq = dx*(int64_t)dx + dz*(int64_t)dz;
                    if (rsq < rmax)
                        cent[j++] = cent[i];
                }
                *imax = icnt = j;
            }
            if (cond->skipref && icnt > 0)
            {   // remove origin instance
                for (int i = 0; i < icnt; i++)
                {
                    if (cent[i].x == at.x && cent[i].z == at.z)
                    {
                        cent[i] = cent[icnt-1];
                        *imax = --icnt;
                        break;
                    }
                }
            }
            if (icnt >= cond->count)
                return COND_OK;
        }
        else
        {   // we need the average position of all instances
            icnt = getMineshafts(gen->mc, gen->seed, rx1, rz1, rx2, rz2, p, MAX_INSTANCES);
            if (icnt < cond->count)
                return COND_FAILED;
            xt = zt = 0;
            int j = 0;
            for (int i = 0; i < icnt; i++)
            {
                if (rmax)
                {   // skip instances outside the radius
                    int dx = cent[i].x - at.x;
                    int dz = cent[i].z - at.z;
                    int64_t rsq = dx*(int64_t)dx + dz*(int64_t)dz;
                    if (rsq >= rmax)
                        continue;
                }
                if (cond->skipref && p[i].x == at.x && p[i].z == at.z)
                    continue;
                xt += p[i].x;
                zt += p[i].z;
                j++;
            }
            if (j >= cond->count)
            {
                cent->x = xt / j;
                cent->z = zt / j;
                return COND_OK;
            }
        }
        return COND_FAILED;


    case F_SPAWN:

        cent->x = cent->z = 0;
        if (pass != PASS_FULL_64)
            return COND_MAYBE_POS_INVAL;

        if (cond->rmax > 0)
        {
            rmax = (cond->rmax-1) * (cond->rmax-1) + 1;
            x1 = at.x - cond->rmax;
            z1 = at.z - cond->rmax;
            x2 = at.x + cond->rmax;
            z2 = at.z + cond->rmax;
        }
        else
        {
            rmax = 0;
            x1 = cond->x1 + at.x;
            z1 = cond->z1 + at.z;
            x2 = cond->x2 + at.x;
            z2 = cond->z2 + at.z;
        }
        if (*abort) return COND_FAILED;
        gen->init4Dim(0);
        pc = getSpawn(&gen->g);
        if (rmax)
        {
            int dx = pc.x - at.x;
            int dz = pc.z - at.z;
            int64_t rsq = dx*(int64_t)dx + dz*(int64_t)dz;
            if (rsq >= rmax)
                return COND_FAILED;
        }
        else if (pc.x < x1 || pc.x > x2 || pc.z < z1 || pc.z > z2)
        {
            return COND_FAILED;
        }

        if (cond->skipref && pc.x == at.x && pc.z == at.z)
            return COND_FAILED;
        *cent = pc;
        return COND_OK;


    case F_FIRST_STRONGHOLD:
        {
            StrongholdIter sh;
            *cent = pc = initFirstStronghold(&sh, gen->mc, gen->seed);
        }
        if (cond->rmax > 0)
        {
            uint64_t rsqmax = (cond->rmax-1) * (cond->rmax-1);
            int dx = pc.x - at.x;
            int dz = pc.z - at.z;
            uint64_t rsq = dx*(int64_t)dx + dz*(int64_t)dz;
            if (rsq > rsqmax)
                return COND_FAILED;
        }
        else
        {
            x1 = cond->x1 + at.x;
            z1 = cond->z1 + at.z;
            x2 = cond->x2 + at.x;
            z2 = cond->z2 + at.z;
            if (pc.x < x1 || pc.x > x2 || pc.z < z1 || pc.z > z2)
                return COND_FAILED;
        }
        if (cond->skipref && pc.x == at.x && pc.z == at.z)
            return COND_FAILED;
        return COND_OK;


    case F_STRONGHOLD:

        // the position is rounded to the nearest chunk and then centered on (8,8)
        // for the pre-selection we will subtract this offset
        if (cond->rmax > 0)
        {
            x1 = at.x - cond->rmax - 8;
            z1 = at.z - cond->rmax - 8;
            x2 = at.x + cond->rmax - 8;
            z2 = at.z + cond->rmax - 8;
        }
        else
        {
            x1 = cond->x1 + at.x - 8;
            z1 = cond->z1 + at.z - 8;
            x2 = cond->x2 + at.x - 8;
            z2 = cond->z2 + at.z - 8;
        }
        rx1 = abs(x1); rx2 = abs(x2);
        rz1 = abs(z1); rz2 = abs(z2);
        // lets treat the final (+/-112) blocks as (+/-120) to account for chunk rounding
        if (x1 <= 112+8 && x2 >= -112-8 && z1 <= 112+8 && z2 >= -112-8)
        {
            rmin = 0;
        }
        else
        {
            xt = (rx1 < rx2 ? rx1 : rx2) - 112-8;
            zt = (rz1 < rz2 ? rz1 : rz2) - 112-8;
            rmin = xt*xt + zt*zt;
        }
        xt = (rx1 > rx2 ? rx1 : rx2) + 112+8;
        zt = (rz1 > rz2 ? rz1 : rz2) + 112+8;
        rmax = xt*xt + zt*zt;

        // undo (8,8) offset
        x1 += 8; z1 += 8;
        x2 += 8; z2 += 8;
        cent->x = (x1 + x2) >> 1;
        cent->z = (z1 + z2) >> 1;

        // -MC_1_8 formula:
        // r = 640 + [0,1]*512 (+/-112)
        // MC_1_9+ formula:
        // r = 1408 + 3072*n + 1280*[0,1] (+/-112)

        if (gen->mc < MC_1_9)
        {
            if (rmax < 640*640 || rmin > 1152*1152)
                return cond->count == 0 ? COND_OK : COND_FAILED;
            r = 0;
            rmin = 640;
            rmax = 1152;
        }
        else
        {   // check if the area is entirely outside the radii ranges in which strongholds can generate
            if (rmax < 1408*1408)
                return cond->count == 0 ? COND_OK : COND_FAILED;
            rmin = sqrt(rmin);
            rmax = sqrt(rmax);
            r = (rmax - 1408) / 3072;       // maximum relevant ring number
            if (rmax - rmin < 3072-1280)    // area does not span more than one ring
            {
                if (rmin > 1408+1280+3072*r)// area is between rings
                    return cond->count == 0 ? COND_OK : COND_FAILED;
            }
            rmin = 1408;
            rmax = 1408+1280;
        }
        // if we are only looking at the inner ring, we can check if the generation angles are suitable
        if (r == 0 && !isInnerRingOk(gen->mc, gen->seed, x1-112-8, z1-112-8, x2+112+8, z2+112+8, rmin, rmax))
            return cond->count == 0 ? COND_OK : COND_FAILED;

        // pre-biome-checks complete, the area appears to line up with possible generation positions
        if (pass != PASS_FULL_64)
        {
            return COND_MAYBE_POS_INVAL;
        }
        else
        {
            if (cond->rmax > 0)
                rmax = (cond->rmax-1) * (cond->rmax-1) + 1;
            else
                rmax = 0;

            StrongholdIter sh;
            initFirstStronghold(&sh, gen->mc, gen->seed);
            icnt = 0;
            xt = zt = 0;
            gen->init4Dim(0);
            while (nextStronghold(&sh, &gen->g) > 0)
            {
                if (*abort)
                    break;
                bool inside;
                if (rmax)
                {
                    int dx = sh.pos.x - at.x;
                    int dz = sh.pos.z - at.z;
                    int64_t rsq = dx*(int64_t)dx + dz*(int64_t)dz;
                    inside = (rsq < rmax);
                }
                else
                {
                    inside = (sh.pos.x >= x1 && sh.pos.x <= x2 &&
                              sh.pos.z >= z1 && sh.pos.z <= z2);
                }
                if (cond->skipref && sh.pos.x == at.x && sh.pos.z == at.z)
                    inside = false;
                if (inside)
                {
                    if (cond->count == 0)
                    {   // exclude
                        return COND_FAILED;
                    }
                    else if (imax)
                    {
                        cent[icnt] = sh.pos;
                        icnt++;
                        if (icnt >= *imax)
                            return COND_OK;
                    }
                    else
                    {
                        xt += sh.pos.x;
                        zt += sh.pos.z;
                        icnt++;
                    }
                }
                if (sh.ringnum > r)
                    break;
            }
            if (cond->count == 0)
            {   // exclusion
                if (imax) *imax = 1;
                return COND_OK;
            }
            else if (imax)
            {
                *imax = icnt;
            }
            else if (icnt)
            {
                cent->x = xt / icnt;
                cent->z = zt / icnt;
            }
            if (icnt >= cond->count)
                return COND_OK;
        }
        return COND_FAILED;


    case F_SLIME:

        rx1 = ((cond->x1 << 4) + at.x) >> 4;
        rz1 = ((cond->z1 << 4) + at.z) >> 4;
        rx2 = ((cond->x2 << 4) + at.x) >> 4;
        rz2 = ((cond->z2 << 4) + at.z) >> 4;

        icnt = 0;
        xt = zt = 0;
        for (int rz = rz1; rz <= rz2; rz++)
        {
            for (int rx = rx1; rx <= rx2; rx++)
            {
                if (cond->skipref && rx == at.x >> 4 && rz == at.z >> 4)
                    continue;
                if (isSlimeChunk(gen->seed, rx, rz))
                {
                    if (cond->count == 0)
                    {
                        return COND_FAILED;
                    }
                    else if (imax)
                    {
                        cent[icnt].x = rx << 4;
                        cent[icnt].z = rz << 4;
                        icnt++;
                        if (icnt >= *imax)
                            return COND_OK;
                    }
                    else
                    {
                        xt += rx;
                        zt += rz;
                        icnt++;
                    }
                }
            }
        }
        if (cond->count == 0)
        {   // exclusion filter
            cent->x = (rx1 + rx2) << 3;
            cent->z = (rz1 + rz2) << 3;
            if (imax) *imax = 1;
            return COND_OK;
        }
        else if (imax)
        {
            *imax = icnt;
        }
        else if (icnt)
        {
            cent->x = (xt << 4) / icnt;
            cent->z = (zt << 4) / icnt;
        }
        if (icnt >= cond->count)
            return COND_OK;
        return COND_FAILED;

    // biome filters reference specific layers
    // MAYBE: options for layers in different versions?
    case F_BIOME:
        if (gen->mc >= MC_1_18) goto L_noise_biome;
        // fallthrough
    case F_BIOME_4_RIVER:
    case F_BIOME_256_OTEMP:

        if (gen->mc >= MC_1_18)
            return COND_FAILED;
        s = finfo.pow2;
        rx1 = ((cond->x1 << s) + at.x) >> s;
        rz1 = ((cond->z1 << s) + at.z) >> s;
        rx2 = ((cond->x2 << s) + at.x) >> s;
        rz2 = ((cond->z2 << s) + at.z) >> s;
        cent->x = ((rx1 + rx2) << s) >> 1;
        cent->z = ((rz1 + rz2) << s) >> 1;
        if (pass == PASS_FAST_48)
            return COND_MAYBE_POS_VALID;
        if (pass == PASS_FULL_48)
        {
            if (gen->mc < MC_1_13 || finfo.layer != L_OCEAN_TEMP_256)
                return COND_MAYBE_POS_VALID;
        }
        valid = COND_FAILED;
        if (rx2 >= rx1 || rz2 >= rz1 || !*abort)
        {
            int w = rx2-rx1+1;
            int h = rz2-rz1+1;
            //gen->init4Dim(0); // seed gets applied by checkForBiomesAtLayer
            if (checkForBiomesAtLayer(&gen->g.ls, &gen->g.ls.layers[finfo.layer],
                NULL, gen->seed, rx1, rz1, w, h, &cond->bf) > 0)
            {
                valid = COND_OK;
            }
        }
        return valid;


    case F_TEMPS:
        if (gen->mc >= MC_1_18)
            return COND_FAILED;
        rx1 = ((cond->x1 << 10) + at.x) >> 10;
        rz1 = ((cond->z1 << 10) + at.z) >> 10;
        rx2 = ((cond->x2 << 10) + at.x) >> 10;
        rz2 = ((cond->z2 << 10) + at.z) >> 10;
        cent->x = ((rx1 + rx2) << 10) >> 1;
        cent->z = ((rz1 + rz2) << 10) >> 1;
        if (pass != PASS_FULL_64)
            return COND_MAYBE_POS_VALID;
        gen->init4Dim(0);
        if (checkForTemps(&gen->g.ls, gen->seed, rx1, rz1, rx2-rx1+1, rz2-rz1+1, cond->temps))
            return COND_OK;
        return COND_FAILED;


    case F_BIOME_4:
    case F_BIOME_16:
    case F_BIOME_64:
    case F_BIOME_256:
    case F_BIOME_NETHER_1:
    case F_BIOME_NETHER_4:
    case F_BIOME_NETHER_16:
    case F_BIOME_NETHER_64:
    case F_BIOME_END_1:
    case F_BIOME_END_4:
    case F_BIOME_END_16:
    case F_BIOME_END_64:

L_noise_biome:
        s = finfo.pow2;
        rx1 = ((cond->x1 << s) + at.x) >> s;
        rz1 = ((cond->z1 << s) + at.z) >> s;
        rx2 = ((cond->x2 << s) + at.x) >> s;
        rz2 = ((cond->z2 << s) + at.z) >> s;
        cent->x = ((rx1 + rx2) << s) >> 1;
        cent->z = ((rz1 + rz2) << s) >> 1;
        if (pass == PASS_FAST_48)
            return COND_MAYBE_POS_VALID;
        // the Nether and End require only the 48-bit seed
        // (except voronoi uses the full 64-bits)
        if (pass == PASS_FULL_48 && finfo.dep64)
            return COND_MAYBE_POS_VALID;
        {
            int w = rx2 - rx1 + 1;
            int h = rz2 - rz1 + 1;
            int y = (s == 0 ? cond->y : cond->y >> 2);
            Range r = {1<<s, rx1, rz1, w, h, y, 1};
            valid = checkForBiomes(&gen->g, NULL, r, finfo.dim, gen->seed,
                &cond->bf, (volatile char*)abort) > 0;
        }
        return valid ? COND_OK : COND_FAILED;


    case F_BIOME_CENTER:
    case F_BIOME_CENTER_256:
        if (pass == PASS_FULL_64)
        {
            s = finfo.pow2;
            rx1 = ((cond->x1 << s) + at.x) >> s;
            rz1 = ((cond->z1 << s) + at.z) >> s;
            rx2 = ((cond->x2 << s) + at.x) >> s;
            rz2 = ((cond->z2 << s) + at.z) >> s;
            int w = rx2 - rx1 + 1;
            int h = rz2 - rz1 + 1;
            Range r = {finfo.step, rx1, rz1, w, h, cond->y >> 2, 1};
            gen->init4Dim(0);

            if (cond->count == 0)
            {   // exclusion
                icnt = getBiomeCenters(
                    cent, NULL, 1, &gen->g, r, cond->biomeId, cond->biomeSize, cond->tol,
                    (volatile char*)abort
                );
                if (icnt == 0)
                {
                    cent->x = (rx1 + rx2) << 1;
                    cent->z = (rz1 + rz2) << 1;
                    if (imax) *imax = 1;
                    return COND_OK;
                }
            }
            else if (imax)
            {   // just check there are at least *inst (== cond->count) instances
                *imax = icnt = getBiomeCenters(
                    cent, NULL, cond->count, &gen->g, r, cond->biomeId, cond->biomeSize, cond->tol,
                    (volatile char*)abort
                );
                if (cond->skipref && icnt > 0)
                {   // remove origin instance
                    for (int i = 0; i < icnt; i++)
                    {
                        if (cent[i].x == at.x && cent[i].z == at.z)
                        {
                            cent[i] = cent[icnt-1];
                            *imax = --icnt;
                            break;
                        }
                    }
                }
                if (icnt >= cond->count)
                    return COND_OK;
            }
            else
            {   // we need the average position of all instances
                icnt = getBiomeCenters(
                    p, NULL, MAX_INSTANCES, &gen->g, r, cond->biomeId, cond->biomeSize, cond->tol,
                    (volatile char*)abort
                );
                xt = zt = 0;
                int j = 0;
                for (int i = 0; i < icnt; i++)
                {
                    if (cond->skipref && p[i].x == at.x && p[i].z == at.z)
                        continue;
                    xt += p[i].x;
                    zt += p[i].z;
                    j++;
                }
                if (j >= cond->count)
                {
                    cent->x = xt / j;
                    cent->z = zt / j;
                    return COND_OK;
                }
            }
            return COND_FAILED;
        }
        return COND_MAYBE_POS_INVAL;


    case F_CLIMATE_NOISE:
        if (gen->mc < MC_1_18)
            return COND_FAILED;
        rx1 = ((cond->x1 << 2) + at.x) >> 2;
        rz1 = ((cond->z1 << 2) + at.z) >> 2;
        rx2 = ((cond->x2 << 2) + at.x) >> 2;
        rz2 = ((cond->z2 << 2) + at.z) >> 2;
        cent->x = ((rx1 + rx2) << 2) >> 1;
        cent->z = ((rz1 + rz2) << 2) >> 1;
        if (pass != PASS_FULL_64)
            return COND_MAYBE_POS_VALID;
        {
            int w = rx2 - rx1 + 1;
            int h = rz2 - rz1 + 1;
            //int y = cond->y >> 2;
            gen->init4Dim(0);
            int order[] = { // use this order for performance
                NP_TEMPERATURE,
                NP_HUMIDITY,
                NP_WEIRDNESS,
                NP_EROSION,
                NP_CONTINENTALNESS,
            };
            valid = 1;
            for (uint j = 0; j < sizeof(order)/sizeof(int); j++)
            {
                int i = order[j];
                if (cond->limok[i][0] == INT_MIN && cond->limok[i][1] == INT_MAX &&
                    cond->limex[i][0] == INT_MIN && cond->limex[i][1] == INT_MAX)
                {
                    continue;
                }
                double pmin, pmax;
                double bounds[] = {
                    (double)cond->limex[i][0],
                    (double)cond->limex[i][1],
                };
                int err = getParaRange(&gen->g.bn.climate[i], &pmin, &pmax, rx1, rz1, w, h, (void*)bounds, f_confine);
                if (err)
                {
                    valid = 0;
                    break;
                }
                int lmin, lmax;
                lmin = cond->limok[i][0];
                lmax = cond->limok[i][1];
                if (pmin > lmax || pmax < lmin)
                {   // outside required limits
                    valid = 0;
                    break;
                }
                lmin = cond->limex[i][0];
                lmax = cond->limex[i][1];
                if (pmin < lmin || pmax > lmax)
                {   // ouside exclusion limits
                    valid = 0;
                    break;
                }
            }
        }
        return valid ? COND_OK : COND_FAILED;

    default:
        break;
    }

    return COND_MAYBE_POS_INVAL;
}


void findQuadStructs(int styp, Generator *g, QVector<QuadInfo> *out)
{
    StructureConfig sconf;
    if (!getStructureConfig_override(styp, g->mc, &sconf))
        return;

    int qmax = 1000;
    Pos *qlist = new Pos[qmax];
    int r = 3e7 / 512;
    int qcnt;

    if (styp == Swamp_Hut)
    {
        qcnt = scanForQuads(
            sconf, 128, g->seed & MASK48,
            low20QuadHutBarely, sizeof(low20QuadHutBarely) / sizeof(uint64_t),
            20, sconf.salt,
            -r, -r, 2*r, 2*r, qlist, qmax
        );

        for (int i = 0; i < qcnt; i++)
        {
            Pos qr = qlist[i];
            Pos qs[4];
            getStructurePos(styp, g->mc, g->seed, qr.x+0, qr.z+0, qs+0);
            getStructurePos(styp, g->mc, g->seed, qr.x+0, qr.z+1, qs+1);
            getStructurePos(styp, g->mc, g->seed, qr.x+1, qr.z+0, qs+2);
            getStructurePos(styp, g->mc, g->seed, qr.x+1, qr.z+1, qs+3);
            if (isViableStructurePos(styp, g, qs[0].x, qs[0].z, 0) &&
                isViableStructurePos(styp, g, qs[1].x, qs[1].z, 0) &&
                isViableStructurePos(styp, g, qs[2].x, qs[2].z, 0) &&
                isViableStructurePos(styp, g, qs[3].x, qs[3].z, 0))
            {
                QuadInfo qinfo;
                for (int j = 0; j < 4; j++)
                    qinfo.p[j] = qs[j];
                qinfo.c = (g->seed + sconf.salt) & 0xfffff;
                qinfo.flt = 0; // TODO
                qinfo.typ = styp;
                qinfo.afk = getOptimalAfk(qs, 7,7,9, &qinfo.spcnt);
                qinfo.rad = isQuadBase(sconf, moveStructure(g->seed,-qr.x,-qr.z), 160);
                out->push_back(qinfo);
            }
        }
    }
    else if (styp == Monument)
    {
        qcnt = scanForQuads(
            sconf, 160, g->seed & MASK48,
            g_qm_90, sizeof(g_qm_90) / sizeof(uint64_t),
            48, sconf.salt,
            -r, -r, 2*r, 2*r, qlist, qmax
        );

        for (int i = 0; i < qcnt; i++)
        {
            Pos qr = qlist[i];
            Pos qs[4];
            getStructurePos(styp, g->mc, g->seed, qr.x+0, qr.z+0, qs+0);
            getStructurePos(styp, g->mc, g->seed, qr.x+0, qr.z+1, qs+1);
            getStructurePos(styp, g->mc, g->seed, qr.x+1, qr.z+0, qs+2);
            getStructurePos(styp, g->mc, g->seed, qr.x+1, qr.z+1, qs+3);
            if (isViableStructurePos(styp, g, qs[0].x, qs[0].z, 0) &&
                isViableStructurePos(styp, g, qs[1].x, qs[1].z, 0) &&
                isViableStructurePos(styp, g, qs[2].x, qs[2].z, 0) &&
                isViableStructurePos(styp, g, qs[3].x, qs[3].z, 0))
            {
                QuadInfo qinfo;
                for (int j = 0; j < 4; j++)
                    qinfo.p[j] = qs[j];
                qinfo.c = (g->seed + sconf.salt) & MASK48;
                qinfo.flt = 0; // TODO
                qinfo.typ = styp;
                qinfo.afk = getOptimalAfk(qs, 58,0/*23*/,58, &qinfo.spcnt);
                qinfo.afk.x -= 29;
                qinfo.afk.z -= 29;
                qinfo.rad = isQuadBase(sconf, moveStructure(g->seed,-qr.x,-qr.z), 160);
                out->push_back(qinfo);
            }
        }
    }

    delete[] qlist;
}






