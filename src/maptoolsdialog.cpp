#include "maptoolsdialog.h"
#include "ui_maptoolsdialog.h"

#include <QCheckBox>
#include <QDoubleValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

MapToolsDialog::MapToolsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MapToolsDialog)
    , mconfig()
    , modified()
{
    ui->setupUi(this);
    QGridLayout *grid = (QGridLayout*) ui->scrollContent->layout();

    int i = grid->rowCount();
    for (int opt = 0; opt < D_STRUCT_NUM; opt++)
    {
        if (opt == D_PORTALN)
            continue;
        QCheckBox *check = new QCheckBox();
        vui[opt].check = check;
        grid->addWidget(check, i, 0);

        QLabel *icon = new QLabel();
        icon->setPixmap(QPixmap(QString(":/icons/") + mapopt2str(opt)));
        grid->addWidget(icon, i, 1);

        QLabel *label = new QLabel(mapopt2display(opt));
        grid->addWidget(label, i, 2);

        if (mconfig.hasScale(opt))
        {
            label->setText(label->text() + ":");
            QLineEdit *line = new QLineEdit();
            line->setValidator(new QDoubleValidator(0.125, 256.0, 3, grid));
            vui[opt].line = line;
            grid->addWidget(line, i, 3);
        }
        i++;
    }

    mconfig.load();
    refresh();
}

MapToolsDialog::~MapToolsDialog()
{
    delete ui;
}

void MapToolsDialog::refresh()
{
    for (int opt = 0; opt < D_STRUCT_NUM; opt++)
    {
        if (vui[opt].check)
            vui[opt].check->setChecked(mconfig.enabled(opt));
        if (vui[opt].line)
            vui[opt].line->setText(QString::number(mconfig.scale(opt)));
    }
    ui->checkZoom->setChecked(mconfig.zoomEnabled);
}

void MapToolsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    int role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::ResetRole)
    {
        mconfig.reset();
        refresh();
    }
    else if (role == QDialogButtonBox::AcceptRole || role == QDialogButtonBox::ApplyRole)
    {
        for (int opt = 0; opt < D_STRUCT_NUM; opt++)
        {
            MapConfig::Opt *p_opt = &mconfig.opts[opt];
            if (vui[opt].check)
            {
                double enabled = vui[opt].check->isChecked();
                modified |= p_opt->enabled != enabled;
                p_opt->enabled = enabled;
            }
            if (vui[opt].line)
            {
                bool ok;
                double scale = vui[opt].line->text().toDouble(&ok);
                if (ok)
                {
                    modified |= p_opt->scale != scale;
                    p_opt->scale = scale;
                }
            }
        }
        mconfig.opts[D_PORTALN] = mconfig.opts[D_PORTAL];
        mconfig.zoomEnabled = ui->checkZoom->isChecked();
        mconfig.save();
        emit updateMapConfig();
    }
}

