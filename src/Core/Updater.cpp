#include "Updater.h"
#include <QDebug>

Updater::Updater(QObject *parent) : QObject(parent)
{
}

void Updater::checkForUpdates()
{
    // TODO: Implement GitHub Releases API check here (similar to loopgit).
    // For now, this is just a stub.
    qDebug() << "Checking for updates...";
}
