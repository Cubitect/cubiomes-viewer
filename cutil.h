#ifndef CUTIL_H
#define CUTIL_H

#include <QMutex>
#include <QString>

#include <random>

#include "cubiomes/finders.h"
#include "cubiomes/util.h"

extern unsigned char biomeColors[256][3];
extern unsigned char tempsColors[256][3];


inline const char* mc2str(int mc)
{
    switch (mc)
    {
    case MC_1_6:  return "1.6"; break;
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
    if (!strcmp(s, "1.6")) return MC_1_6;
    return -1;
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
inline int str2seed(const QString &str, int64_t *out)
{
    if (str.isEmpty())
    {
        *out = getRnd64();
        return S_RANDOM;
    }

    bool ok = false;
    *out = str.toLongLong(&ok);
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

#endif // CUTIL_H
