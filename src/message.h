#ifndef MESSAGE_H
#define MESSAGE_H

#include <QMessageBox>

int warn(QWidget *parent, const QString& title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok);
int warn(QWidget *parent, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok);
int info(QWidget *parent, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok);

#endif // MESSAGE_H
