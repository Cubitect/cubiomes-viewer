#include "quadlistdialog.h"
#include "ui_quadlistdialog.h"

#include "mainwindow.h"
#include "mapview.h"
#include "cutil.h"
#include "seedtables.h"

#include <QMessageBox>
#include <QDebug>
#include <QMenu>


QuadListDialog::QuadListDialog(MainWindow *mainwindow)
    : QDialog(mainwindow)
    , ui(new Ui::QuadListDialog)
    , mainwindow(mainwindow)
{
    ui->setupUi(this);

    QFont mono = QFont("Monospace", 9);
    mono.setStyleHint(QFont::TypeWriter);
    ui->listQuadStruct->setFont(mono);
    ui->listQuadStruct->setColumnWidth(0, 120);
    ui->listQuadStruct->setColumnWidth(1, 80);
    ui->listQuadStruct->setColumnWidth(2, 160);
    ui->listQuadStruct->setColumnWidth(3, 120);

    for (int i = 0, n = ui->comboBoxMC->count(); i < n; i++)
    {
        const std::string& mcs = ui->comboBoxMC->itemText(i).toStdString();
        int mc = str2mc(mcs.c_str());
        if (mc < 0)
            qDebug() << "Unknown MC version: " << mcs.c_str();
        ui->comboBoxMC->setItemData(i, QVariant::fromValue(mc), Qt::UserRole);
        if (mc < MC_1_4)
            ui->comboBoxMC->setItemData(i, false, Qt::UserRole-1);
    }

    loadSeed();
    refresh();
}

QuadListDialog::~QuadListDialog()
{
    delete ui;
}


void QuadListDialog::loadSeed()
{
    ui->comboBoxMC->setCurrentText("1.18");
    ui->lineSeed->clear();

    WorldInfo wi;
    mainwindow->getSeed(&wi, false);

    const char *mcstr = mc2str(wi.mc);
    if (!mcstr)
    {
        qDebug() << "Unknown MC version: " << wi.mc;
        return;
    }

    ui->comboBoxMC->setCurrentText(mcstr);
    ui->lineSeed->setText(QString::asprintf("%" PRId64, (int64_t)wi.seed));
}

bool QuadListDialog::getSeed(WorldInfo *wi)
{
    // init using mainwindow
    bool ok = mainwindow->getSeed(wi, false);
    wi->mc = ui->comboBoxMC->currentData(Qt::UserRole).toInt();
    if (wi->mc < 0)
    {
        wi->mc = MC_NEWEST;
        qDebug() << "Unknown MC version: " << wi->mc;
        ok = false;
    }

    int v = str2seed(ui->lineSeed->text(), &wi->seed);
    if (v == S_RANDOM)
    {
        ui->lineSeed->setText(QString::asprintf("%" PRId64, (int64_t)wi->seed));
    }

    return ok;
}


void QuadListDialog::refresh()
{
    ui->listQuadStruct->setRowCount(0);
    ui->labelMsg->clear();

    WorldInfo wi;
    if (!getSeed(&wi))
        return;

    Generator g;
    setupGenerator(&g, wi.mc, wi.large);
    applySeed(&g, 0, wi.seed);

    QVector<QuadInfo> qsinfo;

    findQuadStructs(Swamp_Hut, &g, &qsinfo);
    findQuadStructs(Monument, &g, &qsinfo);

    int row = 0, qhn = 0, qmn = 0;
    for (QuadInfo& qi : qsinfo)
    {
        const char *label;
        if (qi.typ == Swamp_Hut)
        {
            label = "Quad-Hut";
            qhn++;
        }
        else
        {
            label = "Quad-Monument";
            qmn++;
        }

        QVariant var = QVariant::fromValue(qi.afk);
        qreal dist = qi.afk.x*(qreal)qi.afk.x + qi.afk.z*(qreal)qi.afk.z;
        dist = sqrt(dist);

        ui->listQuadStruct->insertRow(row);

        QTableWidgetItem* stritem = new QTableWidgetItem(label);
        stritem->setData(Qt::UserRole, var);
        ui->listQuadStruct->setItem(row, 0, stritem);

        QTableWidgetItem* distitem = new QTableWidgetItem();
        distitem->setData(Qt::UserRole, var);
        distitem->setData(Qt::DisplayRole, (int)round(dist));
        ui->listQuadStruct->setItem(row, 1, distitem);

        QTableWidgetItem* afkitem = new QTableWidgetItem();
        afkitem->setData(Qt::UserRole, var);
        afkitem->setText(QString::asprintf("(%d,%d)", qi.afk.x, qi.afk.z));
        ui->listQuadStruct->setItem(row, 2, afkitem);

        QTableWidgetItem* raditem = new QTableWidgetItem();
        raditem->setData(Qt::UserRole, var);
        raditem->setText(QString::asprintf("%.1f", qi.rad));
        ui->listQuadStruct->setItem(row, 3, raditem);

        QTableWidgetItem* cntitem = new QTableWidgetItem();
        cntitem->setData(Qt::UserRole, var);
        cntitem->setText(QString::asprintf("%d", qi.spcnt));
        ui->listQuadStruct->setItem(row, 4, cntitem);
        row++;
    }

    ui->listQuadStruct->setSortingEnabled(true);
    ui->listQuadStruct->sortByColumn(1, Qt::AscendingOrder);

    if (qhn == 0 && qmn == 0)
        ui->labelMsg->setText("World contains no quad-structures.");
    else if (qhn && qmn)
        ui->labelMsg->setText(QString::asprintf("World contains %d quad-hut%s and %d quad-monument%s.", qhn, qhn==1?"":"s", qmn, qmn==1?"":"s"));
    else if (qhn)
        ui->labelMsg->setText(QString::asprintf("World contains %d quad-hut%s.", qhn, qhn==1?"":"s"));
    else if (qmn)
        ui->labelMsg->setText(QString::asprintf("World contains %d quad-monument%s.", qmn, qmn==1?"":"s"));
}

void QuadListDialog::on_buttonGo_clicked()
{
    refresh();
}

void QuadListDialog::on_listQuadStruct_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction("Show in map viewer", this, &QuadListDialog::gotoSwampHut);
    menu.exec(ui->listQuadStruct->mapToGlobal(pos));
}

void QuadListDialog::gotoSwampHut()
{
    MapView *mapView = mainwindow->getMapView();
    QTableWidgetItem *item = ui->listQuadStruct->currentItem();
    if (!item)
        return;

    WorldInfo wi;
    if (!getSeed(&wi))
        return;

    QVariant dat = item->data(Qt::UserRole);
    if (dat.isValid())
    {
        Pos p = qvariant_cast<Pos>(dat);
        mainwindow->setSeed(wi);
        mapView->setView(p.x+0.5, p.z+0.5);
    }
}

void QuadListDialog::on_buttonClose_clicked()
{
    close();
}
