#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = nullptr);
    void checkForUpdates(bool silent = false);

signals:
    void updateAvailable(const QString& version, const QString& url, bool isSilent);
    void noUpdateAvailable();
    void errorOccurred(const QString& errorMsg);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    const QString repoOwner = "hakanyz";
    const QString repoName = "baudix";
    const QString currentVersion = "v1.0.0";
    bool m_isSilent = false;
};

#endif // UPDATER_H
