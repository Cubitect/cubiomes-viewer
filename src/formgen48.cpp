#include "formgen48.h"
#include "ui_formgen48.h"

#include "mainwindow.h"
#include "search.h"
#include "seedtables.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>


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

    connect(ui->spinMonumentArea, SIGNAL(editingFinished()), SLOT(onChange()));
    connect(ui->lineSalt, SIGNAL(editingFinished()), SLOT(onChange()));
    connect(ui->lineListSalt, SIGNAL(editingFinished()), SLOT(onChange()));

    QFont mono = QFont("Monospace", 9);
    mono.setStyleHint(QFont::TypeWriter);
    ui->lineList48->setFont(mono);

    cond.type = 0;
    Gen48Settings defaults;
    setSettings(defaults, true);
}

FormGen48::~FormGen48()
{
    delete ui;
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
        QByteArray ba = path.toLatin1();
        l = loadSavedSeeds(ba.data(), &len);
        if (l && len > 0)
        {
            slist48.assign(l, l+len);
            free(l);
            ui->lineList48->setText("[" + QString::number(len) + " seeds] " + finfo.baseName());
            ok = true;
        }
        else
        {
            if (!quiet)
                QMessageBox::warning(this, "Warning", "Failed to load seed list from file", QMessageBox::Ok);
            ui->lineList48->setText("[no seeds!] " + finfo.baseName());
        }
    }
    else
    {
        slist48path.clear();
        ui->lineList48->setText("[none]");
        slist48.clear();
    }
    emit changed();
    return ok;
}

void FormGen48::setSettings(const Gen48Settings& gen48, bool quiet)
{
    ui->tabWidget->setCurrentIndex(gen48.mode);
    ui->comboLow20->setCurrentIndex(gen48.qual);
    ui->spinMonumentArea->setValue(gen48.qmarea);
    ui->lineSalt->setText(QString::number(gen48.salt));
    ui->lineListSalt->setText(QString::number(gen48.listsalt));
    ui->lineEditX1->setText(QString::number(gen48.x1));
    ui->lineEditZ1->setText(QString::number(gen48.z1));
    ui->lineEditX2->setText(QString::number(gen48.x2));
    ui->lineEditZ2->setText(QString::number(gen48.z2));

    setList48(gen48.slist48path, quiet);

    if (gen48.manualarea)
        ui->radioManual->setChecked(true);
    else
        ui->radioAuto->setChecked(true);

    on_tabWidget_currentChanged(gen48.mode);
}

Gen48Settings FormGen48::getSettings(bool resolveauto)
{
    Gen48Settings s;

    s.mode = ui->tabWidget->currentIndex();
    s.qual = ui->comboLow20->currentIndex();
    s.qmarea = ui->spinMonumentArea->value();
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
    const Gen48Settings& gen48 = getSettings(true);
    uint64_t cnt = 0;
    if (gen48.mode == GEN48_QH)
    {
        switch (gen48.qual)
        { // simply hardcoded number of seeds in each category
        case IDEAL_SALTED:    // falltrough
        case IDEAL:     cnt =  74474; break;
        case CLASSIC:   cnt = 107959; break;
        case NORMAL:    cnt = 293716; break;
        case BARELY:    cnt = 755745; break;
        default: cnt = 0;
        }
    }
    else if (gen48.mode == GEN48_QM)
    {
        cnt = 0;
        for (int i = 0, n = sizeof(g_qm_90) / sizeof(int64_t); i < n; i++)
            if (qmonumentQual(g_qm_90[i]) >= gen48.qmarea)
                cnt++;
    }
    else if (gen48.mode == GEN48_LIST)
    {
        cnt = this->slist48.size();
    }
    else
    {
        cnt = MASK48 + 1;
    }

    if (gen48.mode != GEN48_NONE)
    {
        uint64_t w = gen48.x2 - gen48.x1 + 1;
        uint64_t h = gen48.z2 - gen48.z1 + 1;
        uint64_t n = w*h * cnt;
        if (cnt > 0 && n < PRECOMPUTE48_BUFSIZ*sizeof(uint64_t) && n / cnt == w*h)
            cnt = n;
        else
            cnt = MASK48+1;
    }

    return cnt;
}

void FormGen48::updateCount()
{
    uint64_t cnt = estimateSeedCnt();

    if (cnt >= MASK48+1)
    {
        ui->labelCount->setText("all");
    }
    else
    {
        uint64_t total = cnt << 16;
        ui->labelCount->setText(QString::asprintf("%" PRIu64 " * 65536 = %" PRIu64, cnt, total));
    }
}


void FormGen48::updateAutoConditions(const QVector<Condition>& condlist)
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
        modestr = "[Quad-hut]";
    else if (isqm)
        modestr = "[Quad-monument]";
    else
        modestr = "[None]";

    ui->labelAuto->setText(modestr);

    if (cond.type != 0)
    {
        if (ui->tabWidget->currentIndex() == GEN48_AUTO)
        {
            ui->radioAuto->setChecked(true);

            if (isqh)
                ui->comboLow20->setCurrentIndex(cond.type - F_QH_IDEAL);
            else if (isqm)
                ui->spinMonumentArea->setValue((int) ceil( 58*58*4 * (cond.type == F_QM_95 ? 0.95 : 0.90) ));
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
                ui->lineEditX1->setText(QString::number(cond.x1));
                ui->lineEditZ1->setText(QString::number(cond.z1));
                ui->lineEditX2->setText(QString::number(cond.x2));
                ui->lineEditZ2->setText(QString::number(cond.z2));
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
    QString fnam = QFileDialog::getOpenFileName(this, "Load seed list", parent->prevdir, "Text files (*.txt);;Any files (*)");
    if (!fnam.isEmpty())
        setList48(fnam, false);
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

