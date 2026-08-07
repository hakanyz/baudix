#ifndef ISERIALTRANSPORT_H
#define ISERIALTRANSPORT_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QSerialPort>

class ISerialTransport : public QObject
{
    Q_OBJECT
public:
    explicit ISerialTransport(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ISerialTransport() = default;

    virtual QStringList getAvailablePorts() const = 0;
    virtual bool connectDevice(const QString& portName, int baudRate, QSerialPort::DataBits dataBits,
                               QSerialPort::Parity parity, QSerialPort::StopBits stopBits,
                               QSerialPort::FlowControl flowControl) = 0;
    virtual void disconnectDevice() = 0;
    virtual bool writeData(const QByteArray& data) = 0;
    virtual bool isOpen() const = 0;

    virtual bool sendFile(const QString& filePath) = 0;
    virtual void resetCounters() = 0;
    virtual quint64 txBytes() const = 0;
    virtual quint64 rxBytes() const = 0;
    virtual quint64 errorCount() const = 0;

signals:
    void dataReceived(const QByteArray& data);
    void dataSent(const QByteArray& data);
    void connectionStateChanged(bool isOpen, const QString& errorMsg = "");
    void countersUpdated(quint64 tx, quint64 rx, quint64 err);
    void fileTransferProgress(qint64 bytesSent, qint64 bytesTotal);
    void fileTransferFinished();
    void fileTransferError(const QString& error);
};

#endif // ISERIALTRANSPORT_H
