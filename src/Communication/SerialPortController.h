#ifndef SERIALPORTCONTROLLER_H
#define SERIALPORTCONTROLLER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QStringList>
#include <QTimer>

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

    bool sendFile(const QString& filePath) override;
    void resetCounters() override;
    quint64 txBytes() const override { return m_txBytes; }
    quint64 rxBytes() const override { return m_rxBytes; }

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
    void handleBytesWritten(qint64 bytes);

private:
    QSerialPort *m_serialPort;
    quint64 m_txBytes = 0;
    quint64 m_rxBytes = 0;
    
    // RX Framing
    QByteArray m_rxBuffer;
    QTimer* m_framingTimer = nullptr;
    static constexpr int kFramingTimeoutMs = 40;  // ms of silence = end of frame
    static constexpr int kMaxFrameSize    = 2048; // bytes before forced flush
    void flushRxBuffer();
    
    // File Transfer State
    class QFile* m_sendFile = nullptr;
    qint64 m_sendFileTotalBytes = 0;
    qint64 m_sendFileBytesWritten = 0;
    void sendNextFileChunk();
};

#endif // SERIALPORTCONTROLLER_H
