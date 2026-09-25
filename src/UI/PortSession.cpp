#include "PortSession.h"
#include "../Communication/SerialPortController.h"
#include <QSettings>

PortSession::PortSession(const QString& label, QWidget* parent)
    : QObject(parent), m_label(label)
{
    // "A" keeps the original group/key names so existing single-port users keep their settings.
    QString settingsGroup = (label == "A") ? "Connection" : ("Connection_" + label);
    QString sendKey = (label == "A") ? "SendWidget" : ("SendWidget_" + label);

    m_controller = new SerialPortController(this);
    m_connectionWidget = new ConnectionWidget(settingsGroup, parent);
    m_terminalWidget = new TerminalWidget(parent);
    m_sendWidget = new SendWidget(sendKey, parent);

    connect(m_connectionWidget, &ConnectionWidget::connectRequested, this, &PortSession::onConnectRequested);
    connect(m_connectionWidget, &ConnectionWidget::disconnectRequested, this, &PortSession::onDisconnectRequested);
    connect(m_connectionWidget, &ConnectionWidget::refreshPortsRequested, this, [this](){ refreshPorts(); });

    connect(m_sendWidget, &SendWidget::sendDataRequested, this, &PortSession::onSendDataRequested);

    connect(m_controller, &ISerialTransport::dataReceived, this, &PortSession::onControllerDataReceived);
    connect(m_controller, &ISerialTransport::dataSent, this, &PortSession::onControllerDataSent);
    connect(m_controller, &ISerialTransport::connectionStateChanged, this, &PortSession::onControllerStateChanged);
    connect(m_controller, &ISerialTransport::countersUpdated, this, &PortSession::countersUpdated);
    connect(m_controller, &ISerialTransport::fileTransferProgress, this, &PortSession::fileTransferProgress);
    connect(m_controller, &ISerialTransport::fileTransferFinished, this, &PortSession::fileTransferFinished);
    connect(m_controller, &ISerialTransport::fileTransferError, this, &PortSession::fileTransferError);
}

void PortSession::refreshPorts()
{
    m_connectionWidget->setAvailablePorts(m_controller->getAvailablePorts());
}

void PortSession::performSend(const QString& text)
{
    onSendDataRequested(m_sendWidget->formatData(text));
}

void PortSession::loadSettings(QSettings& settings)
{
    m_connectionWidget->loadSettings();
    m_sendWidget->loadSettings(settings);
}

void PortSession::saveSettings(QSettings& settings)
{
    m_connectionWidget->saveSettings();
    m_sendWidget->saveSettings(settings);
}

void PortSession::onConnectRequested()
{
    emit activated();

    QString port = m_connectionWidget->portName();
    if (port.isEmpty()) return;

    int baud = m_connectionWidget->baudRate();
    QSerialPort::DataBits dataBits = static_cast<QSerialPort::DataBits>(m_connectionWidget->dataBits());
    QString stopStr = m_connectionWidget->stopBits();
    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
    if (stopStr == "1.5") stopBits = QSerialPort::OneAndHalfStop;
    else if (stopStr == "2") stopBits = QSerialPort::TwoStop;

    QString parityStr = m_connectionWidget->parity();
    QSerialPort::Parity parity = QSerialPort::NoParity;
    if (parityStr == "Even") parity = QSerialPort::EvenParity;
    else if (parityStr == "Odd") parity = QSerialPort::OddParity;
    else if (parityStr == "Space") parity = QSerialPort::SpaceParity;
    else if (parityStr == "Mark") parity = QSerialPort::MarkParity;

    QString flowStr = m_connectionWidget->flowControl();
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;
    if (flowStr == "RTS/CTS") flowControl = QSerialPort::HardwareControl;
    else if (flowStr == "XON/XOFF") flowControl = QSerialPort::SoftwareControl;

    m_controller->connectDevice(port, baud, dataBits, parity, stopBits, flowControl);
}

void PortSession::onDisconnectRequested()
{
    emit activated();
    if (m_controller->isOpen()) {
        m_controller->disconnectDevice();
    }
}

void PortSession::onSendDataRequested(const QByteArray& data)
{
    emit activated();
    if (!m_controller->isOpen() || data.isEmpty()) return;
    m_controller->writeData(data);
}

void PortSession::onControllerDataReceived(const QByteArray& data)
{
    QString formattedStr = m_terminalWidget->appendData("< RX:", data);
    emit logLine("< RX:", formattedStr);
}

void PortSession::onControllerDataSent(const QByteArray& data)
{
    QString formattedStr = m_terminalWidget->appendData("> TX:", data);
    emit logLine("> TX:", formattedStr);
}

void PortSession::onControllerStateChanged(bool isOpen, const QString& errorMsg)
{
    m_connectionWidget->setConnectedState(isOpen);
    if (isOpen) {
        m_controller->resetCounters();
    } else {
        refreshPorts();
    }
    emit stateChanged(isOpen, errorMsg);
}
