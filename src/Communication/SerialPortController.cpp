#include "SerialPortController.h"
#include <QDebug>

SerialPortController::SerialPortController(QObject *parent) : ISerialTransport(parent)
{
    m_serialPort = new QSerialPort(this);
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialPortController::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialPortController::handleError);
}

SerialPortController::~SerialPortController()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
}

QStringList SerialPortController::getAvailablePorts() const
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ports << info.portName() + " - " + info.description();
    }
    return ports;
}

bool SerialPortController::connectDevice(const QString& portName, int baudRate, QSerialPort::DataBits dataBits,
                                         QSerialPort::Parity parity, QSerialPort::StopBits stopBits,
                                         QSerialPort::FlowControl flowControl)
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    // Extract actual port name (e.g., "COM3" from "COM3 - USB Serial Device")
    QString actualPortName = portName.split(" - ").first();

    m_serialPort->setPortName(actualPortName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setFlowControl(flowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        emit connectionStateChanged(true);
        return true;
    } else {
        emit connectionStateChanged(false, m_serialPort->errorString());
        return false;
    }
}

void SerialPortController::disconnectDevice()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit connectionStateChanged(false);
    }
}

bool SerialPortController::writeData(const QByteArray& data)
{
    if (m_serialPort->isOpen()) {
        qint64 bytesWritten = m_serialPort->write(data);
        return bytesWritten != -1;
    }
    return false;
}

bool SerialPortController::isOpen() const
{
    return m_serialPort->isOpen();
}

void SerialPortController::handleReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void SerialPortController::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
        // Disconnect immediately on critical errors like unplugging the device
        if (error == QSerialPort::ResourceError) {
            disconnectDevice();
            emit connectionStateChanged(false, "Device disconnected unexpectedly.");
        }
    }
}
