#ifndef SEARCH_H
#define SEARCH_H

#include "settings.h"

#include "lua/src/lua.hpp"

#include <QVector>
#include <QString>
#include <QMap>
#include <atomic>

#define PRECOMPUTE48_BUFSIZ ((int64_t)1 << 30)

enum
{
    CAT_NONE,
    CAT_HELPER,
    CAT_QUAD,
    CAT_STRUCT,
    CAT_BIOMES,
    CAT_NETHER,
    CAT_END,
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
    F_BIOME_NETHER_1,
    F_BIOME_NETHER_4,
    F_BIOME_NETHER_16,
    F_BIOME_NETHER_64,
    F_BIOME_NETHER_256,
    F_BIOME_END_1,
    F_BIOME_END_4,
    F_BIOME_END_16,
    F_BIOME_END_64,
    F_PORTALN,
    F_GATEWAY,
    F_MINESHAFT,
    F_SPIRAL_1,
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
    // new filters should be added here at the end to keep some downwards compatibility
    FILTER_MAX,
};

struct FilterInfo
{
    int cat;    // seed source category
    bool dep64; // depends on 64-bit seed
    bool coord; // requires coordinate entry
    bool area;  // requires area entry
    bool rmax;  // supports radial range
    int layer;  // associated generator layer
    int stype;  // structure type
    int step;   // coordinate multiplier
    int pow2;   // bit position of step
    int count;  // can have instances
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
static const struct FilterList
{
    FilterInfo list[FILTER_MAX];

    FilterList() : list{}
    {
        int disp = 0; // display order

        list[F_SELECT] = FilterInfo{
            CAT_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, MC_UNDEF, MC_NEWEST, 0, 0, disp++,
            NULL,
            "",
            ""
        };

#define _(S) QT_TRANSLATE_NOOP("Filter", S)

        list[F_LOGIC_OR] = FilterInfo{
            CAT_HELPER, 0, 0, 0, 0, 0, 0, 1, 0, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/helper.png",
            _("OR logic gate"),
            _("Evaluates as true when any of the conditions that reference it "
            "(by relative location) are met. When no referencing conditions are "
            "defined, it defaults to true.")
        };
        list[F_LOGIC_NOT] = FilterInfo{
            CAT_HELPER, 0, 0, 0, 0, 0, 0, 1, 0, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/helper.png",
            _("NOT logic gate"),
            _("Evaluates as true when none of the conditions that reference it "
            "(by relative location) are met. When no referencing conditions are "
            "defined, it defaults to true.")
        };
        list[F_LUA] = FilterInfo{
            CAT_HELPER, 0, 0, 0, 0, 0, 0, 1, 0, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/helper.png",
            _("Lua"),
            _("Define custom conditions using Lua scripts.")
        };
        list[F_SCALE_TO_NETHER] = FilterInfo{
            CAT_HELPER, 0, 0, 0, 0, 0, 0, 1, 0, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/portal_lit.png",
            _("Coordinate factor x/8"),
            _("Divides relative location by 8, from Overworld to Nether.")
        };
        list[F_SCALE_TO_OVERWORLD] = FilterInfo{
            CAT_HELPER, 0, 0, 0, 0, 0, 0, 1, 0, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/portal_lit.png",
            _("Coordinate factor x*8"),
            _("Multiplies relative location by 8, from Nether to Overworld.")
        };
        const char *spiral_desc = _(
            "<html><head/><body>"
            "Spiral iterator conditions can be used to move a testing position across "
            "a given area using a certain step size. Other conditions that refer to it "
            "as a relative location will be checked at each step. The iteration is "
            "performed in a spiral, so positions closer to the center get priority."
            "</body></html>"
        );
        list[F_SPIRAL_1] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 1, 0, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/reference.png",
            _("Spiral iterator 1:1"),
            spiral_desc
        };
        list[F_SPIRAL_4] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 4, 2, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/reference.png",
            _("Spiral iterator 1:4"),
            spiral_desc
        };
        list[F_SPIRAL_16] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 16, 4, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/reference.png",
            _("Spiral iterator 1:16"),
            spiral_desc
        };
        list[F_SPIRAL_64] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 64, 6, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/reference.png",
            _("Spiral iterator 1:64"),
            spiral_desc
        };
        list[F_SPIRAL_256] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 256, 8, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/reference.png",
            _("Spiral iterator 1:256"),
            spiral_desc
        };
        list[F_SPIRAL_512] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 512, 9, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/reference.png",
            _("Spiral iterator 1:512"),
            spiral_desc
        };
        list[F_SPIRAL_1024] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 1024, 10, 0, MC_UNDEF, MC_NEWEST, DIM_UNDEF, 0, disp++,
            ":icons/reference.png",
            _("Spiral iterator 1:1024"),
            spiral_desc
        };

        list[F_QH_IDEAL] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Swamp_Hut, 512, 9, 0, MC_1_4, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-hut (ideal)"),
            _("The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in one of the best configurations that exist.")
        };

        list[F_QH_CLASSIC] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Swamp_Hut, 512, 9, 0, MC_1_4, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-hut (classic)"),
            _("The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in one of the \"classic\" configurations. "
            "(Checks for huts in the nearest 2x2 chunk corners of each "
            "region.)")
        };

        list[F_QH_NORMAL] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Swamp_Hut, 512, 9, 0, MC_1_4, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-hut (normal)"),
            _("The lower 48-bits provide potential for four swamp huts in "
            "spawning range, such that all of them are within 128 blocks "
            "of a single AFK location, including a vertical tollerance "
            "for a fall damage chute.")
        };

        list[F_QH_BARELY] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Swamp_Hut, 512, 9, 0, MC_1_4, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-hut (barely)"),
            _("The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in any configuration, such that the bounding "
            "boxes are within 128 blocks of a single AFK location.")
        };

        list[F_QM_95] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Monument, 512, 9, 0, MC_1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-ocean-monument (>95%)"),
            _("The lower 48-bits provide potential for 95% of the area of "
            "four ocean monuments to be within 128 blocks of an AFK "
            "location.")
        };

        list[F_QM_90] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Monument, 512, 9, 0, MC_1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-ocean-monument (>90%)"),
            _("The lower 48-bits provide potential for 90% of the area of "
            "four ocean monuments to be within 128 blocks of an AFK "
            "location.")
        };

        list[F_BIOME] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, L_VORONOI_1, 0, 1, 0, 0, MC_B1_7, MC_1_17, 0, 1, disp++, // disable for 1.18
            ":icons/map.png",
            _("Biomes 1:1"),
            _("Allows only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };

        list[F_BIOME_4] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 4, 2, 0, MC_B1_7, MC_NEWEST, 0, 1, disp++,
            ":icons/map.png",
            _("Biomes 1:4"),
            _("Allows only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };
        list[F_BIOME_16] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 16, 4, 0, MC_B1_7, MC_NEWEST, 0, 1, disp++,
            ":icons/map.png",
            _("Biomes 1:16"),
            _("Allows only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };
        list[F_BIOME_64] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 64, 6, 0, MC_B1_7, MC_NEWEST, 0, 1, disp++,
            ":icons/map.png",
            _("Biomes 1:64"),
            _("Allows only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };
        list[F_BIOME_256] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 256, 8, 0, MC_B1_7, MC_NEWEST, 0, 1, disp++,
            ":icons/map.png",
            _("Biomes 1:256"),
            _("Allows only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };

        list[F_BIOME_4_RIVER] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, L_RIVER_MIX_4, 0, 4, 2, 0, MC_1_13, MC_1_17, 0, 0, disp++,
            ":icons/map.png",
            _("Biomes 1:4 RIVER"),
            _("Allows only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer RIVER with scale 1:4. "
            "This layer does not generate ocean variants.")
        };
        list[F_BIOME_256_OTEMP] = FilterInfo{
            CAT_BIOMES, 0, 1, 1, 0, L_OCEAN_TEMP_256, 0, 256, 8, 0, MC_1_13, MC_1_17, 0, 0, disp++,
            ":icons/map.png",
            _("Biomes 1:256 O.TEMP"),
            _("Allows only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer OCEAN TEMPERATURE with scale 1:256. "
            "This generation layer depends only on the lower 48-bits of the seed.")
        };
        list[F_CLIMATE_NOISE] = FilterInfo{
            CAT_BIOMES, 0, 1, 1, 0, 0, 0, 4, 2, 0, MC_1_18, MC_NEWEST, 0, 0, disp++,
            ":icons/map.png",
            _("Climate parameters 1:4"),
            _("Custom limits for the required and allowed climate noise parameters that "
            "the specified area should cover.")
        };
        list[F_CLIMATE_MINMAX] = FilterInfo{
            CAT_BIOMES, 0, 1, 1, 0, 0, 0, 4, 2, 0, MC_1_18, MC_NEWEST, 0, 0, disp++,
            ":icons/map.png",
            _("Locate climate extreme 1:4"),
            _("Finds the location where a climate parameter reaches its minimum or maximum.")
        };
        list[F_BIOME_CENTER] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 4, 2, 1, MC_B1_7, MC_NEWEST, 0, 1, disp++,
            ":icons/map.png",
            _("Locate biome center 1:4"),
            _("Finds the center position of a given biome.")
        };
        list[F_BIOME_CENTER_256] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 256, 8, 1, MC_B1_7, MC_1_17, 0, 1, disp++,
            ":icons/map.png",
            _("Locate biome center 1:256"),
            _("Finds the center position of a given biome. Based on the 1:256 biome layer.")
        };
        list[F_TEMPS] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 1024, 10, 0, MC_1_7, MC_1_17, 0, 0, disp++,
            ":icons/tempcat.png",
            _("Temperature categories"),
            _("Checks that the area has a minimum of all the required temperature categories.")
        };

        list[F_BIOME_NETHER_1] = FilterInfo{
            CAT_NETHER, 1, 1, 1, 0, 0, 0, 1, 0, 0, MC_1_16_1, 0, -1, 1, disp++, // disabled
            ":icons/nether.png",
            _("Nether biomes 1:1 (disabled)"),
            _("Nether biomes after voronoi scaling to 1:1.")
        };
        list[F_BIOME_NETHER_4] = FilterInfo{
            CAT_NETHER, 0, 1, 1, 0, 0, 0, 4, 2, 0, MC_1_16_1, MC_NEWEST, -1, 0, disp++,
            ":icons/nether.png",
            _("Nether biomes 1:4"),
            _("Nether biomes with normal noise sampling at scale 1:4.")
        };
        list[F_BIOME_NETHER_16] = FilterInfo{
            CAT_NETHER, 0, 1, 1, 0, 0, 0, 16, 4, 0, MC_1_16_1, MC_NEWEST, -1, 0, disp++,
            ":icons/nether.png",
            _("Nether biomes 1:16"),
            _("Nether biomes, but only sampled at scale 1:16.")
        };
        list[F_BIOME_NETHER_64] = FilterInfo{
            CAT_NETHER, 0, 1, 1, 0, 0, 0, 64, 6, 0, MC_1_16_1, MC_NEWEST, -1, 0, disp++,
            ":icons/nether.png",
            _("Nether biomes 1:64"),
            _("Nether biomes, but only sampled at scale 1:64.")
        };
        list[F_BIOME_NETHER_256] = FilterInfo{
            CAT_NETHER, 0, 1, 1, 0, 0, 0, 256, 8, 0, MC_1_16_1, MC_NEWEST, -1, 0, disp++,
            ":icons/nether.png",
            _("Nether biomes 1:256"),
            _("Nether biomes, but only sampled at scale 1:256.")
        };

        list[F_BIOME_END_1] = FilterInfo{
            CAT_END, 1, 1, 1, 0, 0, 0, 1, 0, 0, MC_1_9, 0, +1, 1, disp++, // disabled
            ":icons/the_end.png",
            _("End biomes 1:1 (disabled)"),
            _("End biomes after voronoi scaling to 1:1.")
        };
        list[F_BIOME_END_4] = FilterInfo{
            CAT_END, 0, 1, 1, 0, 0, 0, 4, 2, 0, MC_1_9, MC_NEWEST, +1, 0, disp++,
            ":icons/the_end.png",
            _("End biomes 1:4"),
            _("End biomes sampled at scale 1:4. Note this is just a simple upscale of 1:16.")
        };
        list[F_BIOME_END_16] = FilterInfo{
            CAT_END, 0, 1, 1, 0, 0, 0, 16, 4, 0, MC_1_9, MC_NEWEST, +1, 0, disp++,
            ":icons/the_end.png",
            _("End biomes 1:16"),
            _("End biomes with normal sampling at scale 1:16. ")
        };
        list[F_BIOME_END_64] = FilterInfo{
            CAT_END, 0, 1, 1, 0, 0, 0, 64, 6, 0, MC_1_9, MC_NEWEST, +1, 0, disp++,
            ":icons/the_end.png",
            _("End biomes 1:64"),
            _("End biomes with lossy sampling at scale 1:64. ")
        };

        list[F_SPAWN] = FilterInfo{
            CAT_OTHER, 1, 1, 1, 1, 0, 0, 1, 0, 0, MC_B1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/spawn.png",
            _("Spawn"),
            ""
        };

        list[F_SLIME] = FilterInfo{
            CAT_OTHER, 0, 1, 1, 0, 0, 0, 16, 4, 1, MC_UNDEF, MC_NEWEST, 0, 0, disp++,
            ":icons/slime.png",
            _("Slime chunk"),
            ""
        };
        list[F_HEIGHT] = FilterInfo{
            CAT_OTHER, 0, 1, 0, 0, 0, 0, 4, 2, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/height.png",
            _("Surface height"),
            _("Check the approximate surface height at scale 1:4 at a single coordinate.")
        };

        list[F_FIRST_STRONGHOLD] = FilterInfo{
            CAT_OTHER, 0, 1, 1, 1, 0, 0, 1, 0, 0, MC_B1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/stronghold.png",
            _("First stronghold"),
            _("Finds the approxmiate location of the first stronghold "
            "(+/-112 blocks). Depends only on the 48-bit seed.")
        };

        list[F_STRONGHOLD] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, 0, 1, 0, 1, MC_B1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/stronghold.png",
            _("Stronghold"),
            ""
        };

        list[F_VILLAGE] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Village, 1, 0, 1, MC_B1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/village.png",
            _("Village"),
            ""
        };

        list[F_MINESHAFT] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 0, 0, Mineshaft, 1, 0, 1, MC_B1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/mineshaft.png",
            _("Abandoned mineshaft"),
            ""
        };

        list[F_DESERT] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Desert_Pyramid, 1, 0, 1, MC_1_3, MC_NEWEST, 0, 0, disp++,
            ":icons/desert.png",
            _("Desert pyramid"),
            _("In version 1.18+, desert pyramids depend on surface height and may fail to "
            "generate near caves/aquifers, rivers and oceans.")
        };

        list[F_JUNGLE] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Jungle_Temple, 1, 0, 1, MC_1_3, MC_NEWEST, 0, 0, disp++,
            ":icons/jungle.png",
            _("Jungle temple"),
            _("In version 1.18+, jungle temples depend on surface height and may fail to "
            "generate near caves/aquifers, rivers and oceans.")
        };

        list[F_HUT] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Swamp_Hut, 1, 0, 1, MC_1_4, MC_NEWEST, 0, 0, disp++,
            ":icons/hut.png",
            _("Swamp hut"),
            ""
        };

        list[F_MONUMENT] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Monument, 1, 0, 1, MC_1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/monument.png",
            _("Ocean monument"),
            ""
        };

        list[F_IGLOO] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Igloo, 1, 0, 1, MC_1_9, MC_NEWEST, 0, 0, disp++,
            ":icons/igloo.png",
            _("Igloo"),
            ""
        };

        list[F_MANSION] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Mansion, 1, 0, 1, MC_1_11, MC_NEWEST, 0, 0, disp++,
            ":icons/mansion.png",
            _("Woodland mansion"),
            _("In version 1.18+, mansions depend on surface height and may fail to "
            "generate near caves/aquifers, rivers and oceans.")
        };

        list[F_RUINS] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Ocean_Ruin, 1, 0, 1, MC_1_13, MC_NEWEST, 0, 0, disp++,
            ":icons/ruins.png",
            _("Ocean ruins"),
            ""
        };

        list[F_SHIPWRECK] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Shipwreck, 1, 0, 1, MC_1_13, MC_NEWEST, 0, 0, disp++,
            ":icons/shipwreck.png",
            _("Shipwreck"),
            ""
        };

        list[F_TREASURE] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Treasure, 1, 0, 1, MC_1_13, MC_NEWEST, 0, 0, disp++,
            ":icons/treasure.png",
            _("Buried treasure"),
            _("Buried treasures are always positioned near the center of a chunk "
            "rather than a chunk boarder. Make sure the testing area is set "
            "accordingly.")
        };

        list[F_OUTPOST] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Outpost, 1, 0, 1, MC_1_14, MC_NEWEST, 0, 0, disp++,
            ":icons/outpost.png",
            _("Pillager outpost"),
            ""
        };

        list[F_ANCIENT_CITY] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Ancient_City, 1, 0, 1, MC_1_19, MC_NEWEST, 0, 0, disp++,
            ":icons/ancient_city.png",
            _("Ancient city"),
            ""
        };

        list[F_PORTAL] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, Ruined_Portal, 1, 0, 1, MC_1_16_1, MC_NEWEST, 0, 0, disp++,
            ":icons/portal.png",
            _("Ruined portal (overworld)"),
            ""
        };

        list[F_PORTALN] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, Ruined_Portal_N, 1, 0, 1, MC_1_16_1, MC_NEWEST, -1, 0, disp++,
            ":icons/portal.png",
            _("Ruined portal (nether)"),
            ""
        };

        list[F_FORTRESS] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, Fortress, 1, 0, 1, MC_1_0, MC_NEWEST, -1, 0, disp++,
            ":icons/fortress.png",
            _("Nether fortress"),
            ""
        };

        list[F_BASTION] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, Bastion, 1, 0, 1, MC_1_16_1, MC_NEWEST, -1, 0, disp++,
            ":icons/bastion.png",
            _("Bastion remnant"),
            ""
        };

        list[F_ENDCITY] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, End_City, 1, 0, 1, MC_1_9, MC_NEWEST, +1, 0, disp++,
            ":icons/endcity.png",
            _("End city"),
            ""
        };

        list[F_GATEWAY] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, End_Gateway, 1, 0, 1, MC_1_13, MC_NEWEST, +1, 0, disp++,
            ":icons/gateway.png",
            _("End gateway"),
            _("Checks only scattered return gateways. Does not include those generated "
            "when defeating the dragon.")
        };

#undef _ // translation macro

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
        VER_CURRENT     = VER_2_4_0,
    };
    enum { // meta flags
        DISABLED        = 0x0001,
    };
    enum { // condition flags
        FLG_APPROX      = 0x0001,
        FLG_MATCH_ANY   = 0x0010,
        FLG_INRANGE     = 0x0020,
    };
    enum { // variant flags
        VAR_WITH_START  = 0x01, // restrict start piece index and biome
        VAR_ABANODONED  = 0x02, // zombie village
        VAR_ENDSHIP     = 0x04, // end city ship
        VAR_DENSE_BB    = 0x08, // fortress with a 2x2 arrangement of start/crossings
        VAR_NOT         = 0x10, // invert flag (e.g. not abandoned)
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
    uint8_t     deps[16]; // currently unused
    uint64_t    biomeToFind, biomeToFindM; // inclusion biomes
    int32_t     biomeId; // legacy oceanToFind(8)
    uint32_t    biomeSize;
    uint8_t     tol; // legacy specialCnt(4)
    uint8_t     minmax;
    uint8_t     para;
    uint8_t     octave;
    uint8_t     pad2[2]; // legacy zero initialized
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
    float       value;

    // generated members - initialized when the search is started
    uint8_t     generated_start[0]; // address dummy
    BiomeFilter bf;

    // perform version upgrades
    bool versionUpgrade();

    // initialize the generated members
    QString apply(int mc);

    QString toHex() const;
    bool readHex(const QString& hex);

    QString summary() const;
};

static_assert(
    offsetof(Condition, generated_start) == 308,
    "Layout of Condition has changed!"
);

struct ConditionTree
{
    QVector<Condition> condvec;
    std::vector<std::vector<char>> references;

    ~ConditionTree();
    QString set(const QVector<Condition>& cv, int mc);
};

struct SearchThreadEnv
{
    ConditionTree *condtree;

    Generator g;
    SurfaceNoise sn;

    int mc, large;
    uint64_t seed;
    int surfdim;
    int octaves;

    std::map<uint64_t, lua_State*> l_states;

    SearchThreadEnv();
    ~SearchThreadEnv();

    QString init(int mc, bool large, ConditionTree *condtree);

    void setSeed(uint64_t seed);
    void init4Dim(int dim);
    void init4Noise(int nptype, int octaves);
    void prepareSurfaceNoise(int dim);
};


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

/* Checks if a seed satisfies the conditions tree.
 * Returns the lowest condition fulfillment status.
 */
int testTreeAt(
    Pos                         at,             // relative origin
    SearchThreadEnv           * env,            // thread-local environment
    int                         pass,           // search pass
    std::atomic_bool          * abort,          // abort signal
    Pos                       * path = 0        // ok trigger positions
);

int testCondAt(
    Pos                         at,             // relative origin
    SearchThreadEnv           * env,            // thread-local environment
    int                         pass,           // search pass
    std::atomic_bool          * abort,          // abort signal
    Pos                       * cent,           // output center position(s)
    int                       * imax,           // max instances (NULL for avg)
    Condition                 * cond            // condition to check
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
