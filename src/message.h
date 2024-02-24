#ifndef MESSAGE_H
#define MESSAGE_H

#include <QMessageBox>

int warn(QWidget *parent, const QString& title, const QString& text, const QString& prompt, QMessageBox::StandardButtons buttons);
void warn(QWidget *parent, const QString& title, const QString& text);
void warn(QWidget *parent, const QString& text);
void info(QWidget *parent, const QString& text);

#endif // MESSAGE_H
