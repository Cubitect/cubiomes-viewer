#include "util.h"
#include "config.h"

#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>

QString getStartPieceName(int stype, const StructureVariant *sv)
{
    QString name;
    for (size_t i = 0; ; i++)
    {
        const StartPiece *sp = g_start_pieces + i;
        if (sp->stype < 0) break;
        if (sp->stype != stype) continue;
        if (sp->biome >= 0 && sp->biome != sv->biome) continue;
        if (sp->giant >= 0 && sp->giant != sv->giant) continue;
        if (sp->start != sv->start) continue;
        name += sp->name;
        break;
    }
    return name;
}

QString getBiomeDisplay(int mc, int id)
{
    if (mc >= MC_1_18)
    {
        // a bunch of 'new' biomes in 1.18 actually just got renamed
        // (based on their features and biome id conversion when upgrading)
        switch (id)
        {
        case old_growth_birch_forest:   return QApplication::translate("Biome", "Old Growth Birch Forest");
        case old_growth_pine_taiga:     return QApplication::translate("Biome", "Old Growth Pine Taiga");
        case old_growth_spruce_taiga:   return QApplication::translate("Biome", "Old Growth Spruce Taiga");
        case snowy_plains:              return QApplication::translate("Biome", "Snowy Plains");
        case sparse_jungle:             return QApplication::translate("Biome", "Sparse Jungle");
        case stony_shore:               return QApplication::translate("Biome", "Stony Shore");
        case windswept_hills:           return QApplication::translate("Biome", "Windswept Hills");
        case windswept_forest:          return QApplication::translate("Biome", "Windswept Forest");
        case windswept_gravelly_hills:  return QApplication::translate("Biome", "Windswept Gravelly Hills");
        case windswept_savanna:         return QApplication::translate("Biome", "Windswept Savanna");
        case wooded_badlands:           return QApplication::translate("Biome", "Wooded Badlands");
        }
    }

    switch (id)
    {
    case ocean:                         return QApplication::translate("Biome", "Ocean");
    case plains:                        return QApplication::translate("Biome", "Plains");
    case desert:                        return QApplication::translate("Biome", "Desert");
    case mountains:                     return QApplication::translate("Biome", "Mountains");
    case forest:                        return QApplication::translate("Biome", "Forest");
    case taiga:                         return QApplication::translate("Biome", "Taiga");
    case swamp:                         return QApplication::translate("Biome", "Swamp");
    case river:                         return QApplication::translate("Biome", "River");
    case nether_wastes:                 return QApplication::translate("Biome", "Nether Wastes");
    case the_end:                       return QApplication::translate("Biome", "The End");
    // 10
    case frozen_ocean:                  return QApplication::translate("Biome", "Frozen Ocean");
    case frozen_river:                  return QApplication::translate("Biome", "Frozen River");
    case snowy_tundra:                  return QApplication::translate("Biome", "Snowy Plains");
    case snowy_mountains:               return QApplication::translate("Biome", "Snowy Mountains");
    case mushroom_fields:               return QApplication::translate("Biome", "Mushroom Fields");
    case mushroom_field_shore:          return QApplication::translate("Biome", "Mushroom Field Shore");
    case beach:                         return QApplication::translate("Biome", "Beach");
    case desert_hills:                  return QApplication::translate("Biome", "Desert Hills");
    case wooded_hills:                  return QApplication::translate("Biome", "Wooded Hills");
    case taiga_hills:                   return QApplication::translate("Biome", "Taiga Hills");
    // 20
    case mountain_edge:                 return QApplication::translate("Biome", "Mountain Edge");
    case jungle:                        return QApplication::translate("Biome", "Jungle");
    case jungle_hills:                  return QApplication::translate("Biome", "Jungle Hills");
    case jungle_edge:                   return QApplication::translate("Biome", "Jungle Edge");
    case deep_ocean:                    return QApplication::translate("Biome", "Deep Ocean");
    case stone_shore:                   return QApplication::translate("Biome", "Stone Shore");
    case snowy_beach:                   return QApplication::translate("Biome", "Snowy Beach");
    case birch_forest:                  return QApplication::translate("Biome", "Birch Forest");
    case birch_forest_hills:            return QApplication::translate("Biome", "Birch Forest Hills");
    case dark_forest:                   return QApplication::translate("Biome", "Dark Forest");
    // 30
    case snowy_taiga:                   return QApplication::translate("Biome", "Snowy Taiga");
    case snowy_taiga_hills:             return QApplication::translate("Biome", "Snowy Taiga Hills");
    case giant_tree_taiga:              return QApplication::translate("Biome", "Giant Tree Taiga");
    case giant_tree_taiga_hills:        return QApplication::translate("Biome", "Giant Tree Taiga Hills");
    case wooded_mountains:              return QApplication::translate("Biome", "Wooded Mountains");
    case savanna:                       return QApplication::translate("Biome", "Savanna");
    case savanna_plateau:               return QApplication::translate("Biome", "Savanna Plateau");
    case badlands:                      return QApplication::translate("Biome", "Badlands");
    case wooded_badlands_plateau:       return QApplication::translate("Biome", "Wooded Badlands Plateau");
    case badlands_plateau:              return QApplication::translate("Biome", "Badlands Plateau");
    // 40  --  1.13
    case small_end_islands:             return QApplication::translate("Biome", "Small End Islands");
    case end_midlands:                  return QApplication::translate("Biome", "End Midlands");
    case end_highlands:                 return QApplication::translate("Biome", "End Highlands");
    case end_barrens:                   return QApplication::translate("Biome", "End Barrens");
    case warm_ocean:                    return QApplication::translate("Biome", "Warm Ocean");
    case lukewarm_ocean:                return QApplication::translate("Biome", "Lukewarm Ocean");
    case cold_ocean:                    return QApplication::translate("Biome", "Cold Ocean");
    case deep_warm_ocean:               return QApplication::translate("Biome", "Deep Warm Ocean");
    case deep_lukewarm_ocean:           return QApplication::translate("Biome", "Deep Lukewarm Ocean");
    case deep_cold_ocean:               return QApplication::translate("Biome", "Deep Cold Ocean");
    // 50
    case deep_frozen_ocean:             return QApplication::translate("Biome", "Deep Frozen Ocean");
    // Alpha 1.2 - Beta 1.7
    case seasonal_forest:               return QApplication::translate("Biome", "Seasonal Forest");
    case shrubland:                     return QApplication::translate("Biome", "Shrubland");
    case rainforest:                    return QApplication::translate("Biome", "Rain Forest");

    case the_void:                      return QApplication::translate("Biome", "The Void");

    // mutated variants
    case sunflower_plains:              return QApplication::translate("Biome", "Sunflower Plains");
    case desert_lakes:                  return QApplication::translate("Biome", "Desert Lakes");
    case gravelly_mountains:            return QApplication::translate("Biome", "Gravelly Mountains");
    case flower_forest:                 return QApplication::translate("Biome", "Flower Forest");
    case taiga_mountains:               return QApplication::translate("Biome", "Taiga Mountains");
    case swamp_hills:                   return QApplication::translate("Biome", "Swamp Hills");
    case ice_spikes:                    return QApplication::translate("Biome", "Ice Spikes");
    case modified_jungle:               return QApplication::translate("Biome", "Modified Jungle");
    case modified_jungle_edge:          return QApplication::translate("Biome", "Modified Jungle Edge");
    case tall_birch_forest:             return QApplication::translate("Biome", "Tall Birch Forest");
    case tall_birch_hills:              return QApplication::translate("Biome", "Tall Birch Hills");
    case dark_forest_hills:             return QApplication::translate("Biome", "Dark Forest Hills");
    case snowy_taiga_mountains:         return QApplication::translate("Biome", "Snowy Taiga Mountains");
    case giant_spruce_taiga:            return QApplication::translate("Biome", "Giant Spruce Taiga");
    case giant_spruce_taiga_hills:      return QApplication::translate("Biome", "Giant Spruce Taiga Hills");
    case modified_gravelly_mountains:   return QApplication::translate("Biome", "Gravelly Mountains+");
    case shattered_savanna:             return QApplication::translate("Biome", "Shattered Savanna");
    case shattered_savanna_plateau:     return QApplication::translate("Biome", "Shattered Savanna Plateau");
    case eroded_badlands:               return QApplication::translate("Biome", "Eroded Badlands");
    case modified_wooded_badlands_plateau: return QApplication::translate("Biome", "Modified Wooded Badlands Plateau");
    case modified_badlands_plateau:     return QApplication::translate("Biome", "Modified Badlands Plateau");
    // 1.14
    case bamboo_jungle:                 return QApplication::translate("Biome", "Bamboo Jungle");
    case bamboo_jungle_hills:           return QApplication::translate("Biome", "Bamboo Jungle Hills");
    // 1.16
    case soul_sand_valley:              return QApplication::translate("Biome", "Soul Sand Valley");
    case crimson_forest:                return QApplication::translate("Biome", "Crimson Forest");
    case warped_forest:                 return QApplication::translate("Biome", "Warped Forest");
    case basalt_deltas:                 return QApplication::translate("Biome", "Basalt Deltas");
    // 1.17
    case dripstone_caves:               return QApplication::translate("Biome", "Dripstone Caves");
    case lush_caves:                    return QApplication::translate("Biome", "Lush Caves");
    // 1.18
    case meadow:                        return QApplication::translate("Biome", "Meadow");
    case grove:                         return QApplication::translate("Biome", "Grove");
    case snowy_slopes:                  return QApplication::translate("Biome", "Snowy Slopes");
    case stony_peaks:                   return QApplication::translate("Biome", "Stony Peaks");
    case jagged_peaks:                  return QApplication::translate("Biome", "Jagged Peaks");
    case frozen_peaks:                  return QApplication::translate("Biome", "Frozen Peaks");
    // 1.19
    case deep_dark:                     return QApplication::translate("Biome", "Deep Dark");
    case mangrove_swamp:                return QApplication::translate("Biome", "Mangrove Swamp");
    // 1.20
    case cherry_grove:                  return QApplication::translate("Biome", "Cherry Grove");
    }
    const char *name = biome2str(mc, id);
    return name ? name : "";
}

RandGen::RandGen()
{
    std::random_device rd;
    if (rd.entropy())
        mt = std::mt19937_64(rd());
    else
        mt = std::mt19937_64(time(0));
}

// get a random 64-bit integer
uint64_t getRnd64()
{
    static QMutex mutex;
    static RandGen rng;
    uint64_t ret = 0;
    mutex.lock();
    std::uniform_int_distribution<uint64_t> d;
    ret = d(rng.mt);
    mutex.unlock();
    return ret;
}

int str2seed(const QString &str, uint64_t *out)
{
    if (str.isEmpty())
    {
        *out = getRnd64();
        return S_RANDOM;
    }

    bool ok = false;
    *out = (uint64_t) str.toLongLong(&ok);
    if (ok)
    {
        return S_NUMERIC;
    }

    // String.hashCode();
    int32_t hash = 0;
    const ushort *chars = str.utf16();
    for (int i = 0; chars[i] != 0; i++)
    {
        hash = 31 * hash + chars[i];
    }
    *out = hash;
    return S_TEXT;
}

bool IdCmp::operator() (int id1, int id2)
{
   if (id1 == id2) return false;
   // treat 256 as a special case with high priority
   if (id1 == 256) return true;
   if (id2 == 256) return false;
   if (mode == SORT_ID)
       return id1 < id2;
   int v1 = 1, v2 = 1;
   if (mc >= 0)
   {   // biomes not in this version go to the back
       v1 &= biomeExists(mc, id1);
       v2 &= biomeExists(mc, id2);
   }
   if (dim != DIM_UNDEF)
   {   // biomes in other dimensions go to the back
       v1 &= getDimension(id1) == dim;
       v2 &= getDimension(id2) == dim;
   }
   if (v1 ^ v2)
       return v1;
   if (mode == SORT_DIM)
   {
       int d1 = getDimension(id1);
       int d2 = getDimension(id2);
       if (d1 != d2)
           return (d1==0 ? 0 : d1==-1 ? 1 : 2) < (d2==0 ? 0 : d2==-1 ? 1 : 2);
   }
   const char *s1 = biome2str(mc, id1);
   const char *s2 = biome2str(mc, id2);
   if (!s1 && !s2) return id1 < id2;
   if (!s1) return false; // move non-biomes to back
   if (!s2) return true;
   return strcmp(s1, s2) < 0;
}

bool IdCmp::isPrimary(int id)
{
   if (mode == SORT_ID)
       return true;
   if (mc >= 0 && !biomeExists(mc, id))
       return false;
   if (dim != DIM_UNDEF && getDimension(id) != dim)
       return false;
   return true;
}


QPixmap getPix(QString rc, int width)
{
    if (rc.isEmpty() && width)
    {
        QPixmap p(width, width);
        p.fill(Qt::transparent);
        return p;
    }
    QPixmap pix(":/icons/" + rc + ".png");
    if (pix.isNull())
        return pix;
    int w = pix.width();
    if (width != w)
    {
        pix = pix.scaledToWidth(w * 8);
        if (width)
            pix = pix.scaledToWidth(width, Qt::SmoothTransformation);
    }
    w = pix.width();
    if (w > pix.height())
    {
        QPixmap p(w, w);
        p.fill(Qt::transparent);
        QPainter painter(&p);
        painter.drawPixmap(0, (w - pix.height()) / 2, pix);
        pix = p;
    }
    return pix;
}

QIcon getColorIcon(const QColor& col, const QPen& pen)
{
    int s = (int) round(g_fontscale * 14);
    QPixmap pixmap(s, s);
    pixmap.fill(QColor(0,0,0,0));
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    int b = pen.width();
    path.addRoundedRect(pixmap.rect().adjusted(b, b, -b, -b), 3, 3);
    p.setPen(pen);
    p.fillPath(path, col);
    p.drawPath(path);
    return QIcon(pixmap);
}

QIcon getBiomeIcon(int id, bool warn)
{
    QColor col(g_biomeColors[id][0], g_biomeColors[id][1], g_biomeColors[id][2]);
    return getColorIcon(col, warn ? QPen(Qt::red, 2) : QPen(Qt::black, 1));
}
