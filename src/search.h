#ifndef SEARCH_H
#define SEARCH_H

#include "cubiomes/finders.h"

#include "lua/src/lua.hpp"

#include <QVector>
#include <QString>
#include <QMap>
#include <atomic>

enum
{
    CAT_NONE,
    CAT_HELPER,
    CAT_QUAD,
    CAT_STRUCT,
    CAT_BIOMES,
    CAT_OTHER,
    CATEGORY_MAX,
};

enum
{
    F_SELECT,
    F_QH_IDEAL,
    F_QH_CLASSIC,
    F_QH_NORMAL,
    F_QH_BARELY,
    F_QM_95,
    F_QM_90,
    F_BIOME,
    F_BIOME_4_RIVER,
    F_BIOME_16,
    F_BIOME_64,
    F_BIOME_256,
    F_BIOME_256_OTEMP,
    F_TEMPS,
    F_SLIME,
    F_SPAWN,
    F_STRONGHOLD,
    F_DESERT,
    F_JUNGLE,
    F_HUT,
    F_IGLOO,
    F_MONUMENT,
    F_VILLAGE,
    F_OUTPOST,
    F_MANSION,
    F_TREASURE,
    F_RUINS,
    F_SHIPWRECK,
    F_PORTAL,
    F_FORTRESS,
    F_BASTION,
    F_ENDCITY,
    F_BIOME_NETHER,
    F_BIOME_NETHER_4,
    F_BIOME_NETHER_16,
    F_BIOME_NETHER_64,
    F_BIOME_NETHER_256,
    F_BIOME_END,
    F_BIOME_END_4,
    F_BIOME_END_16,
    F_BIOME_END_64,
    F_PORTALN,
    F_GATEWAY,
    F_MINESHAFT,
    F_SPIRAL,
    F_SPIRAL_16,
    F_SPIRAL_64,
    F_SPIRAL_256,
    F_SPIRAL_512,
    F_SPIRAL_1024,
    F_BIOME_4, // differs from F_BIOME_4_RIVER, since this may include oceans
    F_SCALE_TO_NETHER,
    F_SCALE_TO_OVERWORLD,
    F_LOGIC_OR,
    F_SPIRAL_4,
    F_FIRST_STRONGHOLD,
    F_CLIMATE_NOISE,
    F_ANCIENT_CITY,
    F_LOGIC_NOT,
    F_BIOME_CENTER,
    F_BIOME_CENTER_256,
    F_CLIMATE_MINMAX,
    F_HEIGHT,
    F_LUA,
    F_WELL,
    F_TRAILS,
    F_BIOME_SAMPLE,
    F_NOISE_SAMPLE,
    // new filters should be added here at the end to keep some downwards compatibility
    FILTER_MAX,
};

struct FilterInfo
{
    enum {
        LOC_1 = 1, LOC_2 = 2, LOC_R = 4,
        LOC_NIL = 0x0,
        LOC_POS = 0x1,
        LOC_REC = 0x3,
        LOC_RAD = 0x7,
    };
    enum {
        BR_NONE  = 0, // one position - no branching
        BR_FIRST = 1, // choose only first, no branching
        BR_SPLIT = 2, // branches are examined separately
        BR_CLUST = 3, // allow clustering to avoid branching
    };

    int cat;    // seed source category
    bool dep64; // depends on 64-bit seed
    int loc;
    int stype;  // structure type
    int grid;   // coordinate multiplier
    int branch; // branching behaviour
    int mcmin;  // minimum version
    int mcmax;  // maximum version
    int dim;    // dimension
    int hasy;   // has vertical height
    int disp;   // display order
    const char *icon;
    const char *name;
    const char *description;
};

// global table of filter data (as constants with enum indexing)
static const struct FilterList : private FilterInfo
{
    FilterInfo list[FILTER_MAX];

    FilterList() : list{}
    {
        int disp = 0; // display order

        list[F_SELECT] = FilterInfo{
            CAT_NONE, 0, LOC_NIL, 0, 0, BR_NONE, MC_UNDEF, MC_NEWEST, 0, 0, disp++,
            NULL,
            "",
            ""
        };
        list[F_LOGIC_OR] = FilterInfo{
            CAT_HELPER, 0, LOC_NIL, 0, 1, BR_NONE, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            "helper",
            QT_TRANSLATE_NOOP("Filter", "OR logic gate"),
            QT_TRANSLATE_NOOP("Filter",
            "Evaluates as true when any of the conditions that reference it "
            "(by relative location) are met. When no referencing conditions are "
            "defined, it defaults to true.")
        };
        list[F_LOGIC_NOT] = FilterInfo{
            CAT_HELPER, 0, LOC_NIL, 0, 1, BR_NONE, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            "helper",
            QT_TRANSLATE_NOOP("Filter", "NOT logic gate"),
            QT_TRANSLATE_NOOP("Filter",
            "Evaluates as true when none of the conditions that reference it "
            "(by relative location) are met. When no referencing conditions are "
            "defined, it defaults to true.")
        };
        list[F_LUA] = FilterInfo{
            CAT_HELPER, 0, LOC_NIL, 0, 1, BR_NONE, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            "helper",
            QT_TRANSLATE_NOOP("Filter", "Lua"),
            QT_TRANSLATE_NOOP("Filter",
            "Define custom conditions using Lua scripts.")
        };
        list[F_SCALE_TO_NETHER] = FilterInfo{
            CAT_HELPER, 0, LOC_NIL, 0, 1, BR_NONE, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            "portal_lit",
            QT_TRANSLATE_NOOP("Filter", "Coordinate factor x/8"),
            QT_TRANSLATE_NOOP("Filter",
            "Divides relative location by 8, from Overworld to Nether.")
        };
        list[F_SCALE_TO_OVERWORLD] = FilterInfo{
            CAT_HELPER, 0, LOC_NIL, 0, 1, BR_NONE, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            "portal_lit",
            QT_TRANSLATE_NOOP("Filter", "Coordinate factor x*8"),
            QT_TRANSLATE_NOOP("Filter",
            "Multiplies relative location by 8, from Nether to Overworld.")
        };
        list[F_SPIRAL] = FilterInfo{
            CAT_HELPER, 0, LOC_RAD, 0, 1, BR_FIRST, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            "reference",
            QT_TRANSLATE_NOOP("Filter", "Spiral iterator"),
            QT_TRANSLATE_NOOP("Filter",
            "<html><head/><body>"
            "Spiral iterator conditions can be used to move a testing position across "
            "a given area using a certain step size. Other conditions that refer to it "
            "as a relative location will be checked at each step. The iteration is "
            "performed in a spiral, so positions closer to the center get priority."
            "</body></html>")
        };

        list[F_QH_IDEAL] = FilterInfo{
            CAT_QUAD, 0, LOC_RAD, Swamp_Hut, 512, BR_FIRST, MC_1_4, MC_NEWEST, 0, 0, disp++,
            "quad",
            QT_TRANSLATE_NOOP("Filter", "Quad-hut (ideal)"),
            QT_TRANSLATE_NOOP("Filter",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in one of the best configurations that exist.")
        };

        list[F_QH_CLASSIC] = FilterInfo{
            CAT_QUAD, 0, LOC_RAD, Swamp_Hut, 512, BR_FIRST, MC_1_4, MC_NEWEST, 0, 0, disp++,
            "quad",
            QT_TRANSLATE_NOOP("Filter", "Quad-hut (classic)"),
            QT_TRANSLATE_NOOP("Filter",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in one of the \"classic\" configurations. "
            "(Checks for huts in the nearest 2x2 chunk corners of each "
            "region.)")
        };

        list[F_QH_NORMAL] = FilterInfo{
            CAT_QUAD, 0, LOC_RAD, Swamp_Hut, 512, BR_FIRST, MC_1_4, MC_NEWEST, 0, 0, disp++,
            "quad",
            QT_TRANSLATE_NOOP("Filter", "Quad-hut (normal)"),
            QT_TRANSLATE_NOOP("Filter",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, such that all of them are within 128 blocks "
            "of a single AFK location, including a vertical tolerance "
            "for a fall damage chute.")
        };

        list[F_QH_BARELY] = FilterInfo{
            CAT_QUAD, 0, LOC_RAD, Swamp_Hut, 512, BR_FIRST, MC_1_4, MC_NEWEST, 0, 0, disp++,
            "quad",
            QT_TRANSLATE_NOOP("Filter", "Quad-hut (barely)"),
            QT_TRANSLATE_NOOP("Filter",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in any configuration, such that the bounding "
            "boxes are within 128 blocks of a single AFK location.")
        };

        list[F_QM_95] = FilterInfo{
            CAT_QUAD, 0, LOC_RAD, Monument, 512, BR_FIRST, MC_1_8, MC_NEWEST, 0, 0, disp++,
            "quad",
            QT_TRANSLATE_NOOP("Filter", "Quad-ocean-monument (>95%)"),
            QT_TRANSLATE_NOOP("Filter",
            "The lower 48-bits provide potential for 95% of the area of "
            "four ocean monuments to be within 128 blocks of an AFK "
            "location.")
        };

        list[F_QM_90] = FilterInfo{
            CAT_QUAD, 0, LOC_RAD, Monument, 512, BR_FIRST, MC_1_8, MC_NEWEST, 0, 0, disp++,
            "quad",
            QT_TRANSLATE_NOOP("Filter", "Quad-ocean-monument (>90%)"),
            QT_TRANSLATE_NOOP("Filter",
            "The lower 48-bits provide potential for 90% of the area of "
            "four ocean monuments to be within 128 blocks of an AFK "
            "location.")
        };

        list[F_BIOME_SAMPLE] = FilterInfo{
            CAT_BIOMES, 1, LOC_RAD, 0, 1, BR_SPLIT, MC_B1_7, MC_NEWEST, 0, 1, disp++,
            "overworld",
            QT_TRANSLATE_NOOP("Filter", "Biome samples"),
            QT_TRANSLATE_NOOP("Filter",
            "Samples biomes in a given area to find if a proportion of the "
            "biomes match a set of allowed biomes.")
        };

        list[F_BIOME] = FilterInfo{
            CAT_BIOMES, 1, LOC_REC, 0, 1, BR_NONE, MC_B1_7, MC_NEWEST, 0, 1, disp++,
            "overworld",
            QT_TRANSLATE_NOOP("Filter", "Overworld at scale"),
            QT_TRANSLATE_NOOP("Filter",
            "Allows only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };
        list[F_BIOME_NETHER] = FilterInfo{
            CAT_BIOMES, 0, LOC_REC, 0, 1, BR_NONE, MC_1_16_1, MC_NEWEST, -1, 1, disp++,
            "nether",
            QT_TRANSLATE_NOOP("Filter", "Nether at scale"),
            QT_TRANSLATE_NOOP("Filter",
            "Nether biomes sampled on a scaled grid.")
        };
        list[F_BIOME_END] = FilterInfo{
            CAT_BIOMES, 0, LOC_REC, 0, 1, BR_NONE, MC_1_9, MC_NEWEST, +1, 1, disp++,
            "the_end",
            QT_TRANSLATE_NOOP("Filter", "End at scale"),
            QT_TRANSLATE_NOOP("Filter",
            "End biomes sampled on a scaled grid.")
        };

        list[F_BIOME_4_RIVER] = FilterInfo{
            CAT_BIOMES, 1, LOC_REC, 0, 4, BR_NONE, MC_1_13, MC_1_17, 0, 0, disp++,
            "map",
            QT_TRANSLATE_NOOP("Filter", "Biome layer 1:4 RIVER"),
            QT_TRANSLATE_NOOP("Filter",
            "Allows only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer RIVER with scale 1:4. "
            "This layer does not generate ocean variants.")
        };
        list[F_BIOME_256_OTEMP] = FilterInfo{
            CAT_BIOMES, 0, LOC_REC, 0, 256, BR_NONE, MC_1_13, MC_1_17, 0, 0, disp++,
            "map",
            QT_TRANSLATE_NOOP("Filter", "Biome layer 1:256 O.TEMP"),
            QT_TRANSLATE_NOOP("Filter",
            "Allows only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer OCEAN TEMPERATURE with scale 1:256. "
            "This generation layer depends only on the lower 48-bits of the seed.")
        };

        list[F_CLIMATE_NOISE] = FilterInfo{
            CAT_BIOMES, 1, LOC_REC, 0, 4, BR_NONE, MC_1_18, MC_NEWEST, 0, 0, disp++,
            "overworld",
            QT_TRANSLATE_NOOP("Filter", "Climate parameters"),
            QT_TRANSLATE_NOOP("Filter",
            "Custom limits for the required and allowed climate noise parameters that "
            "the specified area should cover.")
        };
        list[F_NOISE_SAMPLE] = FilterInfo{
            CAT_BIOMES, 1, LOC_RAD, 0, 4, BR_SPLIT, MC_1_18, MC_NEWEST, 0, 0, disp++,
            "overworld",
            QT_TRANSLATE_NOOP("Filter", "Climate noise samples"),
            QT_TRANSLATE_NOOP("Filter",
            "Samples climate noise in a given area to find if a proportion of the "
            "biomes match a set of allowed biomes.")
        };
        list[F_CLIMATE_MINMAX] = FilterInfo{
            CAT_BIOMES, 1, LOC_REC, 0, 4, BR_NONE, MC_1_18, MC_NEWEST, 0, 0, disp++,
            "overworld",
            QT_TRANSLATE_NOOP("Filter", "Locate climate extreme"),
            QT_TRANSLATE_NOOP("Filter",
            "Finds the location where a climate parameter reaches its minimum or maximum.")
        };
        list[F_BIOME_CENTER] = FilterInfo{
            CAT_BIOMES, 1, LOC_REC, 0, 4, BR_CLUST, MC_B1_7, MC_NEWEST, 0, 1, disp++,
            "map",
            QT_TRANSLATE_NOOP("Filter", "Locate biome center"),
            QT_TRANSLATE_NOOP("Filter",
            "Finds the center position of a given biome.")
        };
        list[F_BIOME_CENTER_256] = FilterInfo{
            CAT_BIOMES, 1, LOC_REC, 0, 256, BR_CLUST, MC_B1_7, MC_1_17, 0, 1, disp++,
            "map",
            QT_TRANSLATE_NOOP("Filter", "Locate biome center 1:256"),
            QT_TRANSLATE_NOOP("Filter",
            "Finds the center position of a given biome. Based on the 1:256 biome layer.")
        };
        list[F_TEMPS] = FilterInfo{
            CAT_BIOMES, 1, LOC_REC, 0, 1024, BR_NONE, MC_1_7, MC_1_17, 0, 0, disp++,
            "tempcat",
            QT_TRANSLATE_NOOP("Filter", "Temperature categories"),
            QT_TRANSLATE_NOOP("Filter",
            "Checks that the area has a minimum of all the required temperature categories.")
        };

        list[F_SPAWN] = FilterInfo{
            CAT_OTHER, 1, LOC_RAD, 0, 1, BR_NONE, MC_B1_8, MC_NEWEST, 0, 0, disp++,
            "spawn",
            QT_TRANSLATE_NOOP("Filter", "Spawn"),
            ""
        };

        list[F_SLIME] = FilterInfo{
            CAT_OTHER, 0, LOC_REC, 0, 16, BR_CLUST, MC_UNDEF, MC_NEWEST, 0, 0, disp++,
            "slime",
            QT_TRANSLATE_NOOP("Filter", "Slime chunk"),
            ""
        };
        list[F_HEIGHT] = FilterInfo{
            CAT_OTHER, 1, LOC_POS, 0, 4, BR_NONE, MC_1_1, MC_NEWEST, 0, 0, disp++,
            "height",
            QT_TRANSLATE_NOOP("Filter", "Surface height"),
            QT_TRANSLATE_NOOP("Filter",
            "Check the approximate surface height at scale 1:4 at a single coordinate.")
        };

        list[F_FIRST_STRONGHOLD] = FilterInfo{
            CAT_OTHER, 0, LOC_RAD, 0, 1, BR_NONE, MC_B1_8, MC_NEWEST, 0, 0, disp++,
            "stronghold",
            QT_TRANSLATE_NOOP("Filter", "First stronghold"),
            QT_TRANSLATE_NOOP("Filter",
            "Finds the approxmiate location of the first stronghold "
            "(+/-112 blocks). Depends only on the 48-bit seed.")
        };

        list[F_STRONGHOLD] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, 0, 1, BR_CLUST, MC_B1_8, MC_NEWEST, 0, 0, disp++,
            "stronghold",
            QT_TRANSLATE_NOOP("Filter", "Stronghold"),
            ""
        };

        list[F_VILLAGE] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Village, 1, BR_CLUST, MC_B1_8, MC_NEWEST, 0, 0, disp++,
            "village",
            QT_TRANSLATE_NOOP("Filter", "Village"),
            ""
        };

        list[F_MINESHAFT] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Mineshaft, 1, BR_CLUST, MC_B1_8, MC_NEWEST, 0, 0, disp++,
            "mineshaft",
            QT_TRANSLATE_NOOP("Filter", "Abandoned mineshaft"),
            ""
        };

        list[F_DESERT] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Desert_Pyramid, 1, BR_CLUST, MC_1_3, MC_NEWEST, 0, 0, disp++,
            "desert",
            QT_TRANSLATE_NOOP("Filter", "Desert pyramid"),
            QT_TRANSLATE_NOOP("Filter",
            "In version 1.18+, desert pyramids depend on surface height and may fail to "
            "generate near caves/aquifers, rivers and oceans.")
        };

        list[F_JUNGLE] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Jungle_Temple, 1, BR_CLUST, MC_1_3, MC_NEWEST, 0, 0, disp++,
            "jungle",
            QT_TRANSLATE_NOOP("Filter", "Jungle temple"),
            QT_TRANSLATE_NOOP("Filter",
            "In version 1.18+, jungle temples depend on surface height and may fail to "
            "generate near caves/aquifers, rivers and oceans.")
        };

        list[F_HUT] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Swamp_Hut, 1, BR_CLUST, MC_1_4, MC_NEWEST, 0, 0, disp++,
            "hut",
            QT_TRANSLATE_NOOP("Filter", "Swamp hut"),
            ""
        };

        list[F_MONUMENT] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Monument, 1, BR_CLUST, MC_1_8, MC_NEWEST, 0, 0, disp++,
            "monument",
            QT_TRANSLATE_NOOP("Filter", "Ocean monument"),
            ""
        };

        list[F_IGLOO] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Igloo, 1, BR_CLUST, MC_1_9, MC_NEWEST, 0, 0, disp++,
            "igloo",
            QT_TRANSLATE_NOOP("Filter", "Igloo"),
            ""
        };

        list[F_MANSION] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Mansion, 1, BR_CLUST, MC_1_11, MC_NEWEST, 0, 0, disp++,
            "mansion",
            QT_TRANSLATE_NOOP("Filter", "Woodland mansion"),
            QT_TRANSLATE_NOOP("Filter",
            "In version 1.18+, mansions depend on surface height and may fail to "
            "generate near caves/aquifers, rivers and oceans.")
        };

        list[F_RUINS] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Ocean_Ruin, 1, BR_CLUST, MC_1_13, MC_NEWEST, 0, 0, disp++,
            "ruins",
            QT_TRANSLATE_NOOP("Filter", "Ocean ruin"),
            ""
        };

        list[F_SHIPWRECK] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Shipwreck, 1, BR_CLUST, MC_1_13, MC_NEWEST, 0, 0, disp++,
            "shipwreck",
            QT_TRANSLATE_NOOP("Filter", "Shipwreck"),
            ""
        };

        list[F_TREASURE] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Treasure, 1, BR_CLUST, MC_1_13, MC_NEWEST, 0, 0, disp++,
            "treasure",
            QT_TRANSLATE_NOOP("Filter", "Buried treasure"),
            QT_TRANSLATE_NOOP("Filter",
            "Buried treasures are always positioned near the center of a chunk "
            "rather than a chunk boarder. Make sure the testing area is set "
            "accordingly.")
        };

        list[F_WELL] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Desert_Well, 1, BR_CLUST, MC_1_13, MC_NEWEST, 0, 0, disp++,
            "well",
            QT_TRANSLATE_NOOP("Filter", "Desert well"),
            ""
        };

        list[F_OUTPOST] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Outpost, 1, BR_CLUST, MC_1_14, MC_NEWEST, 0, 0, disp++,
            "outpost",
            QT_TRANSLATE_NOOP("Filter", "Pillager outpost"),
            ""
        };

        list[F_ANCIENT_CITY] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Ancient_City, 1, BR_CLUST, MC_1_19, MC_NEWEST, 0, 0, disp++,
            "ancient_city",
            QT_TRANSLATE_NOOP("Filter", "Ancient city"),
            ""
        };

        list[F_TRAILS] = FilterInfo{
            CAT_STRUCT, 1, LOC_RAD, Trail_Ruins, 1, BR_CLUST, MC_1_20, MC_NEWEST, 0, 0, disp++,
            "trails",
            QT_TRANSLATE_NOOP("Filter", "Trail ruins"),
            ""
        };

        list[F_PORTAL] = FilterInfo{
            CAT_STRUCT, 0, LOC_RAD, Ruined_Portal, 1, BR_CLUST, MC_1_16_1, MC_NEWEST, 0, 0, disp++,
            "portal",
            QT_TRANSLATE_NOOP("Filter", "Ruined portal (overworld)"),
            ""
        };

        list[F_PORTALN] = FilterInfo{
            CAT_STRUCT, 0, LOC_RAD, Ruined_Portal_N, 1, BR_CLUST, MC_1_16_1, MC_NEWEST, -1, 0, disp++,
            "portal",
            QT_TRANSLATE_NOOP("Filter", "Ruined portal (nether)"),
            ""
        };

        list[F_FORTRESS] = FilterInfo{
            CAT_STRUCT, 0, LOC_RAD, Fortress, 1, BR_CLUST, MC_1_0, MC_NEWEST, -1, 0, disp++,
            "fortress",
            QT_TRANSLATE_NOOP("Filter", "Nether fortress"),
            ""
        };

        list[F_BASTION] = FilterInfo{
            CAT_STRUCT, 0, LOC_RAD, Bastion, 1, BR_CLUST, MC_1_16_1, MC_NEWEST, -1, 0, disp++,
            "bastion",
            QT_TRANSLATE_NOOP("Filter", "Bastion remnant"),
            ""
        };

        list[F_ENDCITY] = FilterInfo{
            CAT_STRUCT, 0, LOC_RAD, End_City, 1, BR_CLUST, MC_1_9, MC_NEWEST, +1, 0, disp++,
            "endcity",
            QT_TRANSLATE_NOOP("Filter", "End city"),
            ""
        };

        list[F_GATEWAY] = FilterInfo{
            CAT_STRUCT, 0, LOC_RAD, End_Gateway, 1, BR_CLUST, MC_1_13, MC_NEWEST, +1, 0, disp++,
            "gateway",
            QT_TRANSLATE_NOOP("Filter", "End gateway"),
            QT_TRANSLATE_NOOP("Filter",
            "Checks only scattered return gateways. Does not include those generated "
            "when defeating the dragon.")
        };
    }
}
g_filterinfo;


struct /*__attribute__((packed))*/ Condition
{
    // should be POD or at least Standard Layout
    // layout needs to remain consistent across versions

    enum { // condition version upgrades
        VER_LEGACY      = 0,
        VER_2_3_0       = 1,
        VER_2_4_0       = 2,
        VER_3_4_0       = 3,
        VER_4_0_0       = 4,
        VER_CURRENT     = VER_4_0_0,
    };
    enum { // meta flags
        DISABLED        = 0x0001,
    };
    enum { // condition flags
        FLG_APPROX      = 0x0001,
        FLG_MATCH_ANY   = 0x0010,
        FLG_IN_RANGE    = 0x0020,
        FLG_INVERT      = 0x0040,
    };
    enum { // variant flags
        VAR_WITH_START  = 0x0001, // restrict start piece index and biome
        VAR_ABANODONED  = 0x0002, // zombie village
        VAR_ENDSHIP     = 0x0004, // end city ship
        VAR_DENSE_BB    = 0x0008, // fortress with a 2x2 arrangement of start/crossings
        VAR_NOT         = 0x0010, // invert flag (e.g. not abandoned)
        VAR_BASEMENT    = 0x0020, // igloo with basement
    };
    enum { // min/max
        // legacy 0:min<= 1:max>= 2:min>= 3:max<=
        E_LOCATE_MIN    = 0x10,
        E_LOCATE_MAX    = 0x20,
        E_TEST_LOWER    = 0x40,
        E_TEST_UPPER    = 0x80,
    };
    int16_t     type;
    uint16_t    meta;
    int32_t     x1, z1, x2, z2;
    int32_t     save;
    int32_t     relative;
    uint8_t     skipref;
    uint8_t     pad0[3]; // legacy
    char        text[28];
    uint8_t     pad1[12]; // legacy
    uint64_t    hash;
    int8_t      deps[16]; // currently unused
    uint64_t    biomeToFind, biomeToFindM; // inclusion biomes
    int32_t     biomeId; // legacy oceanToFind(8)
    uint32_t    biomeSize;
    uint8_t     tol; // legacy specialCnt(4)
    uint8_t     minmax;
    uint8_t     para;
    uint8_t     octave;
    uint16_t    step;
    uint16_t    version; // condition data version
    uint64_t    biomeToExcl, biomeToExclM; // exclusion biomes
    int32_t     temps[9];
    int32_t     count;
    int32_t     y;
    uint32_t    flags;
    int32_t     rmax; // (<=0):disabled; (>0):strict upper radius
    uint16_t    varflags;
    int16_t     varbiome; // unused
    uint64_t    varstart;
    int32_t     limok[NP_MAX][2];
    int32_t     limex[NP_MAX][2];
    float       vmin;
    float       vmax;
    float       converage;
    float       confidence;

    // generated members - initialized when the search is started
    uint8_t     generated_start[0]; // address dummy
    BiomeFilter bf;

    // perform version upgrades
    bool versionUpgrade();

    // initialize the generated members
    QString apply(int mc);

    QString toHex() const;
    bool readHex(const QString& hex);

    QString summary(bool aligntab) const;
};

static_assert(
    offsetof(Condition, generated_start) == 320,
    "Layout of Condition has changed!"
);


#define MAX_INSTANCES 4096 // should be at least 128

enum
{
    COND_FAILED = 0,            // seed does not meet the condition
    COND_MAYBE_POS_INVAL = 1,   // search pass insufficient for result
    COND_MAYBE_POS_VALID = 2,   // search pass insufficient, but known center
    COND_OK = 3,                // seed satisfies the condition
};

enum
{
    PASS_FAST_48,       // only do fast checks that do not require biome gen
    PASS_FULL_48,       // include possible biome checks for 48-bit seeds
    PASS_FULL_64,       // run full test on a 64-bit seed
};

struct ConditionTree
{
    std::vector<Condition> condvec;
    std::vector<std::vector<char>> references;

    ~ConditionTree();
    QString set(const std::vector<Condition>& cv, int mc);
};

struct SearchThreadEnv
{
    ConditionTree condtree;

    Generator g;
    SurfaceNoise sn;

    int mc, large;
    uint64_t seed;
    int surfdim;
    int octaves;

    int searchpass;
    std::atomic_bool *stop;

    std::map<uint64_t, lua_State*> l_states;

    SearchThreadEnv();
    ~SearchThreadEnv();

    QString init(int mc, bool large, const ConditionTree& condtree);

    void setSeed(uint64_t seed);
    void init4Dim(int dim);
    void init4Noise(int nptype, int octaves);
    void prepareSurfaceNoise(int dim);
};

/* Checks if a seed satisfies the conditions tree.
 * Returns the lowest condition fulfillment status.
 */
int testTreeAt(
    Pos                         at,             // relative origin
    SearchThreadEnv           * env,            // thread-local environment
    int                         pass,           // search pass
    Pos                       * path            // ok trigger positions
);

int testCondAt(
    Pos                         at,             // relative origin
    SearchThreadEnv           * env,            // thread-local environment
    Pos                       * cent,           // output center position(s)
    int                       * imax,           // max instances (NULL for avg)
    const Condition           * cond            // condition to check
);

struct QuadInfo
{
    uint64_t c; // constellation seed
    Pos p[4];   // individual positions
    Pos afk;    // optimal afk position
    int flt;    // filter id (quality)
    int typ;    // type of structure
    int spcnt;  // number of planar spawning spaces
    float rad;  // enclosing radius
};

void findQuadStructs(int styp, Generator *g, QVector<QuadInfo> *out);


#endif // SEARCH_H
