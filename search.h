#ifndef SEARCH_H
#define SEARCH_H

#include "cubiomes/finders.h"


enum
{
    CAT_NONE,
    CAT_48,
    CAT_FULL,
};

struct FilterInfo
{
    int cat;
    bool coord;
    bool area;
    bool biomes;
    int step;
    int count;
    const char *icon;
    const char *name;
    const char *desription;
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
    F_TEMP,
    F_BIOME,
    F_SPAWN,
    F_STRONGHOLD,
    F_DESERT,
    F_JUNGLE,
    F_HUT,
    F_IGLOO,
    F_MONUMENT,
    F_VILLAGE,
    F_OUTPOST,
    FILTER_MAX,
};

// global table of filter data (as constants with enum indexing)
static const struct FilterList
{
    FilterInfo list[FILTER_MAX];

    FilterList() : list{}
    {
        list[F_SELECT] = FilterInfo{
            CAT_NONE, 0, 0, 0, 0, 0,
            NULL,
            "",
            ""
        };

        list[F_QH_IDEAL] = FilterInfo{
            CAT_48, 1, 1, 0, 512, 0,
            NULL,
            "Quad-hut (ideal)",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in one of the best configurations that exist."
        };

        list[F_QH_CLASSIC] = FilterInfo{
            CAT_48, 1, 1, 0, 512, 0,
            NULL,
            "Quad-hut (classic)",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in one of the \"classic\" configurations. "
            "(Checks for huts in the nearest 2x2 chunk corners of each "
            "region.)"
        };

        list[F_QH_NORMAL] = FilterInfo{
            CAT_48, 1, 1, 0, 512, 0,
            NULL,
            "Quad-hut (normal)",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, such that all of them are within 128 blocks "
            "of a single AFK location, including a vertical tollerance "
            "for a fall damage chute."
        };

        list[F_QH_BARELY] = FilterInfo{
            CAT_48, 1, 1, 0, 512, 0,
            NULL,
            "Quad-hut (barely)",
            "The lower 48-bits provide potential for four swamp huts in "
            "spawning range, in any configuration, such that the bounding "
            "boxes are within 128 blocks of a single AFK location."
        };

        list[F_QM_95] = FilterInfo{
            CAT_48, 1, 1, 0, 512, 0,
            NULL,
            "Quad-ocean-monument (>95%)",
            "The lower 48-bits provide potential for 95% of the area of "
            "four ocean monuments to be within 128 blocks of an AFK "
            "location."
        };

        list[F_QM_90] = FilterInfo{
            CAT_48, 1, 1, 0, 512, 0,
            NULL,
            "Quad-ocean-monument (>90%)",
            "The lower 48-bits provide potential for 90% of the area of "
            "four ocean monuments to be within 128 blocks of an AFK "
            "location."
        };

        list[F_TEMP] = FilterInfo{
            CAT_FULL, 1, 0, 0, 1024, 0,
            NULL,
            "All temperatures cluster",
            "Checks that the seed has all temperature categories "
            "(Oceanic, Warm, Lush, Cold, Freezing, and special variants) "
            "in a 3x3 cluster at layer scale 1024. (Input position "
            "represents the center of diversity.)"
        };

        list[F_BIOME] = FilterInfo{
            CAT_FULL, 1, 1, 1, 1, 0,
            NULL,
            "Biome filter",
            "Discard seeds that do not have the required biomes inside "
            "the specified area."
        };

        list[F_SPAWN] = FilterInfo{
            CAT_FULL, 1, 1, 0, 1, 0,
            ":icons/spawn.png",
            "Spawn",
            ""
        };

        list[F_STRONGHOLD] = FilterInfo{
            CAT_FULL, 1, 1, 0, 1, 1,
            ":icons/stronghold.png",
            "Stronghold",
            ""
        };

        list[F_DESERT] = FilterInfo{
            CAT_FULL, 1, 1, 0, 512, 1,
            ":icons/desert.png",
            "Desert pyramid",
            ""
        };

        list[F_JUNGLE] = FilterInfo{
            CAT_FULL, 1, 1, 0, 512, 1,
            ":icons/jungle.png",
            "Jungle temple",
            ""
        };

        list[F_HUT] = FilterInfo{
            CAT_FULL, 1, 1, 0, 512, 1,
            ":icons/hut.png",
            "Swamp hut",
            ""
        };

        list[F_IGLOO] = FilterInfo{
            CAT_FULL, 1, 1, 0, 512, 1,
            ":icons/igloo.png",
            "Igloo",
            ""
        };

        list[F_MONUMENT] = FilterInfo{
            CAT_FULL, 1, 1, 0, 512, 1,
            ":icons/monument.png",
            "Ocean monument",
            ""
        };

        list[F_VILLAGE] = FilterInfo{
            CAT_FULL, 1, 1, 0, 512, 1,
            ":icons/village.png",
            "Village",
            ""
        };

        list[F_OUTPOST] = FilterInfo{
            CAT_FULL, 1, 1, 0, 512, 1,
            ":icons/outpost.png",
            "Pillager outpost",
            ""
        };
    }
}
g_filterinfo;


struct Condition
{
    int type;
    int x1, z1, x2, z2;
    int rx, rz, rw, rh;
    int save;
    int relative;
    BiomeFilter bfilter;
    int count;
};


struct Candidate
{
    struct __attribute__((packed)) SPos
    {
        StructureConfig sconf;
        int x, z;
    };

    // a candidate is a seed base and the structure positions it encompases
    int64_t seed;
    SPos spos[];
};

struct CandidateList
{
    union {
        Candidate *items; // list of candidate items
        char *mem;
    };
    int64_t isiz; // sizeof Candidate, each with scnt structures
    int64_t scnt; // structures in each base item
    int64_t bcnt; // number of base items
};



/* Attempts to construct a list of 48-bit bases that should be further checked.
 * Any conditions that would result in a list larger than a buffer size will
 * not be preloaded in this way.
 */
CandidateList getCandidates(int mc, const Condition *cond, int ccnt, int64_t bufmax);


struct StructPos
{
    StructureConfig sconf;
    int cx, cz; // effective center position
};


int testCond(StructPos *spos, int64_t seed, const Condition *cond, int mc, LayerStack *g, volatile bool *abort);

size_t searchFamily(int64_t seedbuf[], int64_t s, int scnt, int mc,
        LayerStack *g, const Condition cond[], int ccnt, StructPos *spos, volatile bool *abort);



#endif // SEARCH_H
