#include "Updater.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

Updater::Updater(QObject *parent) : QObject(parent)
{
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &Updater::onReplyFinished);
}

void Updater::checkForUpdates()
{
    QString apiUrl = QString("https://api.github.com/repos/%1/%2/releases/latest").arg(repoOwner, repoName);
    QNetworkRequest request((QUrl(apiUrl)));
    
    // GitHub API requires a User-Agent header
    request.setRawHeader("User-Agent", "Baudix-Updater");
    
    networkManager->get(request);
    qDebug() << "Checking for updates at:" << apiUrl;
}

void Updater::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray response = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
    
    if (!jsonDoc.isNull() && jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        QString latestVersion = jsonObj["tag_name"].toString();
        QString htmlUrl = jsonObj["html_url"].toString();
        
        if (latestVersion > currentVersion) {
            emit updateAvailable(latestVersion, htmlUrl);
        } else {
            emit noUpdateAvailable();
        }
    } else {
        emit errorOccurred("Invalid JSON response from GitHub API.");
    }
    
    reply->deleteLater();
}
