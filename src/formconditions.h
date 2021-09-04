#ifndef FORMCONDITIONS_H
#define FORMCONDITIONS_H

#include <QWidget>
#include <QListWidgetItem>

#include "searchthread.h"

namespace Ui {
class FormConditions;
}

class MainWindow;

Q_DECLARE_METATYPE(Condition)

QString cond2str(Condition *cond);

class FormConditions : public QWidget
{
    Q_OBJECT

public:
    explicit FormConditions(MainWindow *parent);
    ~FormConditions();

    QVector<Condition> getConditions() const;
    void updateSensitivity();
    int getIndex(int idx) const;

    QListWidgetItem *lockItem(QListWidgetItem *item);
    void setItemCondition(QListWidget *list, QListWidgetItem *item, Condition *cond);
    void editCondition(QListWidgetItem *item);

signals:
    void changed();

public slots:
    void on_buttonRemoveAll_clicked();
    void on_buttonRemove_clicked();
    void on_buttonEdit_clicked();
    void on_buttonAddFilter_clicked();

    void on_listConditionsFull_itemDoubleClicked(QListWidgetItem *item);
    void on_listConditionsFull_itemSelectionChanged();

    void addItemCondition(QListWidgetItem *item, Condition cond);

private slots:
    void on_listConditionsFull_indexesMoved(const QModelIndexList &indexes);

private:
    MainWindow *parent;
    Ui::FormConditions *ui;
};

#endif // FORMCONDITIONS_H
