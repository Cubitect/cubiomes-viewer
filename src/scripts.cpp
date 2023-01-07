#include "scripts.h"

#include "search.h"
#include "cutil.h"

#include <QApplication>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDirIterator>
#include <QPainter>
#include <QTextBlock>


LuaOutput g_lua_output[100] = {};


static inline uint64_t murmur64(const void *key, int len, uint64_t h = 0)
{
    const uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;

    const uint64_t *p8 = (const uint64_t*) key;
    const uint64_t *e8 = p8 + (len >> 3);

    h ^= len * m;

    while (p8 != e8)
    {
        uint64_t k;
        memcpy(&k, p8++, sizeof(k));
        k *= m; k ^= k >> r; k *= m;
        h ^= k; h *= m;
    }

    const uint8_t *p1 = (const uint8_t*) p8;
    switch (len & 7)
    {
    case 7: h ^= (uint64_t)p1[6] << 48; // fallthrough
    case 6: h ^= (uint64_t)p1[5] << 40; // fallthrough
    case 5: h ^= (uint64_t)p1[4] << 32; // fallthrough
    case 4: h ^= (uint64_t)p1[3] << 24; // fallthrough
    case 3: h ^= (uint64_t)p1[2] << 16; // fallthrough
    case 2: h ^= (uint64_t)p1[1] <<  8; // fallthrough
    case 1: h ^= (uint64_t)p1[0];
        h *= m;
    };

    h ^= h >> r; h *= m; h ^= h >> r;
    return h;
}

#include <QDebug>

QString getLuaDir()
{
    QString luadir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/lua";
    QDir dir(luadir);
    if (!dir.exists())
        dir.mkpath(".");
    return luadir;
}

uint64_t getScriptHash(QFileInfo finfo)
{
    QString fnam = finfo.baseName();
    return murmur64(fnam.data(), fnam.size() * sizeof(QChar));
}

void getScripts(QMap<uint64_t, QString>& scripts)
{
    QString dir = getLuaDir();
    QDirIterator it(dir, QDirIterator::NoIteratorFlags);
    while (it.hasNext())
    {
        QFileInfo f = it.next();
        if (f.suffix() != "lua")
            continue;
        uint64_t hash = getScriptHash(f);
        if (scripts.contains(hash))
            qDebug() << "hash collision on: " << f.absoluteFilePath() << " and " << scripts.value(hash);
        scripts.insert(hash, f.absoluteFilePath());
    }
}

static int l_getBiomeAt(lua_State *L)
{
    lua_getglobal(L, "_cb_env");
    SearchThreadEnv *env = (SearchThreadEnv*) lua_touserdata(L, -1);
    lua_pop(L, 1);

    env->init4Dim(0);

    int x, y = 320, z;
    z = (int) lua_tonumber(L, -1);
    lua_pop(L, 1);
    x = (int) lua_tonumber(L, -1);
    lua_pop(L, 1);
    if (lua_isnumber(L, -1))
    {
        y = x;
        x = (int) lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_Integer id = getBiomeAt(&env->g, 4, x, y, z);
    lua_pushinteger(L, id);
    return 1;
}

lua_State *loadScript(QString path, QString *err)
{
    lua_State *L = luaL_newstate();
    bool ok = false;
    do
    {
        if (luaL_loadfile(L, path.toLocal8Bit().data()) != LUA_OK)
        {
            if (err) *err = lua_tostring(L, -1);
            break;
        }
        luaL_openlibs(L);
        if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK)
        {
            if (err) *err = lua_tostring(L, -1);
            break;
        }
        if (lua_getglobal(L, "check") != LUA_TFUNCTION)
        {
            if (err) *err = QApplication::tr("function check() was not defined");
            break;
        }
        lua_pop(L, 1);
        for (int id = 0; id < 256; id++)
        {
            const char *bname = biome2str(MC_NEWEST, id);
            if (bname)
            {
                lua_pushinteger(L, id);
                lua_setglobal(L, bname);
                const char *bname_old = biome2str(MC_1_13, id);
                if (bname != bname_old)
                {
                    lua_pushinteger(L, id);
                    lua_setglobal(L, bname);
                }
            }
        }
        lua_pushcfunction(L, l_getBiomeAt);
        lua_setglobal(L, "getBiomeAt");
        ok = true;
    }
    while (0);

    if (!ok)
    {
        lua_close(L);
        return nullptr;
    }
    return L;
}

struct node_t
{
    int x, z, id, parent;
};
static void gather_nodes(std::vector<node_t>& nodes, const ConditionTree *tree, const Pos *path, int id)
{
    const std::vector<char>& branches = tree->references[id];
    for (int b : branches)
    {
        node_t n = { path[b].x, path[b].z, b, id };
        nodes.push_back(n);
        gather_nodes(nodes, tree, path, b);
    }
}

#include <QDebug>

static QMutex g_mutex;

int runCheckScript(
    lua_State         * L,
    Pos                 at,
    SearchThreadEnv   * env,
    int                 pass,
    Pos               * path,
    Condition         * cond
)
{
    int top = lua_gettop(L);
    const char *func;
    if (pass == PASS_FAST_48)
        func = "check48";
    else
        func = "check";

    if (lua_getglobal(L, func) != LUA_TFUNCTION)
    {
        lua_settop(L, top);
        if (pass == PASS_FAST_48)
            return COND_MAYBE_POS_INVAL; // 48-bit check is optional
        return COND_FAILED;
    }

    std::vector<node_t> nodes;
    gather_nodes(nodes, env->condtree, path, cond->save);

    lua_pushlightuserdata(L, env);
    lua_setglobal(L, "_cb_env");

    lua_pushinteger(L, (lua_Integer) env->seed);

    // at
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, (lua_Integer) at.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, (lua_Integer) at.z);
    lua_setfield(L, -2, "z");

    // create an array for the subtree positions
    lua_createtable(L, nodes.size(), 0);

    for (int i = 0, n = nodes.size(); i < n; i++)
    {
        const node_t& node = nodes[i];
        lua_createtable(L, 0, 2);
        lua_pushinteger(L, node.x);
        lua_setfield(L, -2, "x");
        lua_pushinteger(L, node.z);
        lua_setfield(L, -2, "z");
        lua_pushinteger(L, node.id);
        lua_setfield(L, -2, "id");
        lua_pushinteger(L, node.parent);
        lua_setfield(L, -2, "parent");

        lua_seti(L, -2, i+1);
    }

    // call: pos = check(seed, area{x1,z1,x2,z2}, branches[b..]{x,z})
    if (lua_pcallk(L, 3, LUA_MULTRET, 0, 0, NULL) != 0)
    {
        QString err = lua_tostring(L, -1);
        qDebug() << err;
        LuaOutput lout = {
            cond->hash, env->seed, func, at,
            err,
        };
        g_mutex.lock();
        g_lua_output[cond->save] = lout;
        g_mutex.unlock();
        lua_settop(L, top);
        return COND_FAILED;
    }
    if (lua_isnumber(L, -1))
    {
        float z = lua_tonumber(L, -1); // accept floats
        lua_pop(L, 1);
        if (lua_isnumber(L, -1))
        {
            float x = lua_tonumber(L, -1);
            lua_pop(L, 1);
            lua_settop(L, top);
            if (!(abs(x) <= 30e6 && abs(z) <= 30e6)) // negate to capture nan and inf
            {
                LuaOutput lout = {
                    cond->hash, env->seed, func, at,
                    QString::asprintf("Output is invalid or out of range: {%g, %g}", x, z)
                };
                g_mutex.lock();
                g_lua_output[cond->save] = lout;
                g_mutex.unlock();
                return COND_FAILED;
            }
            path[cond->save].x = (int) x;
            path[cond->save].z = (int) z;
            if (pass == PASS_FAST_48)
                return COND_MAYBE_POS_VALID;
            return COND_OK;
        }
    }
    lua_settop(L, top);
    return COND_FAILED;
}


class ScriptEditor;
class LineNumberArea : public QWidget
{
public:
    LineNumberArea(ScriptEditor *editor) : QWidget(editor),editor(editor) {}

    QSize sizeHint() const override
    {
        return QSize(editor->lineNumberAreaWidth(), 0);
    }
    void paintEvent(QPaintEvent *event) override
    {
        editor->paintLineNumbers(event);
    }

    ScriptEditor *editor;
};

ScriptEditor::ScriptEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &ScriptEditor::blockCountChanged, this, &ScriptEditor::updateLineNumberAreaWidth);
    connect(this, &ScriptEditor::updateRequest, this, &ScriptEditor::updateLineNumberArea);
    connect(this, &ScriptEditor::cursorPositionChanged, this, &ScriptEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int ScriptEditor::lineNumberAreaWidth()
{
    int d = blockCount();
    d = 1 + (int) log10(d ? d : 1);
    return 3 + d * fontMetrics().horizontalAdvance('_');
}

void ScriptEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void ScriptEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void ScriptEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void ScriptEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;

    QColor lineColor = QColor::fromRgb(255, 255, 0, 64);
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);

    setExtraSelections(extraSelections);
}

void ScriptEditor::paintLineNumbers(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(128, 128, 128, 96));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}


