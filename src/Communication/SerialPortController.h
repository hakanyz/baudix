#ifndef SERIALPORTCONTROLLER_H
#define SERIALPORTCONTROLLER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QStringList>

class SerialPortController : public QObject
{
    Q_OBJECT
public:
    explicit SerialPortController(QObject *parent = nullptr);
    ~SerialPortController();

    QStringList getAvailablePorts() const;
    bool connectDevice(const QString& portName, int baudRate, QSerialPort::DataBits dataBits,
                       QSerialPort::Parity parity, QSerialPort::StopBits stopBits,
                       QSerialPort::FlowControl flowControl);
    void disconnectDevice();
    bool writeData(const QByteArray& data);
    bool isOpen() const;

signals:
    void dataReceived(const QByteArray& data);
    void connectionStateChanged(bool isOpen, const QString& errorMsg = "");

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serialPort;
};

#endif // SERIALPORTCONTROLLER_H
