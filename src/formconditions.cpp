#include "formconditions.h"
#include "ui_formconditions.h"

#include "filterdialog.h"
#include "mainwindow.h"

#include <QMessageBox>


QDataStream& operator<<(QDataStream& out, const Condition& v)
{
    out.writeRawData((const char*)&v, sizeof(Condition));
    return out;
}

QDataStream& operator>>(QDataStream& in, Condition& v)
{
    in.readRawData((char*)&v, sizeof(Condition));
    return in;
}

QString cond2str(Condition *cond)
{
    const FilterInfo& ft = g_filterinfo.list[cond->type];
    QString s = QString::asprintf("[%02d] %-28sx%-3d", cond->save, ft.name, cond->count);

    if (cond->relative)
        s += QString::asprintf("[%02d]+", cond->relative);
    else
        s += "     ";

    if (ft.coord)
        s += QString::asprintf("(%d,%d)", cond->x1*ft.step, cond->z1*ft.step);
    if (ft.area)
        s += QString::asprintf(",(%d,%d)", (cond->x2+1)*ft.step-1, (cond->z2+1)*ft.step-1);

    return s;
}


FormConditions::FormConditions(MainWindow *parent)
    : QWidget(parent)
    , parent(parent)
  , ui(new Ui::FormConditions)
{
    ui->setupUi(this);

    qRegisterMetaType< Condition >("Condition");
    qRegisterMetaTypeStreamOperators< Condition >("Condition");

    QFont mono = QFont("Monospace", 9);
    mono.setStyleHint(QFont::TypeWriter);
    ui->listConditionsFull->setFont(mono);
}

FormConditions::~FormConditions()
{
    delete ui;
}

QVector<Condition> FormConditions::getConditions() const
{
    QVector<Condition> conds;

    for (int i = 0, ie = ui->listConditionsFull->count(); i < ie; i++)
        conds.push_back(qvariant_cast<Condition>(ui->listConditionsFull->item(i)->data(Qt::UserRole)));

    return conds;
}


void FormConditions::updateSensitivity()
{
    int selectcnt = 0;
    selectcnt += ui->listConditionsFull->selectedItems().size();

    if (selectcnt == 0)
    {
        ui->buttonRemove->setEnabled(false);
        ui->buttonEdit->setEnabled(false);
    }
    else if (selectcnt == 1)
    {
        ui->buttonRemove->setEnabled(true);
        ui->buttonEdit->setEnabled(true);
    }
    else
    {
        ui->buttonRemove->setEnabled(true);
        ui->buttonEdit->setEnabled(false);
    }
}


int FormConditions::getIndex(int idx) const
{
    const QVector<Condition> condvec = getConditions();
    int cnt[100] = {};
    for (const Condition& c : condvec)
        if (c.save >= 0 && c.save < 100)
            cnt[c.save]++;
        else return 0;
    if (idx <= 0)
        idx = 1;
    if (cnt[idx] == 0)
        return idx;
    for (int i = 1; i < 100; i++)
        if (cnt[i] == 0)
            return i;
    return 0;
}



QListWidgetItem *FormConditions::lockItem(QListWidgetItem *item)
{
    QListWidgetItem *edit = item->clone();
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    item->setSelected(false);
    item->setBackground(QColor(128, 128, 128, 192));
    edit->setData(Qt::UserRole+1, (qulonglong)item);
    return edit;
}

bool list_contains(QListWidget *list, QListWidgetItem *item)
{
    if (!item)
        return false;
    int n = list->count();
    for (int i = 0; i < n; i++)
        if (list->item(i) == item)
            return true;
    return false;
}


// [ID] Condition Cnt Rel Area
void FormConditions::setItemCondition(QListWidget *list, QListWidgetItem *item, Condition *cond)
{
    QListWidgetItem *target = (QListWidgetItem*) item->data(Qt::UserRole+1).toULongLong();
    if (list_contains(list, target) && !(target->flags() & Qt::ItemIsSelectable))
    {
        item->setData(Qt::UserRole+1, (qulonglong)0);
        *target = *item;
        delete item;
        item = target;
    }
    else
    {
        list->addItem(item);
        cond->save = getIndex(cond->save);
    }

    QString s = cond2str(cond);

    const FilterInfo& ft = g_filterinfo.list[cond->type];
    if (ft.cat == CAT_QUAD)
        item->setBackground(QColor(255, 255, 0, 80));

    item->setText(s);
    item->setData(Qt::UserRole, QVariant::fromValue(*cond));
}


void FormConditions::editCondition(QListWidgetItem *item)
{
    if (!(item->flags() & Qt::ItemIsSelectable))
        return;
    WorldInfo wi;
    parent->getSeed(&wi);
    FilterDialog *dialog = new FilterDialog(this, &parent->config, wi.mc, item, (Condition*)item->data(Qt::UserRole).data());
    QObject::connect(dialog, SIGNAL(setCond(QListWidgetItem*,Condition)), this, SLOT(addItemCondition(QListWidgetItem*,Condition)), Qt::QueuedConnection);
    dialog->show();
}


static void remove_selected(QListWidget *list)
{
    QList<QListWidgetItem*> selected = list->selectedItems();
    for (QListWidgetItem *item : selected)
    {
        delete list->takeItem(list->row(item));
    }
}

void FormConditions::on_buttonRemoveAll_clicked()
{
    ui->listConditionsFull->clear();
    emit changed();
}

void FormConditions::on_buttonRemove_clicked()
{
    remove_selected(ui->listConditionsFull);
    emit changed();
}

void FormConditions::on_buttonEdit_clicked()
{
    QListWidget *list = 0;
    QListWidgetItem* edit = 0;
    QList<QListWidgetItem*> selected;

    list = ui->listConditionsFull;
    selected = list->selectedItems();
    if (!selected.empty())
        edit = lockItem(selected.first());

    if (edit)
        editCondition(edit);
}

void FormConditions::on_buttonAddFilter_clicked()
{
    WorldInfo wi;
    parent->getSeed(&wi);
    FilterDialog *dialog = new FilterDialog(this, &parent->config, wi.mc);
    QObject::connect(dialog, SIGNAL(setCond(QListWidgetItem*,Condition)), this, SLOT(addItemCondition(QListWidgetItem*,Condition)), Qt::QueuedConnection);
    dialog->show();
}

void FormConditions::on_listConditionsFull_itemDoubleClicked(QListWidgetItem *item)
{
    editCondition(lockItem(item));
}

void FormConditions::on_listConditionsFull_itemSelectionChanged()
{
    updateSensitivity();
}

void FormConditions::addItemCondition(QListWidgetItem *item, Condition cond)
{
    const FilterInfo& ft = g_filterinfo.list[cond.type];

    if (ft.cat != CAT_QUAD)
    {
        if (!item)
            item = new QListWidgetItem();
        setItemCondition(ui->listConditionsFull, item, &cond);
    }
    else if (item)
    {
        setItemCondition(ui->listConditionsFull, item, &cond);
    }
    else
    {
        item = new QListWidgetItem();
        setItemCondition(ui->listConditionsFull, item, &cond);

        if (cond.type >= F_QH_IDEAL && cond.type <= F_QH_BARELY)
        {
            Condition cq = cond;
            cq.type = F_HUT;
            //cq.x1 = -128; cq.z1 = -128;
            //cq.x2 = +128; cq.z2 = +128;
            // use 256 to avoid confusion when this restriction is removed
            cq.x1 = -256; cq.z1 = -256;
            cq.x2 = +256; cq.z2 = +256;
            cq.relative = cond.save;
            cq.save = cond.save+1;
            cq.count = 4;
            QListWidgetItem *item = new QListWidgetItem(ui->listConditionsFull, QListWidgetItem::UserType);
            setItemCondition(ui->listConditionsFull, item, &cq);
        }
        else if (cond.type == F_QM_90 || cond.type == F_QM_95)
        {
            Condition cq = cond;
            cq.type = F_MONUMENT;
            //cq.x1 = -160; cq.z1 = -160;
            //cq.x2 = +160; cq.z2 = +160;
            // use 256 to avoid confusion when this restriction is removed
            cq.x1 = -256; cq.z1 = -256;
            cq.x2 = +256; cq.z2 = +256;
            cq.relative = cond.save;
            cq.save = cond.save+1;
            cq.count = 4;
            QListWidgetItem *item = new QListWidgetItem(ui->listConditionsFull, QListWidgetItem::UserType);
            setItemCondition(ui->listConditionsFull, item, &cq);
        }
    }

    emit changed();
}

void FormConditions::on_listConditionsFull_indexesMoved(const QModelIndexList &)
{
    emit changed();
}
