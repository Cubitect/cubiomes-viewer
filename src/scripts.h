#ifndef SCRIPTS_H
#define SCRIPTS_H

#include <QString>
#include <QFileInfo>
#include <QMap>
#include <QPlainTextEdit>

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
    QString out;
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
        Condition         * cond
);

class ScriptEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    ScriptEditor(QWidget *parent = nullptr);
    virtual ~ScriptEditor() {}

    void paintLineNumbers(QPaintEvent *event);
    int lineNumberAreaWidth();

    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
};

#endif // SCRIPTS_H
