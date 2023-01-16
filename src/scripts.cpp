#include "scripts.h"

#include "search.h"
#include "cutil.h"

#include <QApplication>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDirIterator>
#include <QPainter>
#include <QTextBlock>
#include <QTextDocumentFragment>


LuaOutput g_lua_output[100];


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
        g_lua_output[cond->save].set(cond->hash, env->seed, func, at, err);
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
            if (!(abs(x) <= 30e6 && abs(z) <= 30e6)) // negate condition to capture nan and inf
            {
                QString err = QString::asprintf("Output is invalid or out of range: {%g, %g}", x, z);
                g_lua_output[cond->save].set(cond->hash, env->seed, func, at, err);
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


LuaHighlighter::LuaHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    const QString keywords[] = {
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "goto", "if", "in", "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while",
    };
    QTextCharFormat format;
    //keywordFormat.setFontWeight(QFont::Bold);
    format.setForeground(QColor(255, 20, 20));
    for (const QString& k : keywords)
        rules.append(Rule("\\b" + k + "\\b", format));

    format.setFontWeight(QFont::Bold);
    format.setForeground(QColor(0, 168, 255));
    rules.append(Rule("\\b" "check" "\\b", format));
    rules.append(Rule("\\b" "check48" "\\b", format));
    rules.append(Rule("\\b" "getBiomeAt" "\\b", format));

    format.setFontWeight(QFont::Normal);
    format.setForeground(QColor(0, 160, 0));
    format.setFontItalic(true);
    rules.append(Rule("--(?!\\[\\[)[^\n]*", format));
    blockrules.append(BlockRule("--\\[\\[", "\\]\\]", format));

    format.setFontWeight(QFont::Normal);
    format.setForeground(QColor(48, 96, 255));
    format.setFontItalic(false);
    blockrules.append(BlockRule("\\[\\[", "\\]\\]", format));
    blockrules.append(BlockRule("\"", "(\"|$)", format));
    blockrules.append(BlockRule("'", "('|$)", format));

    spaceformat.setForeground(QColor(128, 128, 128, 40));
    rules.append(Rule("\\s+", spaceformat, true));
}

BlockRule *LuaHighlighter::nextBlockRule(const QString& text, int *pos, int *next)
{
    BlockRule *rule = nullptr;
    int min = INT_MAX;
    for (int i = 0, n = blockrules.size(); i < n; i++)
    {
        int s = blockrules[i].start.indexIn(text, *pos);
        if (s >= 0 && s < min)
        {
            rule = &blockrules[i];
            min = s;
        }
    }
    if (rule)
    {
        *pos = min;
        *next = min + rule->start.matchedLength();
    }
    return rule;
}

void LuaHighlighter::highlightBlock(const QString& text)
{
    QString line = text;
    BlockRule *rule = nullptr;
    int match = 0;
    int start = 0;
    int state = previousBlockState();
    if (state < 0)
        rule = nextBlockRule(line, &start, &match);
    else
        rule = &blockrules[state];

    while (rule)
    {
        int end = rule->end.indexIn(text, match);
        if (end == -1)
        {
            markFormated(&line, start, line.length() - start, rule->format);
            break;
        }
        else
        {
            end += rule->end.matchedLength();
            markFormated(&line, start, end - start, rule->format);
            start = end;
            rule = nextBlockRule(line, &start, &match);
        }
    }
    if (rule)
        setCurrentBlockState(rule - &blockrules[0]);
    else
        setCurrentBlockState(-1);

    for (const Rule &rule : qAsConst(rules))
    {
        const QString *l = rule.overlay ? &text : &line;
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(*l);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(*l, index + length);
        }
    }
}

void LuaHighlighter::markFormated(QString *text, int start, int count, const QTextCharFormat& format)
{
    setFormat(start, count, format);
    if (text)
    {
        for (int i = 0; i < count; i++)
            (*text)[start+i] = QChar(' ');
    }
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
    highlighter = new LuaHighlighter(document());

    connect(this, &ScriptEditor::blockCountChanged, this, &ScriptEditor::updateLineNumberAreaWidth);
    connect(this, &ScriptEditor::updateRequest, this, &ScriptEditor::updateLineNumberArea);
    connect(this, &ScriptEditor::cursorPositionChanged, this, &ScriptEditor::highlightCurrentLine);

    QTextOption opt = document()->defaultTextOption();
    opt.setFlags(opt.flags() | QTextOption::ShowTabsAndSpaces);
    document()->setDefaultTextOption(opt);
    setLineWrapMode(LineWrapMode::NoWrap);

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

    QColor lineColor = QColor::fromRgb(255, 255, 0, 28);
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

void ScriptEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return)
    {
        QTextCursor cursor = textCursor();
        int pos = cursor.position();
        int start = pos - cursor.positionInBlock();
        int end = start;
        while (end < pos)
        {
            if (!document()->characterAt(end).isSpace())
                break;
            end++;
        }
        if (start < end)
        {
            const QString text = document()->toPlainText();
            const QChar linebreak = QChar(0x2029);
            QString ws = linebreak + QStringRef(&text, start, end-start).toString();
            cursor.beginEditBlock();
            cursor.insertText(ws, highlighter->spaceformat);
            cursor.endEditBlock();
            return; // cancel normal enter
        }
    }
    else if (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab)
    {
        bool back = event->key() == Qt::Key_Backtab;
        QTextCursor cursor = textCursor();

        if (back || cursor.hasSelection())
        {
            int s = cursor.selectionStart();
            int e = cursor.selectionEnd();
            cursor.setPosition(e);
            e = cursor.block().blockNumber();
            cursor.setPosition(s);
            s = cursor.block().blockNumber();
            cursor.beginEditBlock();
            for (int i = s; i <= e; i++)
            {
                cursor.movePosition(QTextCursor::StartOfBlock);
                if (back == false)
                    cursor.insertText("\t", highlighter->spaceformat);
                else if (document()->characterAt(cursor.position()) == '\t')
                    cursor.deleteChar();
                cursor.movePosition(QTextCursor::NextBlock);
            }
            cursor.endEditBlock();
            return;
        }
    }
    QPlainTextEdit::keyPressEvent(event);
}


