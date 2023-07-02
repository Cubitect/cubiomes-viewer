#include "config.h"
#include "util.h"
#include "seedtables.h"

#include <QThread>
#include <QCoreApplication>
#include <QFontDatabase>


/// globals

unsigned char g_biomeColors[256][3];
unsigned char g_tempsColors[256][3];

ExtGenConfig g_extgen;

qreal g_fontscale = 1;
qreal g_iconscale = 1;


void ExtGenConfig::reset()
{
    experimentalVers = false;
    estimateTerrain = true;
    saltOverride = false;
    for (int i = 0; i < FEATURE_NUM; i++)
        salts[i] = ~(uint64_t)0;
}

void ExtGenConfig::load(QSettings& settings)
{
    experimentalVers = settings.value("world/experimentalVers", experimentalVers).toBool();
    estimateTerrain = settings.value("world/estimateTerrain", estimateTerrain).toBool();
    saltOverride = settings.value("world/saltOverride", saltOverride).toBool();
    for (int st = 0; st < FEATURE_NUM; st++)
    {
        QVariant v = QVariant::fromValue(~(qulonglong)0);
        salts[st] = settings.value(QString("world/salt_") + struct2str(st), v).toULongLong();
    }
}

void ExtGenConfig::save(QSettings& settings)
{
    settings.setValue("world/experimentalVers", experimentalVers);
    settings.setValue("world/estimateTerrain", estimateTerrain);
    settings.setValue("world/saltOverride", saltOverride);
    for (int st = 0; st < FEATURE_NUM; st++)
    {
        uint64_t salt = salts[st];
        if (salt <= MASK48)
            settings.setValue(QString("world/salt_") + struct2str(st), (qulonglong)salt);
    }
}

bool WorldInfo::equals(const WorldInfo& wi) const
{
    return mc == wi.mc && large == wi.large && seed == wi.seed && y == wi.y;
}

void WorldInfo::reset()
{
    mc = MC_NEWEST;
    large = false;
    seed = 0;
    y = 255;
}

void WorldInfo::load(QSettings& settings)
{
    mc = settings.value("map/mc", mc).toInt();
    large = settings.value("map/large", large).toBool();
    seed = settings.value("map/seed", (qlonglong)seed).toLongLong();
    y = settings.value("map/y", y).toInt();
}

void WorldInfo::save(QSettings& settings)
{
    settings.setValue("map/mc", mc);
    settings.setValue("map/large", large);
    settings.setValue("map/seed", (qlonglong)seed);
    settings.setValue("map/y", y);
}

bool WorldInfo::read(const QString& line)
{
    QByteArray ba = line.toLocal8Bit();
    const char *p = ba.data();
    char buf[9];
    int tmp;
    if (sscanf(p, "#MC:       %8[^\n]", buf) == 1)
    {
        mc = str2mc(buf);
        if (mc < 0)
            mc = MC_NEWEST;
        return true;
    }
    if (sscanf(p, "#Large:    %d", &tmp) == 1)
    {
        large = tmp;
        return true;
    }
    return false;
}

void WorldInfo::write(QTextStream& stream)
{
    stream << "#MC:       " << mc2str(mc) << "\n";
    stream << "#Large:    " << large << "\n";
}

void LayerOpt::reset()
{
    mode = LOPT_BIOMES;
    memset(disp, 0, sizeof(disp));
}

int LayerOpt::activeDisp() const
{
    return disp[mode];
}

bool LayerOpt::activeDifference(const LayerOpt& l) const
{
    return mode != l.mode || disp[l.mode] != l.disp[l.mode];
}

bool LayerOpt::isClimate(int mc) const
{
    if (mc <= MC_B1_7)
        return mode == LOPT_BETA_T_1 || mode == LOPT_BETA_H_1;
    if (mc <= MC_1_17)
        return false;
    return mode >= LOPT_NOISE_T_4 && mode <= LOPT_NOISE_W_4;
}

QString mapopt2display(int opt)
{
    switch (opt)
    {
    case D_GRID:        return QApplication::translate("Map", "Grid");
    case D_SLIME:       return QApplication::translate("Map", "Slime Chunks");
    case D_DESERT:      return QApplication::translate("Map", "Desert Pyramid");
    case D_JUNGLE:      return QApplication::translate("Map", "Jungle Temple");
    case D_HUT:         return QApplication::translate("Map", "Swamp Hut");
    case D_IGLOO:       return QApplication::translate("Map", "Igloo");
    case D_VILLAGE:     return QApplication::translate("Map", "Village");
    case D_MANSION:     return QApplication::translate("Map", "Woodland Mansion");
    case D_MONUMENT:    return QApplication::translate("Map", "Ocean Monument");
    case D_RUINS:       return QApplication::translate("Map", "Ocean Ruins");
    case D_SHIPWRECK:   return QApplication::translate("Map", "Shipwreck");
    case D_TREASURE:    return QApplication::translate("Map", "Buried Treasure");
    case D_MINESHAFT:   return QApplication::translate("Map", "Mineshaft");
    case D_WELL:        return QApplication::translate("Map", "Desert Well");
    case D_GEODE:       return QApplication::translate("Map", "Geode");
    case D_OUTPOST:     return QApplication::translate("Map", "Pillager Outpost");
    case D_ANCIENTCITY: return QApplication::translate("Map", "Ancient City");
    case D_TRAILS:      return QApplication::translate("Map", "Trail Ruins");
    case D_PORTAL:      return QApplication::translate("Map", "Ruined Portal");
    case D_PORTALN:     return QApplication::translate("Map", "Ruined Portal (Nether)");
    case D_SPAWN:       return QApplication::translate("Map", "Spawn");
    case D_STRONGHOLD:  return QApplication::translate("Map", "Stronghold");
    case D_FORTESS:     return QApplication::translate("Map", "Nether Fortress");
    case D_BASTION:     return QApplication::translate("Map", "Bastion Remnant");
    case D_ENDCITY:     return QApplication::translate("Map", "End City");
    case D_GATEWAY:     return QApplication::translate("Map", "End Gateway");
    default: return "";
    }
}

const char *mapopt2str(int opt) // to resource string
{
    switch (opt)
    {
    case D_GRID:        return "grid";
    case D_SLIME:       return "slime";
    case D_DESERT:      return "desert";
    case D_JUNGLE:      return "jungle";
    case D_IGLOO:       return "igloo";
    case D_HUT:         return "hut";
    case D_VILLAGE:     return "village";
    case D_MANSION:     return "mansion";
    case D_MONUMENT:    return "monument";
    case D_RUINS:       return "ruins";
    case D_SHIPWRECK:   return "shipwreck";
    case D_TREASURE:    return "treasure";
    case D_MINESHAFT:   return "mineshaft";
    case D_WELL:        return "well";
    case D_GEODE:       return "geode";
    case D_OUTPOST:     return "outpost";
    case D_ANCIENTCITY: return "ancient_city";
    case D_TRAILS:      return "trails";
    case D_PORTAL:      return "portal";
    case D_PORTALN:     return "portaln";
    case D_SPAWN:       return "spawn";
    case D_STRONGHOLD:  return "stronghold";
    case D_FORTESS:     return "fortress";
    case D_BASTION:     return "bastion";
    case D_ENDCITY:     return "endcity";
    case D_GATEWAY:     return "gateway";
    default:            return "";
    }
}

int str2mapopt(const char *s) // from resource string
{
    if (!strcmp(s, "grid"))         return D_GRID;
    if (!strcmp(s, "slime"))        return D_SLIME;
    if (!strcmp(s, "desert"))       return D_DESERT;
    if (!strcmp(s, "jungle"))       return D_JUNGLE;
    if (!strcmp(s, "igloo"))        return D_IGLOO;
    if (!strcmp(s, "hut"))          return D_HUT;
    if (!strcmp(s, "village"))      return D_VILLAGE;
    if (!strcmp(s, "mansion"))      return D_MANSION;
    if (!strcmp(s, "monument"))     return D_MONUMENT;
    if (!strcmp(s, "ruins"))        return D_RUINS;
    if (!strcmp(s, "shipwreck"))    return D_SHIPWRECK;
    if (!strcmp(s, "treasure"))     return D_TREASURE;
    if (!strcmp(s, "mineshaft"))    return D_MINESHAFT;
    if (!strcmp(s, "well"))         return D_WELL;
    if (!strcmp(s, "geode"))        return D_GEODE;
    if (!strcmp(s, "outpost"))      return D_OUTPOST;
    if (!strcmp(s, "ancient_city")) return D_ANCIENTCITY;
    if (!strcmp(s, "trails"))       return D_TRAILS;
    if (!strcmp(s, "portal"))       return D_PORTAL;
    if (!strcmp(s, "portaln"))      return D_PORTALN;
    if (!strcmp(s, "spawn"))        return D_SPAWN;
    if (!strcmp(s, "stronghold"))   return D_STRONGHOLD;
    if (!strcmp(s, "fortress"))     return D_FORTESS;
    if (!strcmp(s, "bastion"))      return D_BASTION;
    if (!strcmp(s, "endcity"))      return D_ENDCITY;
    if (!strcmp(s, "gateway"))      return D_GATEWAY;
    return D_NONE;
}

int mapopt2stype(int opt)
{
    switch (opt)
    {
    case D_DESERT:      return Desert_Pyramid;
    case D_JUNGLE:      return Jungle_Pyramid;
    case D_IGLOO:       return Igloo;
    case D_HUT:         return Swamp_Hut;
    case D_VILLAGE:     return Village;
    case D_MANSION:     return Mansion;
    case D_MONUMENT:    return Monument;
    case D_RUINS:       return Ocean_Ruin;
    case D_SHIPWRECK:   return Shipwreck;
    case D_TREASURE:    return Treasure;
    case D_MINESHAFT:   return Mineshaft;
    case D_WELL:        return Desert_Well;
    case D_GEODE:       return Geode;
    case D_OUTPOST:     return Outpost;
    case D_ANCIENTCITY: return Ancient_City;
    case D_TRAILS:      return Trail_Ruin;
    case D_PORTAL:      return Ruined_Portal;
    case D_PORTALN:     return Ruined_Portal_N;
    case D_FORTESS:     return Fortress;
    case D_BASTION:     return Bastion;
    case D_ENDCITY:     return End_City;
    case D_GATEWAY:     return End_Gateway;
    default:
        return -1;
    }
}

MapConfig::MapConfig(bool init)
{
    if (init)
        reset();
    else
        opts[0].scale = -1;
}

bool MapConfig::hasScale(int opt) const
{
    return opt >= D_DESERT && opt <= D_STRONGHOLD;
}

int MapConfig::getDim(int opt) const
{
    if (opt >= D_PORTALN && opt <= D_BASTION) return DIM_NETHER;
    if (opt >= D_ENDCITY && opt <= D_GATEWAY) return DIM_END;
    return DIM_OVERWORLD;
};

int MapConfig::getTileSize(int opt) const
{
    if (opt == D_MANSION)
        return 1280 * 16;
    if (opt == D_GEODE)
        return 128 * 16;
    return 512 * 16;
}

bool MapConfig::sameMapOpts(const MapConfig& a) const
{
    return memcmp(opts, a.opts, sizeof(opts)) == 0;
}

void MapConfig::reset()
{
    for (int i = 0; i < D_STRUCT_NUM; i++)
    {
        opts[i].scale = hasScale(i) ? 32 : 1;
        opts[i].enabled = true;
    }
    opts[D_GEODE].scale = 16;
    zoomEnabled = false;
}

void MapConfig::load(QSettings& settings)
{
    reset();
    for (int opt = 0; opt < D_STRUCT_NUM; opt++)
    {
        const char *name = mapopt2str(opt);
        double x = scale(opt);
        if (!enabled(opt))
            x = -x;
        x = settings.value(QString("structscale/") + name, x).toDouble();
        opts[opt].scale = fabs(x);
        opts[opt].enabled = x > 0;
    }
    zoomEnabled = settings.value("structscale/zoom", zoomEnabled).toBool();
}

void MapConfig::save(QSettings& settings)
{
    for (int opt = 0; opt < D_STRUCT_NUM; opt++)
    {
        const char *name = mapopt2str(opt);
        double x = scale(opt);
        if (x == 0) x = 1;
        if (!enabled(opt))
            x = -x;
        settings.setValue(QString("structscale/") + name, x);
    }
    settings.setValue(QString("structscale/zoom"), zoomEnabled);
}

void Config::reset()
{
    smoothMotion = true;
    showBBoxes = true;
    restoreSession = true;
    checkForUpdates = false;
    autosaveCycle = 10;
    uistyle = STYLE_SYSTEM;
    maxMatching = 65536;
    gridSpacing = 0;
    gridMultiplier = 0;
    mapCacheSize = 256;
    mapThreads = 0;
    lang = "en_US";
    biomeColorPath = "";
    separator = ";";
    quote = "";
    fontNorm = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    fontMono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    //fontNorm.setFamily(QString::fromUtf8("DejaVu Sans"));
    //fontMono.setFamily(QString::fromUtf8("DejaVu Sans Mono"));
    fontMono.setStyleHint(QFont::Monospace);
    fontNorm.setStyleHint(QFont::AnyStyle);
    fontMono.setPointSize(10);
    fontNorm.setPointSize(10);
    iconScale = 1.0;
}

void Config::load(QSettings& settings)
{
    smoothMotion = settings.value("config/smoothMotion", smoothMotion).toBool();
    showBBoxes = settings.value("config/showBBoxes", showBBoxes).toBool();
    restoreSession = settings.value("config/restoreSession", restoreSession).toBool();
    checkForUpdates = settings.value("config/checkForUpdates", checkForUpdates).toBool();
    autosaveCycle = settings.value("config/autosaveCycle", autosaveCycle).toInt();
    uistyle = settings.value("config/uistyle", uistyle).toInt();
    maxMatching = settings.value("config/maxMatching", maxMatching).toInt();
    gridSpacing = settings.value("config/gridSpacing", gridSpacing).toInt();
    gridMultiplier = settings.value("config/gridMultiplier", gridMultiplier).toInt();
    mapCacheSize = settings.value("config/mapCacheSize", mapCacheSize).toInt();
    mapThreads = settings.value("config/mapThreads", mapThreads).toInt();
    lang = settings.value("config/lang", lang).toString();
    biomeColorPath = settings.value("config/biomeColorPath", biomeColorPath).toString();
    separator = settings.value("config/separator", separator).toString();
    quote = settings.value("config/quote", quote).toString();
    fontNorm = settings.value("config/fontNorm", fontNorm).value<QFont>();
    fontMono = settings.value("config/fontMono", fontMono).value<QFont>();
    iconScale = settings.value("config/iconScale", iconScale).toDouble();
}

void Config::save(QSettings& settings)
{
    settings.setValue("config/smoothMotion", smoothMotion);
    settings.setValue("config/showBBoxes", showBBoxes);
    settings.setValue("config/restoreSession", restoreSession);
    settings.setValue("config/checkForUpdates", checkForUpdates);
    settings.setValue("config/autosaveCycle", autosaveCycle);
    settings.setValue("config/uistyle", uistyle);
    settings.setValue("config/maxMatching", maxMatching);
    settings.setValue("config/gridSpacing", gridSpacing);
    settings.setValue("config/gridMultiplier", gridMultiplier);
    settings.setValue("config/mapCacheSize", mapCacheSize);
    settings.setValue("config/mapThreads", mapThreads);
    settings.setValue("config/lang", lang);
    settings.setValue("config/biomeColorPath", biomeColorPath);
    settings.setValue("config/separator", separator);
    settings.setValue("config/quote", quote);
    settings.setValue("config/fontNorm", fontNorm);
    settings.setValue("config/fontMono", fontMono);
    settings.setValue("config/iconScale", iconScale);
}

void Gen48Config::reset()
{
    mode = GEN48_AUTO;
    slist48path = "";
    salt = 0;
    listsalt = 0;
    qual = IDEAL;
    qmarea = 13028;
    manualarea = false;
    x1 = z1 = x2 = z2 = 0;
}

bool Gen48Config::read(const QString& line)
{
    QByteArray ba = line.toLocal8Bit();
    const char *p = ba.data();
    if (sscanf(p, "#Mode48:   %d", &mode) == 1)            return true;
    if (line.startsWith("#List48:   "))                    { slist48path = line.mid(11).trimmed(); return true; }
    if (sscanf(p, "#Salt:     %" PRIu64, &salt) == 1)      return true;
    if (sscanf(p, "#LSalt:    %" PRIu64, &listsalt) == 1)  return true;
    if (sscanf(p, "#HutQual:  %d", &qual) == 1)            return true;
    if (sscanf(p, "#MonArea:  %d", &qmarea) == 1)          return true;
    if (sscanf(p, "#Gen48X1:  %d", &x1) == 1)              { manualarea = true; return true; }
    if (sscanf(p, "#Gen48Z1:  %d", &z1) == 1)              { manualarea = true; return true; }
    if (sscanf(p, "#Gen48X2:  %d", &x2) == 1)              { manualarea = true; return true; }
    if (sscanf(p, "#Gen48Z2:  %d", &z2) == 1)              { manualarea = true; return true; }
    return false;
}

void Gen48Config::write(QTextStream& stream)
{
    stream << "#Mode48:   " << mode << "\n";
    if (!slist48path.isEmpty())
        stream << "#List48:   " << slist48path.replace("\n", "") << "\n";
    stream << "#HutQual:  " << qual << "\n";
    stream << "#MonArea:  " << qmarea << "\n";
    if (salt != 0)
        stream << "#Salt:     " << salt << "\n";
    if (listsalt != 0)
        stream << "#LSalt:    " << listsalt << "\n";
    if (manualarea)
    {
        stream << "#Gen48X1:  " << x1 << "\n";
        stream << "#Gen48Z1:  " << z1 << "\n";
        stream << "#Gen48X2:  " << x2 << "\n";
        stream << "#Gen48Z2:  " << z2 << "\n";
    }
}

uint64_t Gen48Config::estimateSeedCnt(uint64_t slist48len)
{
    uint64_t cnt = 0;
    if (mode == GEN48_QH)
    {
        switch (qual)
        { // simply hardcoded number of seeds in each category
        case IDEAL_SALTED: // falltrough
        case IDEAL:     cnt =  74474; break;
        case CLASSIC:   cnt = 107959; break;
        case NORMAL:    cnt = 293716; break;
        case BARELY:    cnt = 755745; break;
        default: cnt = 0;
        }
    }
    else if (mode == GEN48_QM)
    {
        cnt = 0;
        for (int i = 0, n = sizeof(g_qm_90) / sizeof(int64_t); i < n; i++)
            if (qmonumentQual(g_qm_90[i]) >= qmarea)
                cnt++;
    }
    else if (mode == GEN48_LIST)
    {
        cnt = slist48len;
    }
    else
    {
        cnt = MASK48 + 1;
    }

    if (mode != GEN48_NONE)
    {
        uint64_t w = x2 - x1 + 1;
        uint64_t h = z2 - z1 + 1;
        uint64_t n = w*h * cnt; // div sizeof(uint64_t)
        if (cnt > 0 && n < PRECOMPUTE48_BUFSIZ * sizeof(uint64_t) && n / cnt == w*h)
            cnt = n;
        else
            cnt = MASK48 + 1;
    }
    return cnt;
}

void SearchConfig::reset()
{
    searchtype = SEARCH_INC;
    slist64path = "";
    threads = QThread::idealThreadCount();
    startseed = 0;
    stoponres = true;
    smin = 0;
    smax = ~(uint64_t)0;
}

bool SearchConfig::read(const QString& line)
{
    QByteArray ba = line.toLocal8Bit();
    const char *p = ba.data();
    int tmp;
    if (sscanf(p, "#Search:   %d", &searchtype) == 1)       return true;
    if (line.startsWith("#List64:   "))                     { slist64path = line.mid(11).trimmed(); return true; }
    if (sscanf(p, "#Threads:  %d", &threads) == 1)          return true;
    if (sscanf(p, "#Progress: %" PRId64, &startseed) == 1)  return true;
    if (sscanf(p, "#ResStop:  %d", &tmp) == 1)              { stoponres = tmp; return true; }
    if (sscanf(p, "#SMin:     %" PRIu64, &smin) == 1)       return true;
    if (sscanf(p, "#SMax:     %" PRIu64, &smax) == 1)       return true;
    return false;
}

void SearchConfig::write(QTextStream& stream)
{
    stream << "#Search:   " << searchtype << "\n";
    if (!slist64path.isEmpty())
        stream << "#List64:   " << slist64path.replace("\n", "") << "\n";
    stream << "#Progress: " << startseed << "\n";
    stream << "#Threads:  " << threads << "\n";
    stream << "#ResStop:  " << (int)stoponres << "\n";
    if (smin != 0)
        stream << "#SMin:     " << smin << "\n";
    if (smax != ~(uint64_t)0)
        stream << "#SMax:     " << smax << "\n";
}

