#include "Updater.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
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
        
        // Find the Windows installer asset
        QString downloadUrl = jsonObj["html_url"].toString(); // fallback
        QJsonArray assets = jsonObj["assets"].toArray();
        for (const QJsonValue& val : assets) {
            QJsonObject asset = val.toObject();
            QString name = asset["name"].toString();
            if (name.endsWith(".exe", Qt::CaseInsensitive)) {
                downloadUrl = asset["browser_download_url"].toString();
                break;
            }
        }
        
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
            emit updateAvailable(latestVersion, downloadUrl, m_isSilent);
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

void Updater::downloadUpdate(const QString& downloadUrl)
{
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
    }
    
    if (m_downloadFile) {
        m_downloadFile->close();
        delete m_downloadFile;
    }

    m_downloadFilePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/Baudix_Update.exe";
    m_downloadFile = new QFile(m_downloadFilePath);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        emit errorOccurred("Could not open temp file for writing.");
        return;
    }

    QUrl url(downloadUrl);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Baudix-Updater");
    
    // Follow redirects is important for GitHub asset downloads
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_downloadReply = networkManager->get(request);
    
    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this](){
        if (m_downloadFile && m_downloadFile->isOpen()) {
            m_downloadFile->write(m_downloadReply->readAll());
        }
    });
    
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &Updater::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished, this, &Updater::onDownloadFinished);
}

void Updater::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void Updater::onDownloadFinished()
{
    if (m_downloadFile) {
        m_downloadFile->close();
        delete m_downloadFile;
        m_downloadFile = nullptr;
    }

    if (m_downloadReply->error() == QNetworkReply::NoError) {
        emit downloadFinished(m_downloadFilePath);
    } else {
        emit errorOccurred("Failed to download update: " + m_downloadReply->errorString());
    }

    m_downloadReply->deleteLater();
    m_downloadReply = nullptr;
}
