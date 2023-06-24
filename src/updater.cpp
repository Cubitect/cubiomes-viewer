#include "updater.h"

#include "aboutdialog.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QMessageBox>
#include <QDesktopServices>


void replyFinished(QNetworkReply *reply, bool quiet)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        if (!quiet)
        {
            QString msg = QObject::translate("UpdaterDialog", "Failed to check for updates:\n");
            switch (reply->error())
            {
            case QNetworkReply::ConnectionRefusedError:
                msg += QObject::translate("UpdaterDialog", "Connection refused.");
                break;
            case QNetworkReply::HostNotFoundError:
                msg += QObject::translate("UpdaterDialog", "Host not found.");
                break;
            case QNetworkReply::TimeoutError:
                msg += QObject::translate("UpdaterDialog", "Connection timed out.");
                break;
            default:
                msg += QObject::translate("UpdaterDialog", "Network error (%1).").arg(reply->error());
                break;
            }
            QMessageBox::warning(NULL, QObject::translate("UpdaterDialog", "Connection Error"), msg, QMessageBox::Ok);
        }
        return;
    }

    QJsonParseError jerr;
    QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &jerr);

    if (jerr.error != QJsonParseError::NoError)
    {
        QMessageBox::warning(
            NULL, QObject::translate("UpdaterDialog", "Json Error"),
            QObject::translate("UpdaterDialog", "Failed to parse Json reply with:\n%1").arg(jerr.errorString()),
            QMessageBox::Ok);
        return;
    }

    QJsonArray jarr = document.array();
    QString newest;

    if (getVersStr().contains("dev"))
    {
        newest = jarr[0].toObject().take("tag_name").toString();
    }
    else
    {
        for (auto jval : jarr)
        {
            if (!jval.toObject().take("prerelease").toBool())
            {
                newest = jval.toObject().take("tag_name").toString();
                break;
            }
        }
    }

    if (newest == getVersStr())
    {
        if (!quiet)
        {
            QMessageBox::information(
                NULL, QObject::translate("UpdaterDialog", "No Updates"),
                QObject::translate("UpdaterDialog", "You using the latest version of Cubiomes-Viewer."));
        }
        return;
    }

    QMessageBox::StandardButton answer = QMessageBox::question(
        NULL, QObject::translate("UpdaterDialog", "New Version"),
        QObject::translate("UpdaterDialog", "<p>A new version: <b>%1</b> is available.</p><p>Open the download page in browser?</p>").arg(newest),
        QMessageBox::Yes|QMessageBox::No);

    if (answer == QMessageBox::Yes)
    {
        QString url = QString("https://github.com/Cubitect/cubiomes-viewer/releases/tag/%1").arg(newest);
        QDesktopServices::openUrl(QUrl(url));
    }
}

void searchForUpdates(bool quiet)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QObject::connect(manager, &QNetworkAccessManager::finished,
        [=](QNetworkReply *reply) {
            replyFinished(reply, quiet);
            manager->deleteLater();
        });

    QUrl qrl("https://api.github.com/repos/Cubitect/cubiomes-viewer/releases");
    manager->get(QNetworkRequest(qrl));
}

