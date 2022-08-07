#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "search.h"
#include "quad.h"

#include <QThread>
#include <QTreeWidgetItem>


class Analysis : public QThread
{
    Q_OBJECT
public:
    explicit Analysis(QObject *parent = nullptr);

    virtual void run() override;
    void requestAbort();

signals:
    void itemDone(QTreeWidgetItem *item);

public:
    QVector<Condition> conds;
    QVector<uint64_t> seeds;
    WorldInfo wi;
    int dim;
    int x1, z1, x2, z2;
    bool ck_struct;
    bool ck_biome;
    bool ck_conds;
    bool map_only;
    bool mapshow[STRUCT_NUM];

private:
    std::atomic_bool stop;
};

#endif // ANALYSIS_H
