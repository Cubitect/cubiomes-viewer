#include "formgen48.h"
#include "ui_formgen48.h"

#include "mainwindow.h"
#include "message.h"
#include "search.h"
#include "seedtables.h"
#include "util.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QSpinBox>
#include <QList>

#include <set>

class SetSpinBox : public QSpinBox
{
public:
    SetSpinBox(QWidget *parent)
    : QSpinBox(parent)
    {
        std::set<int> vset;
        for (const uint64_t *s = g_qm_90; *s; s++)
            vset.insert(qmonumentQual(*s));
        for (int v : vset)
            vlist.push_back(v);

        setRange(vlist.front(), vlist.last());
        setValue(vlist.front());
    }
    virtual ~SetSpinBox() {}

    virtual void stepBy(int steps) override
    {
        int val = value();
        int idx = 0;
        for (int i = 0, n = vlist.size(); i < n; i++)
        {
            if (vlist[i] > val)
                break;
            idx = i;
        }
        idx += steps;
        if (idx < 0)
            idx = 0;
        if (idx >= vlist.size())
            idx = vlist.size() - 1;
        setValue(vlist[idx]);
    }

    QList<int> vlist;
};

FormGen48::FormGen48(MainWindow *parent)
    : QWidget(parent)
    , parent(parent)
    , ui(new Ui::FormGen48)
{
    ui->setupUi(this);

    QIntValidator *intval = new QIntValidator(this);
    ui->lineSalt->setValidator(intval);
    ui->lineListSalt->setValidator(intval);
    ui->lineEditX1->setValidator(intval);
    ui->lineEditZ1->setValidator(intval);
    ui->lineEditX2->setValidator(intval);
    ui->lineEditZ2->setValidator(intval);
    connect(ui->lineEditX1, SIGNAL(editingFinished()), SLOT(onChange()));
    connect(ui->lineEditZ1, SIGNAL(editingFinished()), SLOT(onChange()));
    connect(ui->lineEditX2, SIGNAL(editingFinished()), SLOT(onChange()));
    connect(ui->lineEditZ2, SIGNAL(editingFinished()), SLOT(onChange()));

    spinMonumentArea = new SetSpinBox(this);
    ui->tabQuadM->layout()->addWidget(spinMonumentArea);

    connect(spinMonumentArea, SIGNAL(editingFinished()), SLOT(onChange()));
    connect(ui->lineSalt, SIGNAL(editingFinished()), SLOT(onChange()));
    connect(ui->lineListSalt, SIGNAL(editingFinished()), SLOT(onChange()));

    cond.type = 0;
    Gen48Config defaults;
    setConfig(defaults, true);
}

FormGen48::~FormGen48()
{
    delete ui;
    delete spinMonumentArea;
}

bool FormGen48::setList48(QString path, bool quiet)
{
    bool ok = false;

    if (!path.isEmpty())
    {
        QFileInfo finfo(path);
        slist48path = path;
        parent->prevdir = finfo.absolutePath();

        uint64_t *l = NULL;
        uint64_t len;
        QByteArray ba = path.toLocal8Bit();
        l = loadSavedSeeds(ba.data(), &len);
        if (l != NULL)
        {
            slist48.assign(l, l+len);
            free(l);
            ok = true;
        }
        else if (!quiet)
        {
            int button = warn(this, tr("Warning"),
                tr("Failed to load 48-bit seed list from file:\n\"%1\"").arg(path),
                tr("Reset list path?"), QMessageBox::Reset | QMessageBox::Ignore);
            if (button != QMessageBox::Ignore)
            {
                slist48path.clear();
                slist48.clear();
            }
        }
    }
    else
    {
        slist48path.clear();
        slist48.clear();
    }

    if (slist48path.isEmpty())
    {
        ui->lineList48->setText(tr("[none]"));
    }
    else
    {
        QString fname = QFileInfo(path).baseName();
        if (slist48.empty())
            ui->lineList48->setText(tr("[no seeds!] %1").arg(fname));
        else
            ui->lineList48->setText(tr("[%n seed(s)] %1", "", slist48.size()).arg(fname));
    }

    emit changed();
    return ok;
}

bool FormGen48::setList48(QTextStream& stream)
{
    slist48path.clear();
    slist48.clear();
    while (!stream.atEnd())
    {
        QByteArray line = stream.readLine().toLocal8Bit();
        uint64_t s = 0;
        if (sscanf(line.data(), "%" PRId64, (int64_t*)&s) == 1)
            slist48.push_back(s);
    }

    if (slist48.empty())
        ui->lineList48->setText(tr("[no seeds!]"));
    else
        ui->lineList48->setText(tr("[%n seed(s)]", "", slist48.size()));

    emit changed();
    return true;
}


void FormGen48::setConfig(const Gen48Config& gen48, bool quiet)
{
    ui->tabWidget->setCurrentIndex(gen48.mode);
    ui->comboLow20->setCurrentIndex(gen48.qual);
    spinMonumentArea->setValue(gen48.qmarea);
    ui->lineSalt->setText(QString::number(gen48.salt));
    ui->lineListSalt->setText(QString::number(gen48.listsalt));
    ui->lineEditX1->setText(QString::number(gen48.x1));
    ui->lineEditZ1->setText(QString::number(gen48.z1));
    ui->lineEditX2->setText(QString::number(gen48.x2));
    ui->lineEditZ2->setText(QString::number(gen48.z2));

#if WASM
    (void) quiet;
#else
    setList48(gen48.slist48path, quiet);
#endif

    if (gen48.manualarea)
        ui->radioManual->setChecked(true);
    else
        ui->radioAuto->setChecked(true);

    on_tabWidget_currentChanged(gen48.mode);
}

Gen48Config FormGen48::getConfig(bool resolveauto)
{
    Gen48Config s;

    s.mode = ui->tabWidget->currentIndex();
    s.qual = ui->comboLow20->currentIndex();
    s.qmarea = spinMonumentArea->value();
    s.salt = ui->lineSalt->text().toLongLong();
    s.listsalt = ui->lineListSalt->text().toLongLong();
    s.manualarea = ui->radioManual->isChecked();
    s.x1 = ui->lineEditX1->text().toInt();
    s.z1 = ui->lineEditZ1->text().toInt();
    s.x2 = ui->lineEditX2->text().toInt();
    s.z2 = ui->lineEditZ2->text().toInt();

    s.slist48path = slist48path;

    if (resolveauto)
    {
        if (s.mode == GEN48_AUTO)
        {
            bool isqh = cond.type >= F_QH_IDEAL && cond.type <= F_QH_BARELY;
            bool isqm = cond.type >= F_QM_95 && cond.type <= F_QM_90;
            if (isqh) s.mode = GEN48_QH;
            if (isqm) s.mode = GEN48_QM;
        }
    }

    return s;
}

uint64_t FormGen48::estimateSeedCnt()
{
    return getConfig(true).estimateSeedCnt(slist48.size());
}

void FormGen48::updateCount()
{
    uint64_t cnt = estimateSeedCnt();

    if (cnt >= MASK48+1)
    {
        ui->labelCount->setText(tr("all", "Checking all 64-bit seeds"));
    }
    else
    {
        uint64_t total = cnt << 16;
        ui->labelCount->setText(tr("%1 %2 65536 = %3").arg(cnt).arg(QChar(0xD7)).arg(total));
    }
}


void FormGen48::updateAutoConditions(const std::vector<Condition>& condlist)
{
    cond.type = 0;
    for (const Condition& c : condlist)
    {
        const FilterInfo& finfo = g_filterinfo.list[c.type];
        if (finfo.cat == CAT_QUAD)
        {
            cond = c;
            break;
        }
    }
    updateAutoUi();
}

void FormGen48::updateAutoUi()
{
    QString modestr = "";
    bool isqh = cond.type >= F_QH_IDEAL && cond.type <= F_QH_BARELY;
    bool isqm = cond.type >= F_QM_95 && cond.type <= F_QM_90;
    if (isqh)
        modestr = tr("[Quad-hut]");
    else if (isqm)
        modestr = tr("[Quad-monument]");
    else
        modestr = tr("[None]");

    ui->labelAuto->setText(modestr);

    if (cond.type != 0)
    {
        if (ui->tabWidget->currentIndex() == GEN48_AUTO)
        {
            ui->radioAuto->setChecked(true);

            if (isqh)
                ui->comboLow20->setCurrentIndex(cond.type - F_QH_IDEAL);
            else if (isqm)
                spinMonumentArea->setValue((int) ceil( 58*58*4 * (cond.type == F_QM_95 ? 0.95 : 0.90) ));
        }
        if (ui->radioAuto->isChecked())
        {
            if (ui->tabWidget->currentIndex() == GEN48_LIST)
            {
                ui->lineEditX1->setText("0");
                ui->lineEditZ1->setText("0");
                ui->lineEditX2->setText("0");
                ui->lineEditZ2->setText("0");
            }
            else
            {
                int x1, z1, x2, z2;
                if (cond.rmax > 0)
                {
                    int rmax = cond.rmax - 1;
                    x1 = -rmax >> 9;
                    z1 = -rmax >> 9;
                    x2 = +rmax >> 9;
                    z2 = +rmax >> 9;
                }
                else
                {
                    x1 = cond.x1 >> 9;
                    z1 = cond.z1 >> 9;
                    x2 = cond.x2 >> 9;
                    z2 = cond.z2 >> 9;
                }
                ui->lineEditX1->setText(QString::number(x1));
                ui->lineEditZ1->setText(QString::number(z1));
                ui->lineEditX2->setText(QString::number(x2));
                ui->lineEditZ2->setText(QString::number(z2));
            }
        }
    }
    emit changed();
}

void FormGen48::setAreaEnabled(bool enabled)
{
    ui->labelTranspose->setEnabled(enabled);
    ui->radioAuto->setEnabled(enabled);
    ui->radioManual->setEnabled(enabled);

    enabled &= ui->radioManual->isChecked();
    ui->labelX1->setEnabled(enabled);
    ui->labelZ1->setEnabled(enabled);
    ui->labelX2->setEnabled(enabled);
    ui->labelZ2->setEnabled(enabled);
    ui->lineEditX1->setEnabled(enabled);
    ui->lineEditZ1->setEnabled(enabled);
    ui->lineEditX2->setEnabled(enabled);
    ui->lineEditZ2->setEnabled(enabled);
}

void FormGen48::updateMode()
{
    int mode = ui->tabWidget->currentIndex();

    //if (mode == GEN48_AUTO)
    {
        updateAutoUi();
    }

    if (mode == GEN48_AUTO || mode == GEN48_NONE)
        setAreaEnabled(false);
    else
        setAreaEnabled(true);

    if (mode == GEN48_QH)
    {
        int qual = ui->comboLow20->currentIndex();
        ui->labelSalt->setEnabled(qual == IDEAL_SALTED);
        ui->lineSalt->setEnabled(qual == IDEAL_SALTED);
    }

    emit changed();
}

void FormGen48::on_tabWidget_currentChanged(int)
{
    updateMode();
}

void FormGen48::on_comboLow20_currentIndexChanged(int)
{
    updateMode();
}

void FormGen48::on_buttonBrowse_clicked()
{
    QString filter = tr("Text files (*.txt);;Any files (*)");
#if WASM
    auto fileOpenCompleted = [=](const QString &fnam, const QByteArray &content) {
        if (!fnam.isEmpty()) {
            QTextStream stream(content);
            setList48(stream);
        }
    };
    QFileDialog::getOpenFileContent(filter, fileOpenCompleted);
#else
    QString fnam = QFileDialog::getOpenFileName(this, tr("Load seed list"), parent->prevdir, filter);
    if (!fnam.isEmpty())
        setList48(fnam, false);
#endif
}

void FormGen48::on_radioAuto_toggled()
{
    setAreaEnabled(true);
    emit changed();
}

void FormGen48::onChange()
{
    emit changed();
}

