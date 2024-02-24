#ifndef SCRIPTS_H
#define SCRIPTS_H

#include <QString>
#include <QTime>
#include <QFileInfo>
#include <QMap>
#include <QPlainTextEdit>
#include <QMutex>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

#include "cubiomes/finders.h"

#include "lua/src/lua.hpp"

struct SearchThreadEnv;
struct Condition;

// store script output/errors per condition save index
struct LuaOutput
{
    uint64_t hash;
    uint64_t seed;
    const char *func;
    Pos at;
    QTime time;
    QString msg;
    QMutex mutex;
    LuaOutput() : mutex() { set(0,0,0,Pos{0,0},""); }
    void set(uint64_t h, uint64_t s, const char *f, Pos a, QString m)
    {
        mutex.lock();
        hash = h;
        seed = s;
        func = f;
        at = a;
        time = QTime::currentTime();
        msg = m;
        mutex.unlock();
    }
};
extern LuaOutput g_lua_output[100];

QString getLuaDir();
uint64_t getScriptHash(QFileInfo path);

void getScripts(QMap<uint64_t, QString>& scripts);

lua_State *loadScript(QString path, QString *err = 0);

// tries to run a lua check function
int runCheckScript(
        lua_State         * L,
        Pos                 at,
        SearchThreadEnv   * env,
        int                 pass,
        Pos               * path,
        const Condition   * cond
);

struct Rule
{
    QRegularExpression pattern;
    QTextCharFormat format;
    bool overlay;
    Rule() : pattern(),format(),overlay() {}
    Rule(const QString& p, const QTextCharFormat& f, bool o = false)
        : pattern(p), format(f), overlay(o) {}
};
struct BlockRule
{
    QRegularExpression start, end;
    QTextCharFormat format;
    BlockRule() : start(), end(), format() {}
    BlockRule(const QString& s, const QString& e, const QTextCharFormat& f)
        : start(s), end(e), format(f) {}
};

class LuaHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    LuaHighlighter(QTextDocument *parent = nullptr);

    BlockRule *nextBlockRule(const QString& text, int *pos, int *next);
    virtual void highlightBlock(const QString& text) override;
    void markFormated(QString *text, int start, int count, const QTextCharFormat& format);

    QVector<Rule> rules;
    QVector<BlockRule> blockrules;
    QTextCharFormat spaceformat;
};

class ScriptEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    ScriptEditor(QWidget *parent = nullptr);
    virtual ~ScriptEditor() {}

    void paintLineNumbers(QPaintEvent *event);
    int lineNumberAreaWidth();

    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
    LuaHighlighter *highlighter;
};

#endif // SCRIPTS_H
