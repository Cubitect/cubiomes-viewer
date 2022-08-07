#include "analysis.h"
#include "cutil.h"
#include "settings.h"
#include "quad.h"

#include <QElapsedTimer>

#include <vector>


Analysis::Analysis(QObject *parent)
    : QThread(parent)
{
}

void Analysis::requestAbort()
{
    stop = true;
}

static
QTreeWidgetItem *setConditionTreeItems(ConditionTree& ctree, int node, int64_t seed, Pos cpos[], QTreeWidgetItem* parent)
{
    Condition& c = ctree.condvec[node];
    Pos p = cpos[c.save];
    const std::vector<char>& branches = ctree.references[c.save];

    QTreeWidgetItem* item = new QTreeWidgetItem(parent);
    item->setText(0, c.summary());
    item->setData(0, Qt::UserRole, QVariant::fromValue(seed));
    item->setData(0, Qt::UserRole+1, QVariant::fromValue(p));

    if (branches.empty())
    {
        item->setText(1, Analysis::tr("incomplete"));
    }
    else
    {
        item->setText(1, QString::asprintf("%d,\t%d", p.x, p.z));
        for (char b : branches)
            setConditionTreeItems(ctree, b, seed, cpos, item);
    }
    return item;
}

void Analysis::run()
{
    stop = false;

    Generator g;
    setupGenerator(&g, wi.mc, wi.large);

    QList<QTreeWidgetItem*> topitems;

    for (int64_t seed : qAsConst(seeds))
    {
        if (stop) break;
        wi.seed = seed;
        QTreeWidgetItem *seeditem = new QTreeWidgetItem();
        topitems.append(seeditem);
        seeditem->setText(0, tr("seed"));
        seeditem->setData(1, Qt::DisplayRole, QVariant::fromValue(seed));
        seeditem->setData(0, Qt::UserRole, QVariant::fromValue(seed));

        if (ck_biome)
        {
            const int step = 512;
            long idcnt[256] = {0};

            for (int x = x1; x <= x2 && !stop; x += step)
            {
                for (int z = z1; z <= z2 && !stop; z += step)
                {
                    int w = x2-x+1 < step ? x2-x+1 : step;
                    int h = z2-z+1 < step ? z2-z+1 : step;
                    Range r = {1, x, z, w, h, wi.y, 1};
                    int *ids = allocCache(&g, r);

                    int dims[] = {0, -1, +1};
                    for (int d = 0; d < 3; d++)
                    {
                        if (dims[d] == dim || !map_only)
                        {
                            applySeed(&g, dims[d], wi.seed);
                            genBiomes(&g, ids, r);
                            for (int i = 0; i < w*h; i++)
                                idcnt[ ids[i] & 0xff ]++;
                        }
                    }
                    free(ids);
                }
            }

            if (!stop)
            {
                int bcnt = 0;
                for (int i = 0; i < 256; i++)
                    bcnt += !!idcnt[i];

                QTreeWidgetItem* item_cat = new QTreeWidgetItem(seeditem);
                item_cat->setText(0, tr("biomes", "Analysis ID"));
                item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(bcnt));

                for (int id = 0; id < 256; id++)
                {
                    long cnt = idcnt[id];
                    if (cnt <= 0)
                        continue;
                    const char *s;
                    if (!(s = biome2str(wi.mc, id)))
                        continue;
                    QTreeWidgetItem* item = new QTreeWidgetItem(item_cat);
                    item->setText(0, s);
                    item->setData(1, Qt::DisplayRole, QVariant::fromValue(cnt));
                }
            }
        }

        if (ck_struct)
        {
            std::vector<VarPos> st;
            for (int sopt = D_DESERT; sopt < D_SPAWN; sopt++)
            {
                if (stop) break;
                int sdim = 0;
                if (sopt == D_FORTESS || sopt == D_BASTION || sopt == D_PORTALN)
                    sdim = -1;
                if (sopt == D_ENDCITY || sopt == D_GATEWAY)
                    sdim = 1;
                if (map_only)
                {
                    if (!mapshow[sopt])
                        continue;
                    if (sdim != dim)
                        continue;
                }
                int stype = mapopt2stype(sopt);
                st.clear();
                StructureConfig sconf;
                if (!getStructureConfig_override(stype, wi.mc, &sconf))
                    continue;
                getStructs(&st, sconf, wi, sdim, x1, z1, x2, z2);
                if (st.empty())
                    continue;

                QTreeWidgetItem* item_cat = new QTreeWidgetItem(seeditem);
                const char *s = struct2str(stype);
                item_cat->setText(0, s);
                item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(st.size()));

                for (size_t i = 0; i < st.size(); i++)
                {
                    VarPos vp = st[i];
                    QTreeWidgetItem* item = new QTreeWidgetItem(item_cat);
                    item->setData(0, Qt::UserRole, QVariant::fromValue(seed));
                    item->setData(0, Qt::UserRole+1, QVariant::fromValue(vp.p));
                    item->setText(0, QString::asprintf("%d,\t%d", vp.p.x, vp.p.z));
                    if (vp.v.abandoned)
                    {
                        if (stype == Village)
                            item->setText(1, tr("abandoned", "Village variant"));
                    }
                }
            }

            if (!stop)
            if ((dim == 0 && mapshow[D_SPAWN]) || !map_only)
            {
                applySeed(&g, 0, wi.seed);
                Pos pos = getSpawn(&g);
                if (pos.x >= x1 && pos.x <= x2 && pos.z >= z1 && pos.z <= z2)
                {
                    QTreeWidgetItem* item_cat = new QTreeWidgetItem(seeditem);
                    item_cat->setText(0, tr("spawn", "Analysis ID"));
                    item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(1));
                    QTreeWidgetItem* item = new QTreeWidgetItem(item_cat);
                    item->setData(0, Qt::UserRole, QVariant::fromValue(seed));
                    item->setData(0, Qt::UserRole+1, QVariant::fromValue(pos));
                    item->setText(0, QString::asprintf("%d,\t%d", pos.x, pos.z));
                }
            }

            if (!stop)
            if ((dim == 0 && mapshow[D_STRONGHOLD]) || !map_only)
            {
                StrongholdIter sh;
                initFirstStronghold(&sh, wi.mc, wi.seed);
                std::vector<Pos> shp;
                applySeed(&g, dim, wi.seed);
                while (nextStronghold(&sh, &g) > 0)
                {
                    Pos pos = sh.pos;
                    if (pos.x >= x1 && pos.x <= x2 && pos.z >= z1 && pos.z <= z2)
                        shp.push_back(pos);
                    if (stop) break;
                }

                if (!shp.empty())
                {
                    QTreeWidgetItem* item_cat = new QTreeWidgetItem(seeditem);
                    item_cat->setText(0, tr("stronghold", "Analysis ID"));
                    item_cat->setData(1, Qt::DisplayRole, QVariant::fromValue(shp.size()));
                    for (Pos pos : shp)
                    {
                        QTreeWidgetItem* item = new QTreeWidgetItem(item_cat);
                        item->setData(0, Qt::UserRole, QVariant::fromValue(seed));
                        item->setData(0, Qt::UserRole+1, QVariant::fromValue(pos));
                        item->setText(0, QString::asprintf("%d,\t%d", pos.x, pos.z));
                    }
                }
            }
        }

        if (!stop)
        if (ck_conds && !conds.empty())
        {
            WorldGen gen;
            gen.init(wi.mc, wi.large);
            gen.setSeed(wi.seed);

            ConditionTree condtree;
            condtree.set(conds, wi);

            Pos origin = {0, 0};
            Pos cpos[MAX_INSTANCES] = {};
            if (testTreeAt(origin, &condtree, PASS_FULL_64, &gen, &stop, cpos)
                == COND_OK)
            {
                setConditionTreeItems(condtree, 0, seed, cpos, seeditem);
            }
        }

        if (seeditem->childCount() == 0)
        {
            delete seeditem;
            continue;
        }
        if (stop)
            seeditem->setText(0, tr("seed (incomplete)"));
        emit itemDone(seeditem);
    }
}
