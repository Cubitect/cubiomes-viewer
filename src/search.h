#ifndef SEARCH_H
#define SEARCH_H

#include "cubiomes/finders.h"

#include <QVector>
#include <QString>
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
    F_REFERENCE_1,
    F_REFERENCE_16,
    F_REFERENCE_64,
    F_REFERENCE_256,
    F_REFERENCE_512,
    F_REFERENCE_1024,
    F_BIOME_4, // differs from F_BIOME_4_RIVER, since this may include oceans
    F_SCALE_TO_NETHER,
    F_SCALE_TO_OVERWORLD,
    F_LOGIC_OR,
    F_REFERENCE_4,
    F_FIRST_STRONGHOLD,
    F_CLIMATE_NOISE,
    // new filters should be added here at the end to keep some downwards compatibility
    FILTER_MAX,
};

// global table of filter data (as constants with enum indexing)
static const struct FilterList
{
    FilterInfo list[FILTER_MAX];

    FilterList() : list{}
    {
        int disp = 0; // display order

        list[F_SELECT] = FilterInfo{
            CAT_NONE, 0, 0, 0, 0, 0, 0, 0, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            NULL,
            "",
            ""
        };

#define _(S) QT_TRANSLATE_NOOP("Filter", S)

        list[F_LOGIC_OR] = FilterInfo{
            CAT_HELPER, 0, 0, 0, 0, 0, 0, 1, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/helper.png",
            _("OR logic gate"),
            _("This helper is satisfied iff any of the conditions which use it "
            "for a relative reference are met.")
        };
        list[F_SCALE_TO_NETHER] = FilterInfo{
            CAT_HELPER, 0, 0, 0, 0, 0, 0, 1, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/portal.png",
            _("Coordinate factor x/8"),
            _("Divides relative location by 8, from Overworld to Nether.")
        };
        list[F_SCALE_TO_OVERWORLD] = FilterInfo{
            CAT_HELPER, 0, 0, 0, 0, 0, 0, 1, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/portal.png",
            _("Coordinate factor x*8"),
            _("Multiplies relative location by 8, from Nether to Overworld.")
        };
        const char *ref_desc = _(
            "<html><head/><body>"
            "Reference points can be used to iterate over an area with a "
            "certain step size."
            "</body></html>"
        );
        list[F_REFERENCE_1] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 1, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/reference.png",
            _("Reference point 1:1"),
            ref_desc
        };
        list[F_REFERENCE_4] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 4, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/reference.png",
            _("Reference point 1:4"),
            ref_desc
        };
        list[F_REFERENCE_16] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 16, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/reference.png",
            _("Reference point 1:16"),
            ref_desc
        };
        list[F_REFERENCE_64] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 64, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/reference.png",
            _("Reference point 1:64"),
            ref_desc
        };
        list[F_REFERENCE_256] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 256, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/reference.png",
            _("Reference point 1:256"),
            ref_desc
        };
        list[F_REFERENCE_512] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 512, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/reference.png",
            _("Reference point 1:512"),
            ref_desc
        };
        list[F_REFERENCE_1024] = FilterInfo{
            CAT_HELPER, 0, 1, 1, 0, 0, 0, 1024, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/reference.png",
            _("Reference point 1:1024"),
            ref_desc
        };

        list[F_QH_IDEAL] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Swamp_Hut, 512, 0, MC_1_4, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-hut (ideal)"),
            _("The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in one of the best configurations that exist.")
        };

        list[F_QH_CLASSIC] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Swamp_Hut, 512, 0, MC_1_4, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-hut (classic)"),
            _("The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in one of the \"classic\" configurations. "
            "(Checks for huts in the nearest 2x2 chunk corners of each "
            "region.)")
        };

        list[F_QH_NORMAL] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Swamp_Hut, 512, 0, MC_1_4, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-hut (normal)"),
            _("The lower 48-bits provide potential for four swamp huts in "
            "spawning range, such that all of them are within 128 blocks "
            "of a single AFK location, including a vertical tollerance "
            "for a fall damage chute.")
        };

        list[F_QH_BARELY] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Swamp_Hut, 512, 0, MC_1_4, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-hut (barely)"),
            _("The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in any configuration, such that the bounding "
            "boxes are within 128 blocks of a single AFK location.")
        };

        list[F_QM_95] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Monument, 512, 0, MC_1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-ocean-monument (>95%)"),
            _("The lower 48-bits provide potential for 95% of the area of "
            "four ocean monuments to be within 128 blocks of an AFK "
            "location.")
        };

        list[F_QM_90] = FilterInfo{
            CAT_QUAD, 0, 1, 1, 0, 0, Monument, 512, 0, MC_1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/quad.png",
            _("Quad-ocean-monument (>90%)"),
            _("The lower 48-bits provide potential for 90% of the area of "
            "four ocean monuments to be within 128 blocks of an AFK "
            "location.")
        };

        list[F_BIOME] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, L_VORONOI_1, 0, 1, 0, MC_1_0, MC_1_17, 0, 1, disp++, // disable for 1.18
            ":icons/map.png",
            _("Biome filter 1:1"),
            _("Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };

        list[F_BIOME_4] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 4, 0, MC_1_0, MC_NEWEST, 0, 1, disp++,
            ":icons/map.png",
            _("Biome filter 1:4"),
            _("Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };
        list[F_BIOME_16] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 16, 0, MC_1_0, MC_NEWEST, 0, 1, disp++,
            ":icons/map.png",
            _("Biome filter 1:16"),
            _("Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };
        list[F_BIOME_64] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 64, 0, MC_1_0, MC_NEWEST, 0, 1, disp++,
            ":icons/map.png",
            _("Biome filter 1:64"),
            _("Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };
        list[F_BIOME_256] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 256, 0, MC_1_0, MC_NEWEST, 0, 1, disp++,
            ":icons/map.png",
            _("Biome filter 1:256"),
            _("Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-).")
        };

        list[F_BIOME_4_RIVER] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, L_RIVER_MIX_4, 0, 4, 0, MC_1_13, MC_1_17, 0, 0, disp++,
            ":icons/map.png",
            _("Biome filter 1:4 RIVER"),
            _("Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer RIVER with scale 1:4. "
            "This layer does not generate ocean variants.")
        };
        list[F_BIOME_256_OTEMP] = FilterInfo{
            CAT_BIOMES, 0, 1, 1, 0, L_OCEAN_TEMP_256, 0, 256, 0, MC_1_13, MC_1_17, 0, 0, disp++,
            ":icons/map.png",
            _("Biome filter 1:256 O.TEMP"),
            _("Only seeds with the included (+) biomes in the specified area and "
            "discard those that have biomes that are explicitly excluded (-) "
            "at layer OCEAN TEMPERATURE with scale 1:256. "
            "This generation layer depends only on the lower 48-bits of the seed.")
        };
        list[F_CLIMATE_NOISE] = FilterInfo{
            CAT_BIOMES, 0, 1, 1, 0, 0, 0, 4, 0, MC_1_18, MC_NEWEST, 0, 0, disp++,
            ":icons/map.png",
            _("Climate Parameters 1:4"),
            _("Custom limits for the required and allowed climate noise parameters that "
            "the specified area should cover.")
        };
        list[F_TEMPS] = FilterInfo{
            CAT_BIOMES, 1, 1, 1, 0, 0, 0, 1024, 0, MC_1_7, MC_1_17, 0, 0, disp++,
            ":icons/tempcat.png",
            _("Temperature categories"),
            _("Checks that the area has a minimum of all the required temperature categories.")
        };

        list[F_BIOME_NETHER_1] = FilterInfo{
            CAT_NETHER, 1, 1, 1, 0, 0, 0, 1, 0, MC_1_16, 0, -1, 1, disp++, // disabled
            ":icons/nether.png",
            _("Nether biome filter 1:1 (disabled)"),
            _("Nether biomes after voronoi scaling to 1:1.")
        };
        list[F_BIOME_NETHER_4] = FilterInfo{
            CAT_NETHER, 0, 1, 1, 0, 0, 0, 4, 0, MC_1_16, MC_NEWEST, -1, 0, disp++,
            ":icons/nether.png",
            _("Nether biome filter 1:4"),
            _("Nether biomes with normal noise sampling at scale 1:4.")
        };
        list[F_BIOME_NETHER_16] = FilterInfo{
            CAT_NETHER, 0, 1, 1, 0, 0, 0, 16, 0, MC_1_16, MC_NEWEST, -1, 0, disp++,
            ":icons/nether.png",
            _("Nether biome filter 1:16"),
            _("Nether biomes, but only sampled at scale 1:16.")
        };
        list[F_BIOME_NETHER_64] = FilterInfo{
            CAT_NETHER, 0, 1, 1, 0, 0, 0, 64, 0, MC_1_16, MC_NEWEST, -1, 0, disp++,
            ":icons/nether.png",
            _("Nether biome filter 1:64"),
            _("Nether biomes, but only sampled at scale 1:64.")
        };
        list[F_BIOME_NETHER_256] = FilterInfo{
            CAT_NETHER, 0, 1, 1, 0, 0, 0, 256, 0, MC_1_16, MC_NEWEST, -1, 0, disp++,
            ":icons/nether.png",
            _("Nether biome filter 1:256"),
            _("Nether biomes, but only sampled at scale 1:256.")
        };

        list[F_BIOME_END_1] = FilterInfo{
            CAT_END, 1, 1, 1, 0, 0, 0, 1, 0, MC_1_9, 0, +1, 1, disp++, // disabled
            ":icons/the_end.png",
            _("End biome filter 1:1 (disabled)"),
            _("End biomes after voronoi scaling to 1:1.")
        };
        list[F_BIOME_END_4] = FilterInfo{
            CAT_END, 0, 1, 1, 0, 0, 0, 4, 0, MC_1_9, MC_NEWEST, +1, 0, disp++,
            ":icons/the_end.png",
            _("End biome filter 1:4"),
            _("End biomes sampled at scale 1:4. Note this is just a simple upscale of 1:16.")
        };
        list[F_BIOME_END_16] = FilterInfo{
            CAT_END, 0, 1, 1, 0, 0, 0, 16, 0, MC_1_9, MC_NEWEST, +1, 0, disp++,
            ":icons/the_end.png",
            _("End biome filter 1:16"),
            _("End biomes with normal sampling at scale 1:16. ")
        };
        list[F_BIOME_END_64] = FilterInfo{
            CAT_END, 0, 1, 1, 0, 0, 0, 64, 0, MC_1_9, MC_NEWEST, +1, 0, disp++,
            ":icons/the_end.png",
            _("End biome filter 1:64"),
            _("End biomes with lossy sampling at scale 1:64. ")
        };

        list[F_SPAWN] = FilterInfo{
            CAT_OTHER, 1, 1, 1, 1, 0, 0, 1, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/spawn.png",
            _("Spawn"),
            ""
        };

        list[F_SLIME] = FilterInfo{
            CAT_OTHER, 0, 1, 1, 0, 0, 0, 16, 1, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/slime.png",
            _("Slime chunk"),
            ""
        };

        list[F_FIRST_STRONGHOLD] = FilterInfo{
            CAT_OTHER, 0, 1, 1, 1, 0, 0, 1, 0, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/stronghold.png",
            _("First stronghold"),
            _("Finds the approxmiate location of the first stronghold "
            "(+/-112 blocks). Depends only on the 48-bit seed.")
        };

        list[F_STRONGHOLD] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, 0, 1, 1, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/stronghold.png",
            _("Stronghold"),
            ""
        };

        list[F_VILLAGE] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Village, 1, 1, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/village.png",
            _("Village"),
            ""
        };

        list[F_MINESHAFT] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 0, 0, Mineshaft, 1, 1, MC_1_0, MC_NEWEST, 0, 0, disp++,
            ":icons/mineshaft.png",
            _("Abandoned mineshaft"),
            ""
        };

        list[F_DESERT] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Desert_Pyramid, 1, 1, MC_1_3, MC_NEWEST, 0, 0, disp++,
            ":icons/desert.png",
            _("Desert pyramid"),
            _("In version 1.18, desert pyramids depend on surface height and may fail to "
            "generate near caves/aquifers, rivers and oceans.")
        };

        list[F_JUNGLE] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Jungle_Temple, 1, 1, MC_1_3, MC_NEWEST, 0, 0, disp++,
            ":icons/jungle.png",
            _("Jungle temple"),
            _("In version 1.18, jungle temples depend on surface height and may fail to "
            "generate near caves/aquifers, rivers and oceans.")
        };

        list[F_HUT] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Swamp_Hut, 1, 1, MC_1_4, MC_NEWEST, 0, 0, disp++,
            ":icons/hut.png",
            _("Swamp hut"),
            ""
        };

        list[F_MONUMENT] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Monument, 1, 1, MC_1_8, MC_NEWEST, 0, 0, disp++,
            ":icons/monument.png",
            _("Ocean monument"),
            ""
        };

        list[F_IGLOO] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Igloo, 1, 1, MC_1_9, MC_NEWEST, 0, 0, disp++,
            ":icons/igloo.png",
            _("Igloo"),
            ""
        };

        list[F_MANSION] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Mansion, 1, 1, MC_1_11, MC_NEWEST, 0, 0, disp++,
            ":icons/mansion.png",
            _("Woodland mansion"),
            _("In version 1.18, mansions depend on surface height and may fail to "
            "generate near caves/aquifers, rivers and oceans.")
        };

        list[F_RUINS] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Ocean_Ruin, 1, 1, MC_1_13, MC_NEWEST, 0, 0, disp++,
            ":icons/ruins.png",
            _("Ocean ruins"),
            ""
        };

        list[F_SHIPWRECK] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Shipwreck, 1, 1, MC_1_13, MC_NEWEST, 0, 0, disp++,
            ":icons/shipwreck.png",
            _("Shipwreck"),
            ""
        };

        list[F_TREASURE] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Treasure, 1, 1, MC_1_13, MC_NEWEST, 0, 0, disp++,
            ":icons/treasure.png",
            _("Buried treasure"),
            _("Buried treasures are always positioned near the center of a chunk "
            "rather than a chunk boarder. Make sure the testing area is set "
            "accordingly.")
        };

        list[F_OUTPOST] = FilterInfo{
            CAT_STRUCT, 1, 1, 1, 1, 0, Outpost, 1, 1, MC_1_14, MC_NEWEST, 0, 0, disp++,
            ":icons/outpost.png",
            _("Pillager outpost"),
            ""
        };

        list[F_PORTAL] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, Ruined_Portal, 1, 1, MC_1_16, MC_NEWEST, 0, 0, disp++,
            ":icons/portal.png",
            _("Ruined portal (overworld)"),
            ""
        };

        list[F_PORTALN] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, Ruined_Portal_N, 1, 1, MC_1_16, MC_NEWEST, -1, 0, disp++,
            ":icons/portal.png",
            _("Ruined portal (nether)"),
            ""
        };

        list[F_FORTRESS] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, Fortress, 1, 1, MC_1_0, MC_NEWEST, -1, 0, disp++,
            ":icons/fortress.png",
            _("Nether fortress"),
            ""
        };

        list[F_BASTION] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, Bastion, 1, 1, MC_1_16, MC_NEWEST, -1, 0, disp++,
            ":icons/bastion.png",
            _("Bastion remnant"),
            ""
        };

        list[F_ENDCITY] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, End_City, 1, 1, MC_1_9, MC_NEWEST, +1, 0, disp++,
            ":icons/endcity.png",
            _("End city"),
            ""
        };

        list[F_GATEWAY] = FilterInfo{
            CAT_STRUCT, 0, 1, 1, 1, 0, End_Gateway, 1, 1, MC_1_13, MC_NEWEST, +1, 0, disp++,
            ":icons/gateway.png",
            _("End gateway"),
            _("Scattered end gateway return portals, not including those "
            "generated when defeating the dragon.")
        };

#undef _ // translation macro

    }
}
g_filterinfo;

struct /*__attribute__((packed))*/ Condition
{   // data - needs to supports memset/memcpy and memory layout should be
    // consistent across versions
    enum { // meta flags
        DISABLED = 0x0001,
    };
    enum { // variant flags
        START_PIECE_MASK = (1ULL << 63),
        ABANDONED_MASK   = (1ULL << 62),
    };
    int16_t     type;
    uint16_t    meta;
    int32_t     x1, z1, x2, z2;
    int32_t     save;
    int32_t     relative;
    uint32_t    pad1; // unused
    BiomeFilter bfilter;
    int32_t     temps[9];
    int32_t     count;
    int32_t     y;
    uint32_t    flags;
    int32_t     rmax; // (<=0):disabled; (>0):strict upper radius
    uint32_t    pad2; // unused
    uint64_t    variants;
    int32_t     limok[6][2];
    int32_t     limex[6][2];

    QString toHex() const;
    bool readHex(const QString& hex);

    QString summary() const;

    static int toVariantBit(int biome, int variant);
    static void fromVariantBit(int bit, int *biome, int *variant);
    bool villageOk(int mc, StructureVariant sv) const;
};

struct ConditionTree
{
    QVector<Condition> condvec;
    QVector<QVector<char>> references;

    void set(const QVector<Condition>& cv);
};

struct StructPos
{
    StructureConfig sconf;
    int cx, cz; // effective center position
};


struct WorldGen
{
    Generator g;
    SurfaceNoise sn;

    int mc, large;
    uint64_t seed;
    bool initsurf;

    void init(int mc, bool large)
    {
        this->mc = mc;
        this->large = large;
        this->seed = 0;
        this->initsurf = false;
        setupGenerator(&g, mc, large);
    }

    void setSeed(uint64_t seed)
    {
        this->seed = seed;
    }

    void init4Dim(int dim)
    {
        uint64_t mask = (dim == 0 ? ~0ULL : MASK48);
        if (dim != g.dim || (seed & mask) != (g.seed & mask))
        {
            applySeed(&g, dim, seed);
            initsurf = false;
        }
        else if (g.mc >= MC_1_15 && seed != g.seed)
        {
            g.sha = getVoronoiSHA(seed);
        }
    }

    void setSurfaceNoise()
    {
        if (!initsurf)
        {
            initSurfaceNoiseEnd(&sn, seed);
            initsurf = true;
        }
    }
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

/* Checks if a seeds satisfies the conditions tree.
 * Returns the lowest condition fulfillment status.
 */
int testTreeAt(
    Pos                         at,             // relative origin
    ConditionTree             * tree,           // condition tree
    int                         pass,           // search pass
    WorldGen                  * gen,            // world generator
    std::atomic_bool          * abort,          // abort signal
    Pos                       * path = 0        // ok trigger positions
);

int testCondAt(
    Pos                         at,             // relative origin
    Pos                       * cent,           // output center position(s)
    int                       * imax,           // max instances (NULL for avg)
    Condition                 * cond,           // condition to check
    int                         pass,
    WorldGen                  * gen,
    std::atomic_bool          * abort
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
