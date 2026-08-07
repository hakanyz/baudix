#include "SerialPortController.h"
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QMetaObject>
#include <QThread>

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(QObject* parent = nullptr) : QObject(parent) {
        m_serialPort = new QSerialPort(this);
        connect(m_serialPort, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
        connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialWorker::handleError);
        connect(m_serialPort, &QSerialPort::bytesWritten, this, &SerialWorker::handleBytesWritten);

        m_framingTimer = new QTimer(this);
        m_framingTimer->setSingleShot(true);
        connect(m_framingTimer, &QTimer::timeout, this, &SerialWorker::flushRxBuffer);
    }

    ~SerialWorker() {
        if (m_sendFile) {
            if (m_sendFile->isOpen()) m_sendFile->close();
            delete m_sendFile;
        }
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }
    }

public slots:
    bool connectDevice(const QString& portName, int baudRate, QSerialPort::DataBits dataBits,
                       QSerialPort::Parity parity, QSerialPort::StopBits stopBits,
                       QSerialPort::FlowControl flowControl) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }

        m_serialPort->setPortName(portName);
        m_serialPort->setBaudRate(baudRate);
        m_serialPort->setDataBits(dataBits);
        m_serialPort->setParity(parity);
        m_serialPort->setStopBits(stopBits);
        m_serialPort->setFlowControl(flowControl);

        if (m_serialPort->open(QIODevice::ReadWrite)) {
            emit connectionStateChanged(true, "");
            return true;
        } else {
            emit connectionStateChanged(false, m_serialPort->errorString());
            return false;
        }
    }

    void disconnectDevice() {
        if (m_serialPort->isOpen()) {
            if (m_sendFile) {
                m_sendFile->close();
                delete m_sendFile;
                m_sendFile = nullptr;
            }
            flushRxBuffer();
            m_framingTimer->stop();
            m_serialPort->close();
            emit connectionStateChanged(false, "");
        }
    }

    void writeData(const QByteArray& data) {
        if (m_serialPort->isOpen()) {
            m_serialPort->write(data);
        }
    }

    void resetCounters() {
        m_txBytes = 0;
        m_rxBytes = 0;
        m_errCount = 0;
        emit countersUpdated(m_txBytes, m_rxBytes, m_errCount);
    }

    void sendFile(const QString& filePath) {
        if (!m_serialPort->isOpen()) return;

        if (m_sendFile) {
            if (m_sendFile->isOpen()) m_sendFile->close();
            delete m_sendFile;
            m_sendFile = nullptr;
        }

        m_sendFile = new QFile(filePath, this);
        if (!m_sendFile->open(QIODevice::ReadOnly)) {
            delete m_sendFile;
            m_sendFile = nullptr;
            emit fileTransferError("Cannot open file.");
            return;
        }

        m_sendFileTotalBytes = m_sendFile->size();
        m_sendFileBytesWritten = 0;
        sendNextFileChunk();
    }

signals:
    void dataReceived(const QByteArray& data);
    void connectionStateChanged(bool isOpen, const QString& errorMsg);
    void countersUpdated(quint64 tx, quint64 rx, quint64 err);
    void fileTransferProgress(qint64 bytesSent, qint64 bytesTotal);
    void fileTransferFinished();
    void fileTransferError(const QString& error);

private slots:
    void handleReadyRead() {
        QByteArray incoming = m_serialPort->readAll();
        if (incoming.isEmpty()) return;

        m_rxBuffer.append(incoming);

        while (m_rxBuffer.contains('\n')) {
            int nlIdx = m_rxBuffer.indexOf('\n');
            QByteArray frame = m_rxBuffer.left(nlIdx + 1);
            m_rxBuffer.remove(0, nlIdx + 1);
            m_rxBytes += frame.size();
            emit dataReceived(frame);
            emit countersUpdated(m_txBytes, m_rxBytes, m_errCount);
        }

        if (m_rxBuffer.size() >= kMaxFrameSize) {
            flushRxBuffer();
            return;
        }

        if (!m_rxBuffer.isEmpty()) {
            m_framingTimer->start(kFramingTimeoutMs);
        }
    }

    void flushRxBuffer() {
        if (m_rxBuffer.isEmpty()) return;
        m_rxBytes += m_rxBuffer.size();
        emit dataReceived(m_rxBuffer);
        emit countersUpdated(m_txBytes, m_rxBytes, m_errCount);
        m_rxBuffer.clear();
    }

    void handleError(QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
            m_errCount++;
            emit countersUpdated(m_txBytes, m_rxBytes, m_errCount);

            if (error == QSerialPort::ResourceError) {
                disconnectDevice();
                emit connectionStateChanged(false, "Device disconnected unexpectedly.");
            }
        }
    }

    void handleBytesWritten(qint64 bytes) {
        m_txBytes += bytes;
        emit countersUpdated(m_txBytes, m_rxBytes, m_errCount);

        if (m_sendFile && m_sendFile->isOpen()) {
            sendNextFileChunk();
        }
    }

private:
    void sendNextFileChunk() {
        if (!m_sendFile || !m_sendFile->isOpen()) return;

        if (m_sendFile->atEnd()) {
            m_sendFile->close();
            delete m_sendFile;
            m_sendFile = nullptr;
            emit fileTransferFinished();
            return;
        }

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

    QSerialPort *m_serialPort;
    quint64 m_txBytes = 0;
    quint64 m_rxBytes = 0;
    quint64 m_errCount = 0;

    QByteArray m_rxBuffer;
    QTimer* m_framingTimer = nullptr;
    static constexpr int kFramingTimeoutMs = 40;
    static constexpr int kMaxFrameSize = 2048;

    QFile* m_sendFile = nullptr;
    qint64 m_sendFileTotalBytes = 0;
    qint64 m_sendFileBytesWritten = 0;
};

SerialPortController::SerialPortController(QObject *parent) : ISerialTransport(parent)
{
    m_workerThread = new QThread(this);
    m_worker = new SerialWorker();
    
    // Move the worker to the dedicated thread
    m_worker->moveToThread(m_workerThread);

    // Forward worker signals to our signals (and slots to update cache)
    connect(m_worker, &SerialWorker::dataReceived, this, &SerialPortController::dataReceived);
    connect(m_worker, &SerialWorker::fileTransferProgress, this, &SerialPortController::fileTransferProgress);
    connect(m_worker, &SerialWorker::fileTransferFinished, this, &SerialPortController::fileTransferFinished);
    connect(m_worker, &SerialWorker::fileTransferError, this, &SerialPortController::fileTransferError);
    connect(m_worker, &SerialWorker::countersUpdated, this, &SerialPortController::onWorkerCountersUpdated);
    connect(m_worker, &SerialWorker::connectionStateChanged, this, &SerialPortController::onWorkerConnectionStateChanged);
    
    // Cleanup worker when thread finishes
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread->start();
}

SerialPortController::~SerialPortController()
{
    m_workerThread->quit();
    m_workerThread->wait();
}

QStringList SerialPortController::getAvailablePorts() const
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ports << info.portName() + " (" + info.description() + ")";
    }
    return ports;
}

bool SerialPortController::connectDevice(const QString& portName, int baudRate, QSerialPort::DataBits dataBits,
                                         QSerialPort::Parity parity, QSerialPort::StopBits stopBits,
                                         QSerialPort::FlowControl flowControl)
{
    QString actualPortName = portName.split(" (").first();
    bool result = false;
    
    // Invoke the connect method synchronously in the worker thread
    QMetaObject::invokeMethod(m_worker, "connectDevice", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool, result),
                              Q_ARG(QString, actualPortName),
                              Q_ARG(int, baudRate),
                              Q_ARG(QSerialPort::DataBits, dataBits),
                              Q_ARG(QSerialPort::Parity, parity),
                              Q_ARG(QSerialPort::StopBits, stopBits),
                              Q_ARG(QSerialPort::FlowControl, flowControl));
    return result;
}

void SerialPortController::disconnectDevice()
{
    QMetaObject::invokeMethod(m_worker, "disconnectDevice", Qt::QueuedConnection);
}

bool SerialPortController::writeData(const QByteArray& data)
{
    if (!m_isOpen) return false;
    
    // Invoke write asynchronously so large writes don't block the UI
    QMetaObject::invokeMethod(m_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, data));
    return true;
}

bool SerialPortController::isOpen() const
{
    return m_isOpen;
}

void SerialPortController::resetCounters()
{
    QMetaObject::invokeMethod(m_worker, "resetCounters", Qt::QueuedConnection);
}

bool SerialPortController::sendFile(const QString& filePath)
{
    if (!m_isOpen) return false;
    QMetaObject::invokeMethod(m_worker, "sendFile", Qt::QueuedConnection, Q_ARG(QString, filePath));
    return true;
}

void SerialPortController::onWorkerCountersUpdated(quint64 tx, quint64 rx, quint64 err)
{
    m_txBytes = tx;
    m_rxBytes = rx;
    m_errCount = err;
    emit countersUpdated(tx, rx, err);
}

void SerialPortController::onWorkerConnectionStateChanged(bool isOpen, const QString& errorMsg)
{
    m_isOpen = isOpen;
    emit connectionStateChanged(isOpen, errorMsg);
}

#include "SerialPortController.moc"

