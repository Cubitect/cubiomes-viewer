#include "updater.h"

#include <QDesktopServices>
#include <QMessageBox>

Updater::Updater(bool s)
{
    startup = s;
}

void Updater::replyFinished(QNetworkReply *reply)
{
    QJsonParseError jsonError;
    QJsonDocument document = QJsonDocument::fromJson(  reply->readAll(), &jsonError );

    if( jsonError.error != QJsonParseError::NoError )
    {
        qDebug() << QString("Parsing update JSON failed: %1").arg(jsonError.errorString());
        return;
    }

    QJsonArray arr = document.array();
    QString newestVersion = arr[0].toObject().take("tag_name").toString();

    if (newestVersion == CURRENT_VERSION) {
        if (!startup) {
             QMessageBox::information(NULL, tr("No Updates"), tr("You using the latest version of cubiomes-viewer"));
        }
        return;
    }

    QMessageBox::StandardButton answer;
    answer = QMessageBox::question(NULL, tr("New Version"), tr("A new version is aviable. Do you want to download it?"), QMessageBox::Yes|QMessageBox::No);

    if (answer == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl(QString("https://github.com/Cubitect/cubiomes-viewer/releases/tag/%1").arg(newestVersion))); 
    }
}

void Updater::searchForUpdates()
{
    QUrl qrl("https://api.github.com/repos/Cubitect/cubiomes-viewer/releases");
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    manager->get(QNetworkRequest(qrl));
}
