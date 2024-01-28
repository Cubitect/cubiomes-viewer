#ifndef UTIL_H
#define UTIL_H

#include <QMutex>
#include <QString>
#include <QFontMetrics>
#include <QApplication>
#include <QPen>

#include <random>

#include "cubiomes/quadbase.h"
#include "cubiomes/util.h"

struct StartPiece
{
    int stype;
    int biome;
    int start;
    int giant;
    const char *name;
    int row, col; // UI layout
};

static const StartPiece g_start_pieces[] =
{   // the index is used to encode the start pieces in the condition
    // (and should therefore ideally remain constant across upgrades)
    {Village, plains, 0, -1, "plains_fountain_01", 0, 0},
    {Village, plains, 1, -1, "plains_meeting_point_1", 1, 0},
    {Village, plains, 2, -1, "plains_meeting_point_2", 2, 0},
    {Village, plains, 3, -1, "plains_meeting_point_3", 3, 0},
    {Village, desert, 1, -1, "desert_meeting_point_1", 4, 1},
    {Village, desert, 2, -1, "desert_meeting_point_2", 5, 1},
    {Village, desert, 3, -1, "desert_meeting_point_3", 6, 1},
    {Village, savanna, 1, -1, "savanna_meeting_point_1", 0, 1},
    {Village, savanna, 2, -1, "savanna_meeting_point_2", 1, 1},
    {Village, savanna, 3, -1, "savanna_meeting_point_3", 2, 1},
    {Village, savanna, 4, -1, "savanna_meeting_point_4", 3, 1},
    {Village, taiga, 1, -1, "taiga_meeting_point_1", 7, 0},
    {Village, taiga, 2, -1, "taiga_meeting_point_2", 8, 0},
    {Village, snowy_tundra, 1, -1, "snowy_meeting_point_1", 4, 0},
    {Village, snowy_tundra, 2, -1, "snowy_meeting_point_2", 5, 0},
    {Village, snowy_tundra, 3, -1, "snowy_meeting_point_3", 6, 0},
    {Bastion, -1, 0, -1, "units", 0, 0},
    {Bastion, -1, 1, -1, "hoglin_stable", 1, 0},
    {Bastion, -1, 2, -1, "treasure", 2, 0},
    {Bastion, -1, 3, -1, "bridge", 3, 0},
    {Ruined_Portal, -1, 1, 1, "giant_ruined_portal_1", 0, 1},
    {Ruined_Portal, -1, 2, 1, "giant_ruined_portal_2", 1, 1},
    {Ruined_Portal, -1, 3, 1, "giant_ruined_portal_3", 2, 1},
    {Ruined_Portal, -1, 1, 0, "ruined_portal_1", 0, 0},
    {Ruined_Portal, -1, 2, 0, "ruined_portal_2", 1, 0},
    {Ruined_Portal, -1, 3, 0, "ruined_portal_3", 2, 0},
    {Ruined_Portal, -1, 4, 0, "ruined_portal_4", 3, 0},
    {Ruined_Portal, -1, 5, 0, "ruined_portal_5", 4, 0},
    {Ruined_Portal, -1, 6, 0, "ruined_portal_6", 5, 0},
    {Ruined_Portal, -1, 7, 0, "ruined_portal_7", 6, 0},
    {Ruined_Portal, -1, 8, 0, "ruined_portal_8", 7, 0},
    {Ruined_Portal, -1, 9, 0, "ruined_portal_9", 8, 0},
    {Ruined_Portal, -1, 10, 0, "ruined_portal_10", 9, 0},
    {-1,0,0,0,0,0,0}
};

QString getStartPieceName(int stype, const StructureVariant *sv);

QString getBiomeDisplay(int mc, int id);

int txtWidth(const QFontMetrics& fm, const QString& s);
int txtWidth(const QFontMetrics& fm);
int txtWidth(const QFont& f, const QString& s);
int txtWidth(const QFont& f);

struct RandGen
{
    RandGen();
    std::mt19937_64 mt;
};

// get a random 64-bit integer
uint64_t getRnd64();


enum { S_TEXT, S_NUMERIC, S_RANDOM };
int str2seed(const QString &str, uint64_t *out);

struct IdCmp
{
    enum { SORT_ID, SORT_LEX, SORT_DIM };

    IdCmp(int mode, int mc, int dim) : mode(mode),mc(mc),dim(dim) {}

    bool operator() (int id1, int id2);
    bool isPrimary(int id);

    int mode;
    int mc;
    int dim;
};

QPixmap getPix(QString rc, int width = 0);

QIcon getColorIcon(const QColor& col, const QPen& pen = QPen(Qt::black, 1));

QIcon getBiomeIcon(int id, bool warn);


#endif // UTIL_H
