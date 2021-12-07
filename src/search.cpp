#include "search.h"
#include "seedtables.h"
#include "mainwindow.h"

#include <QThread>

#include <algorithm>


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
    if (intersectRectLine(x1-112, z1-112, x2+112, z2+112, c*r1, s*r1, c*r2, s*r2))
        return true;
    c = cos(sh.angle + M_PI*4/3);
    s = sin(sh.angle + M_PI*4/3);
    if (intersectRectLine(x1-112, z1-112, x2+112, z2+112, c*r1, s*r1, c*r2, s*r2))
        return true;

    return false;
}


/* Checks if a seeds satisfies the conditions list.
 */
int testSeedAt(
    Pos                         at,
    Pos                         cpos[100],
    QVector<Condition>        * condvec,
    int                         pass,
    WorldGen                  * gen,
    std::atomic_bool          * abort,
    char                        states[100],
    int                         idxc0
)
{
    Condition *cond = condvec->data();
    int n = condvec->size();
    int p = PASS_FAST_48;
    int ret;

    cpos[0] = at;

    for (int i = idxc0; i < n; i++)
    {
        states[ cond[i].save ] = 0;
    }

    // check fast 48-bit first and then the specified pass (if not equal)
    for (;;)
    {
        ret = COND_OK;
        for (int i = idxc0; i < n; i++)
        {
            Condition *c = cond + i;

            int sav = c->save;
            int rel = c->relative;
            int st;

            if (rel)
            {
                if (states[rel] != COND_OK &&
                    states[rel] != COND_MAYBE_POS_VALID)
                {   // condition is relative to something we don't know yet
                    st = COND_MAYBE_POS_INVAL;
                    states[sav] = st;
                    if (ret > st)
                        ret = st;
                    continue;
                }
            }

            if (states[sav] == COND_OK)
                continue; // already checked and satisfied


            int rx1, rz1, rx2, rz2;
            int sref = -1;

            switch (c->type)
            {
            case F_REFERENCE_1:     sref = 0;  goto L_ref_pow2;
            case F_REFERENCE_16:    sref = 4;  goto L_ref_pow2;
            case F_REFERENCE_64:    sref = 6;  goto L_ref_pow2;
            case F_REFERENCE_256:   sref = 8;  goto L_ref_pow2;
            case F_REFERENCE_512:   sref = 9;  goto L_ref_pow2;
            case F_REFERENCE_1024:  sref = 10; goto L_ref_pow2;
            L_ref_pow2:
                rx1 = ((c->x1 << sref) + at.x) >> sref;
                rz1 = ((c->z1 << sref) + at.z) >> sref;
                rx2 = ((c->x2 << sref) + at.x) >> sref;
                rz2 = ((c->z2 << sref) + at.z) >> sref;
                break;
            default:
                sref = -1;
                break;
            }

            if (sref >= 0)
            {
                // helper condition -
                // iterating over an area at a given scale with recursion

                states[sav] = COND_OK; // relatives need OK parent state
                st = COND_FAILED;
                for (int z = rz1; z <= rz2; z++)
                {
                    for (int x = rx1; x <= rx2; x++)
                    {
                        cpos[sav].x = (x << sref);
                        cpos[sav].z = (z << sref);

                        int sta = testSeedAt(
                            cpos[sav],
                            cpos,
                            condvec,
                            p,
                            gen,
                            abort,
                            states,
                            i+1
                        );

                        if (sta > st)
                            st = sta;
                        if (st == COND_OK)
                            goto L_ref_finish;
                        if (*abort)
                            return COND_FAILED;
                    }
                }
            }
            else if (c->type == F_SCALE_TO_NETHER)
            {
                states[sav] = COND_OK;
                cpos[sav].x = cpos[rel].x / 8;
                cpos[sav].z = cpos[rel].z / 8;
                st = testCondAt(cpos[sav], cpos+sav, c, pass, gen, abort);
            }
            else if (c->type == F_SCALE_TO_OVERWORLD)
            {
                states[sav] = COND_OK;
                cpos[sav].x = cpos[rel].x * 8;
                cpos[sav].z = cpos[rel].z * 8;
                st = testCondAt(cpos[sav], cpos+sav, c, pass, gen, abort);
            }
            else
            {
                st = testCondAt(cpos[rel], cpos+sav, c, pass, gen, abort);
            }

        L_ref_finish:;
            if (st == COND_FAILED)
                return COND_FAILED;
            if (st < ret)
                ret = st;
            states[sav] = st;

            if (sref > 0)
                break;
        }
        if (p == pass)
            break;
        p = pass;
    }

    return ret;
}

int testSeedAt(
    Pos                         at,
    Pos                         cpos[100],
    QVector<Condition>        * condvec,
    int                         pass,
    WorldGen                  * gen,
    std::atomic_bool          * abort
)
{
    char states[100];
    return testSeedAt(at, cpos, condvec, pass, gen, abort, states, 0);
}

static const int g_qh_c_n = sizeof(low20QuadHutBarely) / sizeof(uint64_t);
static QuadInfo qh_constellations[g_qh_c_n];

// initialize global tables
void _init(void) __attribute__((constructor));
void _init(void)
{
    int st = Swamp_Hut;
    StructureConfig sc = SWAMP_HUT_CONFIG;
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


/* Tests if a condition is satisfied with 'at' as origin for a search pass.
 * If sufficiently satisfied (check return value) the center point is stored
 * in 'cent'.
 */
int
testCondAt(
    Pos                 at,     // relative origin
    Pos               * cent,   // output center position
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
    int i, j, n;
    int64_t s, r, rmin, rmax;
    const uint64_t *seeds;
    Pos p[128];

    const FilterInfo& finfo = g_filterinfo.list[cond->type];

    if ((st = finfo.stype) > 0)
    {
        if (!getStructureConfig_override(finfo.stype, gen->mc, &sconf))
            return COND_FAILED;
    }

    switch (cond->type)
    {
    case F_REFERENCE_1:
    case F_REFERENCE_16:
    case F_REFERENCE_64:
    case F_REFERENCE_256:
    case F_REFERENCE_512:
    case F_REFERENCE_1024:
    case F_SCALE_TO_NETHER:
    case F_SCALE_TO_OVERWORLD:
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
                rx1, rz1, rx2 - rx1 + 1, rz2 - rz1 + 1, p, 128);
        if (n < 1)
            return COND_FAILED;
        valid = COND_FAILED;
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
            //if (!isQuadBaseFeature24(sconf, s, 7,7,9))
            //    return COND_FAILED;
            valid = COND_OK;
            cent->x = (pc.x << 9) + qi->afk.x;
            cent->z = (pc.z << 9) + qi->afk.z;
            break;
        }
        return valid;

    case F_QM_95:   qual = 58*58*4 * 95 / 100;  goto L_qm_any;
    case F_QM_90:   qual = 58*58*4 * 90 / 100;
L_qm_any:

        rx1 = ((cond->x1 << 9) + at.x) >> 9;
        rz1 = ((cond->z1 << 9) + at.z) >> 9;
        rx2 = ((cond->x2 << 9) + at.x) >> 9;
        rz2 = ((cond->z2 << 9) + at.z) >> 9;
        if (scanForQuads(
                sconf, 160, (gen->seed) & MASK48, g_qm_90,
                sizeof(g_qm_90) / sizeof(uint64_t), 48, sconf.salt,
                rx1, rz1, rx2 - rx1 + 1, rz2 - rz1 + 1, &pc, 1) >= 1)
        {
            rx = pc.x; rz = pc.z;
            s = moveStructure(gen->seed, -rx, -rz);
            if (qmonumentQual(s + sconf.salt) >= qual)
            {
                getStructurePos(st, gen->mc, gen->seed, rx+0, rz+0, p+0);
                getStructurePos(st, gen->mc, gen->seed, rx+0, rz+1, p+1);
                getStructurePos(st, gen->mc, gen->seed, rx+1, rz+0, p+2);
                getStructurePos(st, gen->mc, gen->seed, rx+1, rz+1, p+3);
                *cent = getOptimalAfk(p, 58,23,58, 0);
                cent->x -= 29; // monument is centered
                cent->z -= 29;
                return COND_OK;
            }
        }
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

    case F_FORTRESS:
    case F_BASTION:

    case F_ENDCITY:
    case F_GATEWAY:

        x1 = cond->x1 + at.x;
        z1 = cond->z1 + at.z;
        x2 = cond->x2 + at.x;
        z2 = cond->z2 + at.z;

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
        n = 0;

        // Note "<="
        for (rz = rz1; rz <= rz2 && !*abort; rz++)
        {
            for (rx = rx1; rx <= rx2; rx++)
            {
                if (!getStructurePos(st, gen->mc, gen->seed, rx+0, rz+0, &pc))
                    continue;
                if (pc.x < x1 || pc.x > x2 || pc.z < z1 || pc.z > z2)
                    continue;
                if (pass == PASS_FULL_64 || (pass == PASS_FULL_48 && !finfo.dep64))
                {
                    if (*abort) return COND_FAILED;

                    if (st == Village && cond->variants)
                    {
                        int vv[] = {
                            plains, desert, savanna, taiga, snowy_tundra,
                            // plains village variant covers meadows
                        };
                        int vn = sizeof(vv) / sizeof(int);
                        int i;
                        for (i = 0; i < vn; i++)
                        {
                            StructureVariant vt = getVillageType(
                                gen->mc, gen->seed, pc.x, pc.z, vv[i]);
                            if (cond->villageOk(gen->mc, vt))
                                break;
                        }
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
                    else if (st == Village)
                    {
                        if (cond->variants)
                        {
                            StructureVariant vt = getVillageType(
                                gen->mc, gen->seed, pc.x, pc.z, id);
                            if (!cond->villageOk(gen->mc, vt))
                                continue;
                        }
                    }
                    if (gen->mc >= MC_1_18)
                    {
                        if (!isViableStructureTerrain(st, &gen->g, pc.x, pc.z))
                            continue;
                    }
                }
                xt += pc.x;
                zt += pc.z;
                n++;
            }
        }

        if (n >= cond->count)
        {
            cent->x = xt / n;
            cent->z = zt / n;

            if (pass == PASS_FULL_64 || (pass == PASS_FULL_48 && !finfo.dep64))
                return COND_OK;
            // some non-exhaustive structure clusters do not
            // have known center positions with 48-bit seeds
            if (cond->count != (1+rx2-rx1) * (1+rz2-rz1))
                return COND_MAYBE_POS_INVAL;
            return COND_MAYBE_POS_VALID;
        }
        return COND_FAILED;

    case F_MINESHAFT:

        x1 = cond->x1 + at.x;
        z1 = cond->z1 + at.z;
        x2 = cond->x2 + at.x;
        z2 = cond->z2 + at.z;
        rx1 = x1 >> 4;
        rz1 = z1 >> 4;
        rx2 = x2 >> 4;
        rz2 = z2 >> 4;
        n = getMineshafts(gen->mc, gen->seed, rx1, rz1, rx2, rz2, p, 128);
        if (n >= cond->count)
        {
            xt = zt = 0;
            for (int i = 0; i < n; i++)
            {
                xt += p[i].x;
                zt += p[i].z;
            }
            cent->x = xt / n;
            cent->z = zt / n;
            return COND_OK;
        }
        return COND_FAILED;

    case F_SPAWN:

        cent->x = cent->z = 0;
        if (pass != PASS_FULL_64)
            return COND_MAYBE_POS_INVAL;

        x1 = cond->x1 + at.x;
        z1 = cond->z1 + at.z;
        x2 = cond->x2 + at.x;
        z2 = cond->z2 + at.z;

        if (*abort) return COND_FAILED;
        gen->init4Dim(0);
        pc = getSpawn(&gen->g);
        if (pc.x >= x1 && pc.x <= x2 && pc.z >= z1 && pc.z <= z2)
        {
            *cent = pc;
            return COND_OK;
        }
        return COND_FAILED;

    case F_STRONGHOLD:

        x1 = cond->x1 + at.x;
        z1 = cond->z1 + at.z;
        x2 = cond->x2 + at.x;
        z2 = cond->z2 + at.z;

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

        if (gen->mc < MC_1_9)
        {
            if (rmax < 640*640 || rmin > 1152*1152)
                return COND_FAILED;
            r = 0;
            rmin = 640;
            rmax = 1152;
        }
        else
        {   // check if the area is entirely outside the radii ranges in which strongholds can generate
            if (rmax < 1408*1408)
                return COND_FAILED;
            rmin = sqrt(rmin);
            rmax = sqrt(rmax);
            r = (rmax - 1408) / 3072;       // maximum relevant ring number
            if (rmax - rmin < 3072-1280)    // area does not span more than one ring
            {
                if (rmin > 1408+1280+3072*r)
                    return COND_FAILED;     // area is between rings
            }
            rmin = 1408;
            rmax = 1408+1280;
        }
        // if we are only looking at the inner ring, we can check if the generation angles are suitable
        if (r == 0 && !isInnerRingOk(gen->mc, gen->seed, x1, z1, x2, z2, rmin, rmax))
            return COND_FAILED;

        // pre-biome-checks complete, the area appears to line up with possible generation positions
        if (pass != PASS_FULL_64)
        {
            cent->x = cent->z = 0;
            return COND_MAYBE_POS_INVAL;
        }
        else
        {
            StrongholdIter sh;
            initFirstStronghold(&sh, gen->mc, gen->seed);
            n = 0;
            gen->init4Dim(0);
            while (nextStronghold(&sh, &gen->g) > 0)
            {
                if (*abort || sh.ringnum > r)
                    break;

                if (sh.pos.x >= x1 && sh.pos.x <= x2 && sh.pos.z >= z1 && sh.pos.z <= z2)
                {
                    if (++n >= cond->count)
                    {   // should the center use all strongholds for consitency?
                        *cent = sh.pos;
                        return COND_OK;
                    }
                }

                if (sh.ringnum == r && sh.ringidx+1 == sh.ringmax)
                    break;
            }
        }
        return COND_FAILED;

    case F_SLIME:

        rx1 = ((cond->x1 << 4) + at.x) >> 4;
        rz1 = ((cond->z1 << 4) + at.z) >> 4;
        rx2 = ((cond->x2 << 4) + at.x) >> 4;
        rz2 = ((cond->z2 << 4) + at.z) >> 4;

        n = 0;
        xt = zt = 0;
        for (int rz = rz1; rz <= rz2; rz++)
        {
            for (int rx = rx1; rx <= rx2; rx++)
            {
                if (isSlimeChunk(gen->seed, rx, rz))
                {
                    xt += rx;
                    zt += rz;
                    n++;
                }
            }
        }
        if (n >= cond->count)
        {
            cent->x = (xt << 4) / n;
            cent->z = (zt << 4) / n;
            return COND_OK;
        }
        return COND_FAILED;

    // biome filters reference specific layers
    // MAYBE: options for layers in different versions?
    case F_BIOME:           s = 0;
        if (gen->mc >= MC_1_18)    goto L_noise_biome;
        else                       goto L_biome_filter_any;
    case F_BIOME_4_RIVER:   s = 2; goto L_biome_filter_any;
    case F_BIOME_256_OTEMP: s = 8; goto L_biome_filter_any;

L_biome_filter_any:
        if (gen->mc >= MC_1_18)
            return COND_FAILED;
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
                NULL, gen->seed, rx1, rz1, w, h, cond->bfilter, cond->approx) > 0)
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


    case F_BIOME_4:         s = 2;  goto L_noise_biome;
    case F_BIOME_16:        s = 4;  goto L_noise_biome;
    case F_BIOME_64:        s = 6;  goto L_noise_biome;
    case F_BIOME_256:       s = 8;  goto L_noise_biome;
    case F_BIOME_NETHER_1:  s = 0;  goto L_noise_biome;
    case F_BIOME_NETHER_4:  s = 2;  goto L_noise_biome;
    case F_BIOME_NETHER_16: s = 4;  goto L_noise_biome;
    case F_BIOME_NETHER_64: s = 6;  goto L_noise_biome;
    case F_BIOME_END_1:     s = 0;  goto L_noise_biome;
    case F_BIOME_END_4:     s = 2;  goto L_noise_biome;
    case F_BIOME_END_16:    s = 4;  goto L_noise_biome;
    case F_BIOME_END_64:    s = 6;  goto L_noise_biome;

L_noise_biome:
        if (pass == PASS_FAST_48)
            return COND_MAYBE_POS_VALID;
        // the Nether and End require only the 48-bit seed
        // (except voronoi uses the full 64-bits)
        if (pass == PASS_FULL_48 && finfo.dep64)
            return COND_MAYBE_POS_VALID;
        rx1 = ((cond->x1 << s) + at.x) >> s;
        rz1 = ((cond->z1 << s) + at.z) >> s;
        rx2 = ((cond->x2 << s) + at.x) >> s;
        rz2 = ((cond->z2 << s) + at.z) >> s;
        cent->x = ((rx1 + rx2) << s) >> 1;
        cent->z = ((rz1 + rz2) << s) >> 1;
        {
            int w = rx2 - rx1 + 1;
            int h = rz2 - rz1 + 1;
            int y = (s == 0 ? cond->y : cond->y >> 2);
            Range r = {1<<s, rx1, rz1, w, h, y, 1};
            valid = checkForBiomes(&gen->g, NULL, r, finfo.dim, gen->seed,
                cond->bfilter, cond->approx, (volatile char*)abort) > 0;
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






