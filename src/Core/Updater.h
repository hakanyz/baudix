#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = nullptr);
    void checkForUpdates(bool silent = false);
    void downloadUpdate(const QString& downloadUrl);

signals:
    void updateAvailable(const QString& version, const QString& url, bool isSilent);
    void noUpdateAvailable();
    void errorOccurred(const QString& errorMsg);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString& filePath);

private slots:
    void onReplyFinished(QNetworkReply *reply);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();

private:
    QNetworkAccessManager *networkManager;
    const QString repoOwner = "hakanyz";
    const QString repoName = "baudix";

#ifndef BAUDIX_VERSION_STR
#define BAUDIX_VERSION_STR "1.2.18"
#endif

    const QString currentVersion = "v" BAUDIX_VERSION_STR;
    bool m_isSilent = false;
    QNetworkReply* m_downloadReply = nullptr;
    QFile* m_downloadFile = nullptr;
    QString m_downloadFilePath;
};

#endif // UPDATER_H
