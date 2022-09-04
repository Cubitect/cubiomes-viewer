#include "structuredialog.h"
#include "ui_structuredialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QPushButton>

#include "world.h"
#include "cutil.h"

StructureDialog::StructureDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StructureDialog)
    , modified()
{
    ui->setupUi(this);
    QGridLayout *grid = new QGridLayout(ui->groupVis);

    loadStructVis(structvis);

    grid->addWidget(new QLabel(tr("blocks per pixel")), 0, 2);

    int i = 1;
    for (auto it : structvis)
    {
        int opt = it.first;
        double scale = it.second;
        int j = 0;

        QLabel *icon = new QLabel();
        icon->setPixmap(getMapIcon(opt));
        grid->addWidget(icon, i, j++);

        QString name = struct2str(mapopt2stype(opt));
        QLabel *label = new QLabel(name + ":");
        grid->addWidget(label, i, j++);

        QLineEdit *line = new QLineEdit();
        line->setValidator(new QDoubleValidator(0.125, 256.0, 3, grid));
        line->setText(QString::number(scale));
        entries[it.first] = line;
        grid->addWidget(line, i, j++);
        i++;
    }

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &StructureDialog::onAccept);
    QPushButton *reset = ui->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    connect(reset, &QPushButton::clicked, this, &StructureDialog::onReset);
}

StructureDialog::~StructureDialog()
{
    delete ui;
}

void StructureDialog::onAccept()
{
    for (auto it : entries)
    {
        QLineEdit *line = it.second;
        modified |= line->isModified();
        bool ok;
        double scale = line->text().toDouble(&ok);
        if (ok)
            structvis[it.first] = scale;
    }
}

void StructureDialog::onReset()
{
    double scale = 32;
    for (auto it : entries)
        it.second->setText(QString::number(scale));
    modified = true;
}
