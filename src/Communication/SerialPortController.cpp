#include "SerialPortController.h"
#include <QDebug>
#include <QFile>

SerialPortController::SerialPortController(QObject *parent) : ISerialTransport(parent)
{
    m_serialPort = new QSerialPort(this);
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialPortController::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialPortController::handleError);
    connect(m_serialPort, &QSerialPort::bytesWritten, this, &SerialPortController::handleBytesWritten);
}

SerialPortController::~SerialPortController()
{
    if (m_sendFile) {
        if (m_sendFile->isOpen()) m_sendFile->close();
        delete m_sendFile;
    }
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
        if (m_sendFile) {
            m_sendFile->close();
            delete m_sendFile;
            m_sendFile = nullptr;
        }
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

void SerialPortController::resetCounters()
{
    m_txBytes = 0;
    m_rxBytes = 0;
    emit countersUpdated(m_txBytes, m_rxBytes);
}

void SerialPortController::handleReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    if (!data.isEmpty()) {
        m_rxBytes += data.size();
        emit countersUpdated(m_txBytes, m_rxBytes);
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

void SerialPortController::handleBytesWritten(qint64 bytes)
{
    m_txBytes += bytes;
    emit countersUpdated(m_txBytes, m_rxBytes);
    
    // If we have an active file transfer, continue sending chunks
    if (m_sendFile && m_sendFile->isOpen()) {
        sendNextFileChunk();
    }
}

bool SerialPortController::sendFile(const QString& filePath)
{
    if (!isOpen()) return false;
    
    if (m_sendFile) {
        if (m_sendFile->isOpen()) m_sendFile->close();
        delete m_sendFile;
        m_sendFile = nullptr;
    }
    
    m_sendFile = new QFile(filePath, this);
    if (!m_sendFile->open(QIODevice::ReadOnly)) {
        delete m_sendFile;
        m_sendFile = nullptr;
        return false;
    }
    
    m_sendFileTotalBytes = m_sendFile->size();
    m_sendFileBytesWritten = 0;
    
    // Start the transfer by sending the first chunk
    sendNextFileChunk();
    return true;
}

void SerialPortController::sendNextFileChunk()
{
    if (!m_sendFile || !m_sendFile->isOpen()) return;
    
    // Check if we reached the end
    if (m_sendFile->atEnd()) {
        m_sendFile->close();
        delete m_sendFile;
        m_sendFile = nullptr;
        emit fileTransferFinished();
        return;
    }
    
    // Read up to 4096 bytes chunk
    QByteArray chunk = m_sendFile->read(4096);
    if (chunk.isEmpty() && m_sendFile->error() != QFile::NoError) {
        QString err = m_sendFile->errorString();
        m_sendFile->close();
        delete m_sendFile;
        m_sendFile = nullptr;
        emit fileTransferError(err);
        return;
    }
    
    qint64 written = m_serialPort->write(chunk);
    if (written == -1) {
        m_sendFile->close();
        delete m_sendFile;
        m_sendFile = nullptr;
        emit fileTransferError(m_serialPort->errorString());
        return;
    }
    
    m_sendFileBytesWritten += chunk.size();
    emit fileTransferProgress(m_sendFileBytesWritten, m_sendFileTotalBytes);
}
