#include "quadlistdialog.h"
#include "ui_quadlistdialog.h"

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
    ui->listQuadStruct->setColumnWidth(0, 100);
    ui->listQuadStruct->setColumnWidth(1, 80);
    ui->listQuadStruct->setColumnWidth(2, 160);

    loadSeed();
    refresh();
}

QuadListDialog::~QuadListDialog()
{
    delete ui;
}


void QuadListDialog::loadSeed()
{
    ui->comboBoxMC->setCurrentText("1.16");
    ui->lineSeed->clear();

    int mc;
    int64_t seed;
    mainwindow->getSeed(&mc, &seed, false);

    const char *mcstr = mc2str(mc);
    if (!mcstr)
    {
        qDebug() << "Unknown MC version: " << mc;
        return;
    }

    ui->comboBoxMC->setCurrentText(mcstr);
    ui->lineSeed->setText(QString::asprintf("%" PRId64, seed));
}

bool QuadListDialog::getSeed(int *mc, int64_t *seed)
{
    const std::string& mcs = ui->comboBoxMC->currentText().toStdString();
    *mc = str2mc(mcs.c_str());
    if (*mc < 0)
    {
        qDebug() << "Unknown MC version: " << *mc;
        return false;
    }

    int v = str2seed(ui->lineSeed->text(), seed);
    if (v == S_RANDOM)
        ui->lineSeed->setText(QString::asprintf("%" PRId64, *seed));

    return true;
}


void QuadListDialog::refresh()
{
    ui->listQuadStruct->setRowCount(0);
    ui->labelMsg->clear();

    int mc;
    int64_t seed;
    if (!getSeed(&mc, &seed))
        return;

    LayerStack g;
    setupGenerator(&g, mc);

    StructureConfig sconf = mc >= MC_1_13 ? SWAMP_HUT_CONFIG : SWAMP_HUT_CONFIG_112;
    const int maxq = 1000;
    Pos *qlist = new Pos[maxq];
    int r = 3e7 / 512;
    int qcnt;

    qcnt = scanForQuads(
                sconf, 128, (seed) & MASK48,
                low20QuadHutBarely, sizeof(low20QuadHutBarely) / sizeof(int64_t), 20, sconf.salt,
                -r, -r, 2*r, 2*r, qlist, maxq);

    if (qcnt >= maxq)
        QMessageBox::warning(this, "Warning", "Quad-hut scanning buffer exhausted, results will be incomplete.");

    ui->listQuadStruct->setSortingEnabled(false);
    int row = 0, qhn = 0, qmn = 0;
    for (int i = 0; i < qcnt; i++)
    {
        Pos qh[4] = {
            getStructurePos(sconf, seed, qlist[i].x+0, qlist[i].z+0, 0),
            getStructurePos(sconf, seed, qlist[i].x+0, qlist[i].z+1, 0),
            getStructurePos(sconf, seed, qlist[i].x+1, qlist[i].z+0, 0),
            getStructurePos(sconf, seed, qlist[i].x+1, qlist[i].z+1, 0),
        };
        if (isViableStructurePos(sconf.structType, mc, &g, seed, qh[0].x, qh[0].z) &&
            isViableStructurePos(sconf.structType, mc, &g, seed, qh[1].x, qh[1].z) &&
            isViableStructurePos(sconf.structType, mc, &g, seed, qh[2].x, qh[2].z) &&
            isViableStructurePos(sconf.structType, mc, &g, seed, qh[3].x, qh[3].z))
        {
            ui->listQuadStruct->insertRow(row);
            Pos afk;
            afk = getOptimalAfk(qh, 7,7,9, 0);
            float rad = isQuadBase(sconf, moveStructure(seed, -qlist[i].x, -qlist[i].z), 128);
            int dist = (int) round(sqrt(afk.x * (qreal)afk.x + afk.z * (qreal)afk.z));
            QVariant var = QVariant::fromValue(afk);

            QTableWidgetItem* stritem = new QTableWidgetItem("Quad-Hut");
            stritem->setData(Qt::UserRole, var);
            ui->listQuadStruct->setItem(row, 0, stritem);

            QTableWidgetItem* distitem = new QTableWidgetItem();
            distitem->setData(Qt::UserRole, var);
            distitem->setData(Qt::DisplayRole, dist);
            ui->listQuadStruct->setItem(row, 1, distitem);

            QTableWidgetItem* afkitem = new QTableWidgetItem();
            afkitem->setData(Qt::UserRole, var);
            afkitem->setText(QString::asprintf("(%d,%d)", afk.x, afk.z));
            ui->listQuadStruct->setItem(row, 2, afkitem);

            QTableWidgetItem* raditem = new QTableWidgetItem();
            raditem->setData(Qt::UserRole, var);
            raditem->setText(QString::asprintf("%.1f", rad));
            ui->listQuadStruct->setItem(row, 3, raditem);
            row++;
        }
    }
    qhn = row;

    if (mc >= MC_1_8)
    {
        sconf = MONUMENT_CONFIG;
        qcnt = scanForQuads(
                    sconf, 160, seed & MASK48,
                    g_qm_90, sizeof(g_qm_90) / sizeof(int64_t), 48, 0,
                    -r, -r, 2*r, 2*r, qlist, maxq);

        if (qcnt >= maxq)
            QMessageBox::warning(this, "Warning", "Quad-monument scanning buffer exhausted, results will be incomplete.");

        for (int i = 0; i < qcnt; i++)
        {
            Pos qm[4] = {
                getStructurePos(sconf, seed, qlist[i].x+0, qlist[i].z+0, 0),
                getStructurePos(sconf, seed, qlist[i].x+0, qlist[i].z+1, 0),
                getStructurePos(sconf, seed, qlist[i].x+1, qlist[i].z+0, 0),
                getStructurePos(sconf, seed, qlist[i].x+1, qlist[i].z+1, 0),
            };
            if (isViableStructurePos(sconf.structType, mc, &g, seed, qm[0].x, qm[0].z) &&
                isViableStructurePos(sconf.structType, mc, &g, seed, qm[1].x, qm[1].z) &&
                isViableStructurePos(sconf.structType, mc, &g, seed, qm[2].x, qm[2].z) &&
                isViableStructurePos(sconf.structType, mc, &g, seed, qm[3].x, qm[3].z))
            {
                ui->listQuadStruct->insertRow(row);
                Pos afk;
                afk = getOptimalAfk(qm, 58,23,58, 0);
                afk.x -= 29; afk.z -= 29; // monuments position is centered
                float rad = isQuadBase(sconf, moveStructure(seed, -qlist[i].x, -qlist[i].z), 160);
                int dist = (int) round(sqrt(afk.x * (qreal)afk.x + afk.z * (qreal)afk.z));
                QVariant var = QVariant::fromValue(afk);

                QTableWidgetItem* stritem = new QTableWidgetItem("Quad-Monument");
                stritem->setData(Qt::UserRole, var);
                ui->listQuadStruct->setItem(row, 0, stritem);

                QTableWidgetItem* distitem = new QTableWidgetItem();
                distitem->setData(Qt::UserRole, var);
                distitem->setData(Qt::DisplayRole, dist);
                ui->listQuadStruct->setItem(row, 1, distitem);

                QTableWidgetItem* afkitem = new QTableWidgetItem();
                afkitem->setData(Qt::UserRole, var);
                afkitem->setText(QString::asprintf("(%d,%d)", afk.x, afk.z));
                ui->listQuadStruct->setItem(row, 2, afkitem);

                QTableWidgetItem* raditem = new QTableWidgetItem();
                raditem->setData(Qt::UserRole, var);
                raditem->setText(QString::asprintf("%.1f", rad));
                ui->listQuadStruct->setItem(row, 3, raditem);
                row++;
            }
        }
    }
    qmn = row - qhn;

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

    delete[] qlist;
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

    int mc;
    int64_t seed;
    if (!getSeed(&mc, &seed))
        return;

    QVariant dat = item->data(Qt::UserRole);
    if (dat.isValid())
    {
        Pos p = qvariant_cast<Pos>(dat);
        mainwindow->setSeed(mc, seed);
        mapView->setView(p.x+0.5, p.z+0.5);
    }
}

void QuadListDialog::on_buttonClose_clicked()
{
    close();
}
