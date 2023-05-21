#include "structuredialog.h"
#include "ui_structuredialog.h"

#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QPushButton>

#include "cutil.h"
#include "world.h"

StructureDialog::StructureDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StructureDialog)
    , mconfig()
    , modified()
{
    ui->setupUi(this);
    QGridLayout *grid = new QGridLayout(ui->groupVis);

    grid->addWidget(new QLabel(tr("enabled")), 0, 0, 1, 3);
    grid->addWidget(new QLabel(tr("blocks per pixel")), 0, 3);

    int i = 1;
    for (int opt = D_DESERT; opt < D_SPAWN; opt++)
    {
        if (!mconfig.valid(opt))
            continue;
        int j = 0;

        QCheckBox *check = new QCheckBox();
        vui[opt].check = check;
        grid->addWidget(check, i, j++);

        QLabel *icon = new QLabel();
        icon->setPixmap(getMapIcon(opt));
        grid->addWidget(icon, i, j++);

        QString name = struct2str(mapopt2stype(opt));
        QLabel *label = new QLabel(name + ":");
        grid->addWidget(label, i, j++);

        QLineEdit *line = new QLineEdit();
        line->setValidator(new QDoubleValidator(0.125, 256.0, 3, grid));
        vui[opt].line = line;
        grid->addWidget(line, i, j++);
        i++;
    }

    mconfig.load();
    refresh();
}

StructureDialog::~StructureDialog()
{
    delete ui;
}

void StructureDialog::refresh()
{
    for (int opt = D_DESERT; opt < D_SPAWN; opt++)
    {
        if (vui[opt].check)
            vui[opt].check->setChecked(mconfig.enabled(opt));
        if (vui[opt].line)
            vui[opt].line->setText(QString::number(mconfig.scale(opt)));
    }
}

void StructureDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    int role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::ResetRole)
    {
        mconfig.reset();
        refresh();
    }
    else if (role == QDialogButtonBox::AcceptRole || role == QDialogButtonBox::ApplyRole)
    {
        for (int opt = D_DESERT; opt < D_SPAWN; opt++)
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
        mconfig.save();
        emit updateMapConfig();
    }
}

