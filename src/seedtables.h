#ifndef SEEDTABLES_H
#define SEEDTABLES_H

#include <inttypes.h>

// Quad monument bases are too expensive to generate on the fly and there are
// so few of them that they can be hard coded, rather than loading from a file.
static const int64_t g_qm_90[] = {
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

static const int64_t g_qm_95[] = {
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

#endif // SEEDTABLES_H
