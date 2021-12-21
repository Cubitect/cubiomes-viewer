#pragma once

#include <QObject>

#include <QtCore>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#define CURRENT_VERSION "1.12.1"

class Updater : public QObject
{
Q_OBJECT
    QNetworkAccessManager *manager;
    bool startup;
private slots:
    void replyFinished(QNetworkReply *);
public:
    explicit Updater(bool s);
    void searchForUpdates();
};
