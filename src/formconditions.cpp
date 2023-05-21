#include "formconditions.h"
#include "ui_formconditions.h"

#include "conditiondialog.h"
#include "mainwindow.h"

#include <QMessageBox>
#include <QMenu>
#include <QClipboard>


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

FormConditions::FormConditions(QWidget *parent)
    : QWidget(parent)
    , parent(dynamic_cast<MainWindow*>(parent))
    , ui(new Ui::FormConditions)
{
    ui->setupUi(this);

    if (!this->parent)
    {
        QLayoutItem *item;
        while ((item = ui->layoutButtons->takeAt(0)))
        {
            delete item->widget();
        }
    }

    qRegisterMetaType< Condition >("Condition");
    qRegisterMetaTypeStreamOperators< Condition >("Condition");

    ui->listConditionsFull->setFont(*gp_font_mono);
}

FormConditions::~FormConditions()
{
    delete ui;
}

QVector<Condition> FormConditions::getConditions() const
{
    QVector<Condition> conds;

    for (int i = 0, ie = ui->listConditionsFull->count(); i < ie; i++)
    {
        Condition c = qvariant_cast<Condition>(ui->listConditionsFull->item(i)->data(Qt::UserRole));
        conds.push_back(c);
    }
    return conds;
}

void FormConditions::updateSensitivity()
{
    if (!parent)
        return;
    QList<QListWidgetItem*> selected = ui->listConditionsFull->selectedItems();

    if (selected.size() == 0)
    {
        ui->buttonRemove->setEnabled(false);
        ui->buttonEdit->setEnabled(false);
    }
    else if (selected.size() == 1)
    {
        ui->buttonRemove->setEnabled(true);
        ui->buttonEdit->setEnabled(true);
    }
    else
    {
        ui->buttonRemove->setEnabled(true);
        ui->buttonEdit->setEnabled(false);
    }

    int disabled = 0;
    for (int i = 0; i < selected.size(); i++)
    {
        Condition c = qvariant_cast<Condition>(selected[i]->data(Qt::UserRole));
        if (c.meta & Condition::DISABLED)
            disabled++;
    }
    if (disabled == 0)
        ui->buttonDisable->setText(tr("Disable"));
    else if (disabled == selected.size())
        ui->buttonDisable->setText(tr("Enable"));
    else
        ui->buttonDisable->setText(tr("Toggle"));
    ui->buttonDisable->setEnabled(!selected.empty());
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

    QString s = cond->summary();

    const FilterInfo& ft = g_filterinfo.list[cond->type];
    if (ft.cat == CAT_QUAD)
        item->setBackground(QColor(255, 255, 0, 80));

    item->setText(s);
    item->setData(Qt::UserRole, QVariant::fromValue(*cond));
}


void FormConditions::editCondition(QListWidgetItem *item)
{
    if (!(item->flags() & Qt::ItemIsSelectable) || parent == NULL)
        return;
    WorldInfo wi;
    parent->getSeed(&wi);
    ConditionDialog *dialog = new ConditionDialog(this, &parent->config, wi.mc, item, (Condition*)item->data(Qt::UserRole).data());
    QObject::connect(dialog, SIGNAL(setCond(QListWidgetItem*,Condition,int)), this, SLOT(addItemCondition(QListWidgetItem*,Condition,int)), Qt::QueuedConnection);
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

void FormConditions::on_buttonDisable_clicked()
{
    emit changed();
    QList<QListWidgetItem*> selected = ui->listConditionsFull->selectedItems();
    for (QListWidgetItem *item : selected)
    {
        Condition c = qvariant_cast<Condition>(item->data(Qt::UserRole));
        c.meta ^= Condition::DISABLED;
        item->setText(c.summary());
        item->setData(Qt::UserRole, QVariant::fromValue(c));
    }
    updateSensitivity();
}

void FormConditions::on_buttonEdit_clicked()
{
    QListWidgetItem* edit = 0;
    QList<QListWidgetItem*> selected = ui->listConditionsFull->selectedItems();

    if (!selected.empty())
        edit = lockItem(selected.first());

    if (edit)
        editCondition(edit);
}

void FormConditions::on_buttonAddFilter_clicked()
{
    if (!parent)
        return;
    WorldInfo wi;
    parent->getSeed(&wi);
    ConditionDialog *dialog = new ConditionDialog(this, &parent->config, wi.mc);
    QObject::connect(dialog, SIGNAL(setCond(QListWidgetItem*,Condition,int)), this, SLOT(addItemCondition(QListWidgetItem*,Condition)), Qt::QueuedConnection);
    dialog->show();
}

void FormConditions::on_listConditionsFull_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    // this is a contextual temporary menu so shortcuts are only indicated here,
    // but will not function - see keyReleaseEvent() for shortcut implementation

    int n = ui->listConditionsFull->selectedItems().size();

    if (parent)
    {
       QAction *actadd = menu.addAction(QIcon::fromTheme("list-add"),
            tr("Add new condition"), this,
            &FormConditions::conditionsAdd, QKeySequence::New);
        actadd->setEnabled(true);

        QAction *actedit = menu.addAction(QIcon(),
            tr("Edit condition"), this,
            &FormConditions::conditionsEdit, QKeySequence::Open);
        actedit->setEnabled(n == 1);

        QAction *actcut = menu.addAction(QIcon::fromTheme("edit-cut"),
            tr("Cut %n condition(s)", "", n), this,
            &FormConditions::conditionsCut, QKeySequence::Cut);
        actcut->setEnabled(n > 0);

        QAction *actcopy = menu.addAction(QIcon::fromTheme("edit-copy"),
            tr("Copy %n condition(s)", "", n), this,
            &FormConditions::conditionsCopy, QKeySequence::Copy);
        actcopy->setEnabled(n > 0);

        int pn = conditionsPaste(true);
        QAction *actpaste = menu.addAction(QIcon::fromTheme("edit-paste"),
            tr("Paste %n condition(s)", "", pn), this,
            &FormConditions::conditionsPaste, QKeySequence::Paste);
        actpaste->setEnabled(pn > 0);

        QAction *actdel = menu.addAction(QIcon::fromTheme("edit-delete"),
            tr("Remove %n condition(s)", "", n), this,
            &FormConditions::conditionsDelete, QKeySequence::Delete);
        actdel->setEnabled(n > 0);
    }
    else
    {
        QAction *actcopy = menu.addAction(QIcon::fromTheme("edit-copy"),
            tr("Copy %n condition(s)", "", n), this,
            &FormConditions::conditionsCopy, QKeySequence::Copy);
        actcopy->setEnabled(n > 0);
    }

    menu.exec(ui->listConditionsFull->mapToGlobal(pos));
}

void FormConditions::on_listConditionsFull_itemDoubleClicked(QListWidgetItem *item)
{
    if (!parent)
        return;
    editCondition(lockItem(item));
}

void FormConditions::on_listConditionsFull_itemSelectionChanged()
{
    updateSensitivity();
}

void FormConditions::conditionsAdd()
{
    if (!parent)
        return;
    on_buttonAddFilter_clicked();
}

void FormConditions::conditionsEdit()
{
    if (!parent)
        return;
    on_buttonEdit_clicked();
}

void FormConditions::conditionsCut()
{
    if (!parent)
        return;
    conditionsCopy();
    on_buttonRemove_clicked();
}

void FormConditions::conditionsCopy()
{
    QString text;
    QList<QListWidgetItem*> selected = ui->listConditionsFull->selectedItems();
    for (QListWidgetItem *item : selected)
    {
        Condition c = qvariant_cast<Condition>(item->data(Qt::UserRole));
        text += (text.isEmpty() ? "" : "\n") + c.toHex();
    }
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
}

int FormConditions::conditionsPaste(bool countonly)
{
    if (!parent)
        return 0;
    QClipboard *clipboard = QGuiApplication::clipboard();
    QStringList slist = clipboard->text().split('\n');
    Condition c;
    int cnt = 0;
    for (QString s : slist)
    {
        if (!c.readHex(s))
            continue;
        cnt++;
        if (countonly)
            continue;
        addItemCondition(NULL, c, 1);
    }
    return cnt;
}

void FormConditions::conditionsDelete()
{
    if (!parent)
        return;
    on_buttonRemove_clicked();
}

void FormConditions::addItemCondition(QListWidgetItem *item, Condition cond, int modified)
{
    const FilterInfo& ft = g_filterinfo.list[cond.type];

    if (ft.cat != CAT_QUAD)
    {
        if (!item) {
            item = new QListWidgetItem();
            modified = 1;
        }
        setItemCondition(ui->listConditionsFull, item, &cond);
    }
    else if (item)
    {
        setItemCondition(ui->listConditionsFull, item, &cond);
    }
    else
    {
        modified = 1;
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

    if (modified)
        emit changed();
}

void FormConditions::on_listConditionsFull_indexesMoved(const QModelIndexList &)
{
    emit changed();
}

void FormConditions::keyReleaseEvent(QKeyEvent *event)
{
    if (ui->listConditionsFull->hasFocus())
    {
        if (event->matches(QKeySequence::New))
            conditionsAdd();
        else if (event->matches(QKeySequence::Open))
            conditionsEdit();
        else if (event->matches(QKeySequence::Cut))
            conditionsCut();
        else if (event->matches(QKeySequence::Copy))
            conditionsCopy();
        else if (event->matches(QKeySequence::Paste))
            conditionsPaste();
        else if (event->matches(QKeySequence::Delete))
            conditionsDelete();
    }
    QWidget::keyReleaseEvent(event);
}


