#include "src/rangedialog.h"
#include "ui_rangedialog.h"

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

bool RangeDialog::getBounds(uint64_t *smin, uint64_t *smax)
{
    bool ok, allok = true;
    int base = 10;
    if (ui->checkHex->isChecked())
        base = 16;
    *smin = ui->lineMin->text().toULongLong(&ok, base);
    allok &= ok;
    if (!ok)
        *smin = 0;
    *smax = ui->lineMax->text().toULongLong(&ok, base);
    allok &= ok;
    if (!ok)
        *smax = ~(uint64_t)0;
    return allok;
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
}
