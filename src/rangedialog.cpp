#include "src/rangedialog.h"
#include "ui_rangedialog.h"

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

RangeDialog::RangeDialog(QWidget *parent, uint64_t smin, uint64_t smax)
    : QDialog(parent)
    , ui(new Ui::RangeDialog)
{
    ui->setupUi(this);
    ui->lineMin->setText(QString::asprintf("%" PRIu64, smin));
    ui->lineMax->setText(QString::asprintf("%" PRIu64, smax));
}

RangeDialog::~RangeDialog()
{
    delete ui;
}

void RangeDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::StandardButton b = ui->buttonBox->standardButton(button);

    if (b == QDialogButtonBox::RestoreDefaults)
    {
        const char *fmt = "%" PRIu64;
        if (ui->checkHex->isChecked())
            fmt = "%" PRIx64;

        ui->lineMin->setText(QString::asprintf(fmt, 0));
        ui->lineMax->setText(QString::asprintf(fmt, ~(uint64_t)0));
    }
    if (b == QDialogButtonBox::Ok)
    {
        bool ok;
        int base = 10;
        if (ui->checkHex->isChecked())
            base = 16;
        uint64_t smin = ui->lineMin->text().toULongLong(&ok, base);
        if (!ok) smin = 0;
        uint64_t smax = ui->lineMax->text().toULongLong(&ok, base);
        if (!ok) smax = ~(uint64_t)0;
        emit applyBounds(smin, smax);
    }
}
