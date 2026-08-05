#ifndef SERIALPORTCONTROLLER_H
#define SERIALPORTCONTROLLER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QStringList>

#include "ISerialTransport.h"

class SerialPortController : public ISerialTransport
{
    Q_OBJECT
public:
    explicit SerialPortController(QObject *parent = nullptr);
    ~SerialPortController() override;

    QStringList getAvailablePorts() const override;
    bool connectDevice(const QString& portName, int baudRate, QSerialPort::DataBits dataBits,
                       QSerialPort::Parity parity, QSerialPort::StopBits stopBits,
                       QSerialPort::FlowControl flowControl) override;
    void disconnectDevice() override;
    bool writeData(const QByteArray& data) override;
    bool isOpen() const override;

    bool sendFile(const QString& filePath);
    void resetCounters();
    quint64 txBytes() const { return m_txBytes; }
    quint64 rxBytes() const { return m_rxBytes; }

signals:
    void countersUpdated(quint64 tx, quint64 rx);
    void fileTransferProgress(qint64 bytesSent, qint64 bytesTotal);
    void fileTransferFinished();
    void fileTransferError(const QString& error);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void handleBytesWritten(qint64 bytes);

private:
    QSerialPort *m_serialPort;
    quint64 m_txBytes = 0;
    quint64 m_rxBytes = 0;
    
    // File Transfer State
    class QFile* m_sendFile = nullptr;
    qint64 m_sendFileTotalBytes = 0;
    qint64 m_sendFileBytesWritten = 0;
    void sendNextFileChunk();
};

#endif // SERIALPORTCONTROLLER_H
