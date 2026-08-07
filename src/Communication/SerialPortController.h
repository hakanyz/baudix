#ifndef SERIALPORTCONTROLLER_H
#define SERIALPORTCONTROLLER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QStringList>
#include <QThread>

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
    quint64 errorCount() const override { return m_errCount; }

private slots:
    void onWorkerCountersUpdated(quint64 tx, quint64 rx, quint64 err);
    void onWorkerConnectionStateChanged(bool isOpen, const QString& errorMsg);

private:
    QThread* m_workerThread;
    class SerialWorker* m_worker;

    // Cached state for synchronous access
    quint64 m_txBytes = 0;
    quint64 m_rxBytes = 0;
    quint64 m_errCount = 0;
    bool m_isOpen = false;
};

#endif // SERIALPORTCONTROLLER_H
