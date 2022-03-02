#ifndef EXAMPLESDIALOG_H
#define EXAMPLESDIALOG_H

#include "settings.h"
#include "formconditions.h"

#include <QDialog>

namespace Ui {
class ExamplesDialog;
}

class QRadioButton;

class ExamplesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExamplesDialog(QWidget *parent, WorldInfo wi);
    ~ExamplesDialog();

    void addExample(QString rc, QString title, QString desc, QString comment, bool enabled);
    QString getExample();

private slots:
    void onSelectionChange(QRadioButton *radio);

private:
    Ui::ExamplesDialog *ui;
    struct Texts {
        QString rc, title, desc, comment;
    };
    std::map<QRadioButton*, Texts> examples;
    FormConditions *formconds;
};

#endif // EXAMPLESDIALOG_H
