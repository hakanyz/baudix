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

void Updater::checkForUpdates(bool silent)
{
    m_isSilent = silent;
    QString apiUrl = QString("https://api.github.com/repos/%1/%2/releases/latest").arg(repoOwner, repoName);
    QUrl url(apiUrl);
    QNetworkRequest request(url);
    
    // GitHub API requires a User-Agent header
    request.setRawHeader("User-Agent", "Baudix-Updater");
    
    networkManager->get(request);
    qDebug() << "Checking for updates at:" << apiUrl;
}

void Updater::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        if (!m_isSilent) {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
        return;
    }

    QByteArray response = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
    
    if (!jsonDoc.isNull() && jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        QString latestVersion = jsonObj["tag_name"].toString();
        QString htmlUrl = jsonObj["html_url"].toString();
        
        // Semantic version comparison (handles cases like v1.0.2 vs v1.0.10)
        auto isNewer = [](const QString& latest, const QString& current) {
            QString l = latest; l.remove('v');
            QString c = current; c.remove('v');
            QStringList lParts = l.split('.');
            QStringList cParts = c.split('.');
            for (int i = 0; i < std::max(lParts.size(), cParts.size()); ++i) {
                int lPart = i < lParts.size() ? lParts[i].toInt() : 0;
                int cPart = i < cParts.size() ? cParts[i].toInt() : 0;
                if (lPart > cPart) return true;
                if (lPart < cPart) return false;
            }
            return false;
        };

        if (isNewer(latestVersion, currentVersion)) {
            emit updateAvailable(latestVersion, htmlUrl, m_isSilent);
        } else {
            if (!m_isSilent) {
                emit noUpdateAvailable();
            }
        }
    } else {
        if (!m_isSilent) {
            emit errorOccurred("Invalid JSON response from GitHub API.");
        }
    }
    
    reply->deleteLater();
}
