#ifndef EXAMPLESDIALOG_H
#define EXAMPLESDIALOG_H

#include "settings.h"

#include <QDialog>

namespace Ui {
class ExamplesDialog;
}

class QListWidgetItem;

class ExamplesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExamplesDialog(QWidget *parent, WorldInfo wi);
    ~ExamplesDialog();

    void addExample(QString rc, QString desc, bool enabled);
    QString getExample();

private slots:
    void on_listExamples_currentRowChanged(int currentRow);

    void on_listExamples_itemDoubleClicked(QListWidgetItem *item);

private:
    Ui::ExamplesDialog *ui;
};

#endif // EXAMPLESDIALOG_H
