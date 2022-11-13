#include "gotodialog.h"
#include "ui_gotodialog.h"

#include "mapview.h"

#include <QDoubleValidator>
#include <QKeyEvent>
#include <QClipboard>
#include <QMessageBox>


GotoDialog::GotoDialog(MapView *map, qreal x, qreal z, qreal scale)
    : QDialog(map)
    , ui(new Ui::GotoDialog)
    , mapview(map)
{
    ui->setupUi(this);

    scalemin = 1.0 / 4096;
    scalemax = 65536;
    ui->lineX->setValidator(new QDoubleValidator(-3e7, 3e7, 1, ui->lineX));
    ui->lineZ->setValidator(new QDoubleValidator(-3e7, 3e7, 1, ui->lineZ));
    ui->lineScale->setValidator(new QDoubleValidator(scalemin, scalemax, 16, ui->lineScale));

    ui->lineX->setText(QString::asprintf("%.1f", x));
    ui->lineZ->setText(QString::asprintf("%.1f", z));
    ui->lineScale->setText(QString::asprintf("%.4f", scale));
}

GotoDialog::~GotoDialog()
{
    delete ui;
}

void GotoDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::StandardButton b = ui->buttonBox->standardButton(button);

    if (b == QDialogButtonBox::Ok || b == QDialogButtonBox::Apply)
    {
        qreal x = ui->lineX->text().toDouble();
        qreal z = ui->lineZ->text().toDouble();
        qreal scale = ui->lineScale->text().toDouble();
        if (scale > 4096)
        {
            int button = QMessageBox::warning(this, tr("Unsafe Scale"),
                tr("Setting a very large scale may be unsafe.\n"
                   "Continue anyway?"), QMessageBox::Abort|QMessageBox::Yes);
            if (button == QMessageBox::Abort)
                return;
        }
        if (scale < scalemin) scale = scalemin;
        if (scale > scalemax) scale = scalemax;
        ui->lineScale->setText(QString::asprintf("%.4f", scale));
        mapview->setView(x, z, scale);
    }
    else if (b == QDialogButtonBox::Reset)
    {
        ui->lineX->setText("0");
        ui->lineZ->setText("0");
        ui->lineScale->setText("16");
    }
}

void GotoDialog::keyReleaseEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Paste))
    {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QStringList xz = clipboard->text().split(" ");
        if (xz.count() == 2)
        {
            ui->lineX->setText(xz[0].section(",", 0, 0));
            ui->lineZ->setText(xz[1].section(",", 0, 0));
            return;
        }
    }
    QWidget::keyReleaseEvent(event);
}

