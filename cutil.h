#ifndef CUTIL_H
#define CUTIL_H

#include <QMutex>

#include <random>

#include "cubiomes/finders.h"
#include "cubiomes/util.h"

extern unsigned char biomeColors[256][3];
extern unsigned char tempsColors[256][3];


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

inline const char *biome2str(int id)
{
    switch (id)
    {
    case ocean: return "ocean";
    case plains: return "plains";
    case desert: return "desert";
    case mountains: return "mountains";
    case forest: return "forest";
    case taiga: return "taiga";
    case swamp: return "swamp";
    case river: return "river";
    case nether_wastes: return "nether_wastes";
    case the_end: return "the_end";
    // 10
    case frozen_ocean: return "frozen_ocean";
    case frozen_river: return "frozen_river";
    case snowy_tundra: return "snowy_tundra";
    case snowy_mountains: return "snowy_mountains";
    case mushroom_fields: return "mushroom_fields";
    case mushroom_field_shore: return "mushroom_field_shore";
    case beach: return "beach";
    case desert_hills: return "desert_hills";
    case wooded_hills: return "wooded_hills";
    case taiga_hills: return "taiga_hills";
    // 20
    case mountain_edge: return "mountain_edge";
    case jungle: return "jungle";
    case jungle_hills: return "jungle_hills";
    case jungle_edge: return "jungle_edge";
    case deep_ocean: return "deep_ocean";
    case stone_shore: return "stone_shore";
    case snowy_beach: return "snowy_beach";
    case birch_forest: return "birch_forest";
    case birch_forest_hills: return "birch_forest_hills";
    case dark_forest: return "dark_forest";
    // 30
    case snowy_taiga: return "snowy_taiga";
    case snowy_taiga_hills: return "snowy_taiga_hills";
    case giant_tree_taiga: return "giant_tree_taiga";
    case giant_tree_taiga_hills: return "giant_tree_taiga_hills";
    case wooded_mountains: return "wooded_mountains";
    case savanna: return "savanna";
    case savanna_plateau: return "savanna_plateau";
    case badlands: return "badlands";
    case wooded_badlands_plateau: return "wooded_badlands_plateau";
    case badlands_plateau: return "badlands_plateau";
    // 40  --  1.13
    case small_end_islands: return "small_end_islands";
    case end_midlands: return "end_midlands";
    case end_highlands: return "end_highlands";
    case end_barrens: return "end_barrens";
    case warm_ocean: return "warm_ocean";
    case lukewarm_ocean: return "lukewarm_ocean";
    case cold_ocean: return "cold_ocean";
    case deep_warm_ocean: return "deep_warm_ocean";
    case deep_lukewarm_ocean: return "deep_lukewarm_ocean";
    case deep_cold_ocean: return "deep_cold_ocean";
    // 50
    case deep_frozen_ocean: return "deep_frozen_ocean";

    case the_void: return "the_void";

    // mutated variants
    case sunflower_plains: return "sunflower_plains";
    case desert_lakes: return "desert_lakes";
    case gravelly_mountains: return "gravelly_mountains";
    case flower_forest: return "flower_forest";
    case taiga_mountains: return "taiga_mountains";
    case swamp_hills: return "swamp_hills";
    case ice_spikes: return "ice_spikes";
    case modified_jungle: return "modified_jungle";
    case modified_jungle_edge: return "modified_jungle_edge";
    case tall_birch_forest: return "tall_birch_forest";
    case tall_birch_hills: return "tall_birch_hills";
    case dark_forest_hills: return "dark_forest_hills";
    case snowy_taiga_mountains: return "snowy_taiga_mountains";
    case giant_spruce_taiga: return "giant_spruce_taiga";
    case giant_spruce_taiga_hills: return "giant_spruce_taiga_hills";
    case modified_gravelly_mountains: return "modified_gravelly_mountains";
    case shattered_savanna: return "shattered_savanna";
    case shattered_savanna_plateau: return "shattered_savanna_plateau";
    case eroded_badlands: return "eroded_badlands";
    case modified_wooded_badlands_plateau: return "modified_wooded_badlands_plateau";
    case modified_badlands_plateau: return "modified_badlands_plateau";
    // 1.14
    case bamboo_jungle: return "bamboo_jungle";
    case bamboo_jungle_hills: return "bamboo_jungle_hills";
    // 1.16
    case soul_sand_valley: return "soul_sand_valley";
    case crimson_forest: return "crimson_forest";
    case warped_forest: return "warped_forest";
    case basalt_deltas: return "basalt_deltas";
    }
    return NULL;
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

#endif // CUTIL_H
