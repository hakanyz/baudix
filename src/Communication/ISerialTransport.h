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

signals:
    void dataReceived(const QByteArray& data);
    void connectionStateChanged(bool isOpen, const QString& errorMsg = "");
};

#endif // ISERIALTRANSPORT_H
