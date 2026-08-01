#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QString>

class Updater : public QObject
{
    QObject
public:
    explicit Updater(QObject *parent = nullptr);
    void checkForUpdates();

signals:
    void updateAvailable(const QString& version, const QString& url);
    void noUpdateAvailable();
    void errorOccurred(const QString& errorMsg);
};

#endif // UPDATER_H
