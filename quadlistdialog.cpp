#include "quadlistdialog.h"
#include "ui_quadlistdialog.h"

#include "mapview.h"

#include "search.h"

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
    ui->listQuadHuts->setFont(mono);
    ui->listQuadHuts->setColumnWidth(0, 80);
    ui->listQuadHuts->setColumnWidth(1, 160);

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

    const QByteArray& ba = ui->lineSeed->text().toLocal8Bit();
    int v = str2seed(ba.data(), seed);
    if (v == S_RANDOM)
        ui->lineSeed->setText(QString::asprintf("%" PRId64, *seed));

    return true;
}

void QuadListDialog::refresh()
{
    ui->listQuadHuts->setRowCount(0);
    ui->labelMsg->clear();

    int mc;
    int64_t seed;
    if (!getSeed(&mc, &seed))
        return;

    LayerStack g;
    setupGenerator(&g, mc);

    StructureConfig sconf = mc >= MC_1_13 ? SWAMP_HUT_CONFIG : SWAMP_HUT_CONFIG_112;
    const int maxqh = 1000;
    Pos *qhlist = new Pos[maxqh];
    const int64_t *lbits = low20QuadHutBarely;
    int lbitcnt = sizeof(low20QuadHutBarely) / sizeof(int64_t);
    int r = 3e7 / 512;
    int qhcnt = 0;

    for (int i = 0; i < lbitcnt; i++)
    {
        int64_t l20 = lbits[i];
        qhcnt += scanForQuads(sconf, (seed + sconf.salt) & MASK48, l20, -r, -r, 2*r, 2*r, qhlist+qhcnt, maxqh-qhcnt);
    }
    if (qhcnt >= maxqh)
        QMessageBox::warning(this, "Warning", "Quad-hut scanning buffer exhausted, results will be incomplete.");

    ui->listQuadHuts->setSortingEnabled(false);
    int qhn = 0;
    for (int i = 0; i < qhcnt; i++)
    {
        Pos qh[4] = {
            getStructurePos(sconf, seed, qhlist[i].x+0, qhlist[i].z+0, 0),
            getStructurePos(sconf, seed, qhlist[i].x+0, qhlist[i].z+1, 0),
            getStructurePos(sconf, seed, qhlist[i].x+1, qhlist[i].z+0, 0),
            getStructurePos(sconf, seed, qhlist[i].x+1, qhlist[i].z+1, 0),
        };
        if (isViableStructurePos(sconf.structType, mc, &g, seed, qh[0].x, qh[0].z) &&
            isViableStructurePos(sconf.structType, mc, &g, seed, qh[1].x, qh[1].z) &&
            isViableStructurePos(sconf.structType, mc, &g, seed, qh[2].x, qh[2].z) &&
            isViableStructurePos(sconf.structType, mc, &g, seed, qh[3].x, qh[3].z))
        {
            ui->listQuadHuts->insertRow(qhn);
            Pos afk;
            afk = getOptimalAfk(qh, 7,7,9, 0);
            float rad = isQuadBaseFeature(sconf, moveStructure(seed, -qhlist[i].x, -qhlist[i].z), 7,7,9, 128);
            int dist = (int) round(sqrt(afk.x * (qreal)afk.x + afk.z * (qreal)afk.z));
            QVariant var = QVariant::fromValue(afk);

            QTableWidgetItem* distitem = new QTableWidgetItem();
            distitem->setData(Qt::UserRole, var);
            distitem->setData(Qt::DisplayRole, dist);
            ui->listQuadHuts->setItem(qhn, 0, distitem);

            QTableWidgetItem* afkitem = new QTableWidgetItem();
            afkitem->setData(Qt::UserRole, var);
            afkitem->setText(QString::asprintf("(%d,%d)", afk.x, afk.z));
            ui->listQuadHuts->setItem(qhn, 1, afkitem);

            QTableWidgetItem* raditem = new QTableWidgetItem();
            raditem->setData(Qt::UserRole, var);
            raditem->setText(QString::asprintf("%.1f", rad));
            ui->listQuadHuts->setItem(qhn, 2, raditem);
            qhn++;
        }
    }
    ui->listQuadHuts->setSortingEnabled(true);
    ui->listQuadHuts->sortByColumn(0, Qt::AscendingOrder);

    if (qhn == 0)
        ui->labelMsg->setText("World contains no quad-huts.");
    else
        ui->labelMsg->setText(QString::asprintf("World contains %d quad-hut%s.", qhn, qhn==1?"":"s"));

    delete[] qhlist;
}

void QuadListDialog::on_buttonGo_clicked()
{
    refresh();
}

void QuadListDialog::on_listQuadHuts_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction("Show in map viewer", this, &QuadListDialog::gotoSwampHut);
    menu.exec(ui->listQuadHuts->mapToGlobal(pos));
}

void QuadListDialog::gotoSwampHut()
{
    MapView *mapView = mainwindow->getMapView();
    QTableWidgetItem *item = ui->listQuadHuts->currentItem();
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
