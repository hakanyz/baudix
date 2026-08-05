#include "ConnectionWidget.h"
#include <QFormLayout>
#include <QEvent>
#include <QSpacerItem>

ConnectionWidget::ConnectionWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("dockContent");
    setupUI();
}

void ConnectionWidget::setupUI()
{
    QFormLayout *connLayout = new QFormLayout(this);
    
    m_portCombo = new QComboBox(this);
    m_portCombo->installEventFilter(this);
    connLayout->addRow("COM Port", m_portCombo);
    
    m_baudCombo = new QComboBox(this);
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200", "921600"});
    m_baudCombo->setCurrentText("115200");
    connLayout->addRow("Baud Rate", m_baudCombo);
    
    m_dataBitsCombo = new QComboBox(this);
    m_dataBitsCombo->addItems({"5", "6", "7", "8"});
    m_dataBitsCombo->setCurrentText("8");
    connLayout->addRow("Data Bits", m_dataBitsCombo);
    
    m_stopBitsCombo = new QComboBox(this);
    m_stopBitsCombo->addItems({"1", "1.5", "2"});
    connLayout->addRow("Stop Bits", m_stopBitsCombo);
    
    m_parityCombo = new QComboBox(this);
    m_parityCombo->addItems({"None", "Even", "Odd", "Space", "Mark"});
    connLayout->addRow("Parity", m_parityCombo);
    
    m_flowControlCombo = new QComboBox(this);
    m_flowControlCombo->addItems({"None", "Hardware", "Software"});
    connLayout->addRow("Flow Control", m_flowControlCombo);
    
    connLayout->addItem(new QSpacerItem(0, 5, QSizePolicy::Minimum, QSizePolicy::Fixed));

    m_btnConnect = new QPushButton("Connect", this);
    m_btnConnect->setObjectName("connectBtn");
    m_btnConnect->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_btnConnect->setFixedHeight(26);
    connect(m_btnConnect, &QPushButton::clicked, this, &ConnectionWidget::onConnectButtonClicked);
    connLayout->addRow(m_btnConnect);
}

QString ConnectionWidget::portName() const
{
    return m_portCombo->currentText();
}

int ConnectionWidget::baudRate() const
{
    return m_baudCombo->currentText().toInt();
}

int ConnectionWidget::dataBits() const
{
    return m_dataBitsCombo->currentText().toInt();
}

QString ConnectionWidget::stopBits() const
{
    return m_stopBitsCombo->currentText();
}

QString ConnectionWidget::parity() const
{
    return m_parityCombo->currentText();
}

QString ConnectionWidget::flowControl() const
{
    return m_flowControlCombo->currentText();
}

void ConnectionWidget::setAvailablePorts(const QStringList& ports)
{
    QString current = m_portCombo->currentText();
    QStringList existingPorts;
    for (int i = 0; i < m_portCombo->count(); ++i) {
        existingPorts << m_portCombo->itemText(i);
    }

    if (existingPorts != ports) {
        m_portCombo->clear();
        m_portCombo->addItems(ports);
        int idx = m_portCombo->findText(current);
        if (idx >= 0) {
            m_portCombo->setCurrentIndex(idx);
        }
    }
}

void ConnectionWidget::setConnectedState(bool isConnected)
{
    m_isConnected = isConnected;
    if (isConnected) {
        m_btnConnect->setText("Disconnect");
        m_btnConnect->setStyleSheet("background-color: #e06c75; color: white; font-weight: bold; border-radius: 4px;");
        m_portCombo->setEnabled(false);
        m_baudCombo->setEnabled(false);
        m_dataBitsCombo->setEnabled(false);
        m_stopBitsCombo->setEnabled(false);
        m_parityCombo->setEnabled(false);
        m_flowControlCombo->setEnabled(false);
    } else {
        m_btnConnect->setText("Connect");
        m_btnConnect->setStyleSheet(""); // reset to default qss
        m_portCombo->setEnabled(true);
        m_baudCombo->setEnabled(true);
        m_dataBitsCombo->setEnabled(true);
        m_stopBitsCombo->setEnabled(true);
        m_parityCombo->setEnabled(true);
        m_flowControlCombo->setEnabled(true);
    }
}

void ConnectionWidget::onConnectButtonClicked()
{
    if (m_isConnected) {
        emit disconnectRequested();
    } else {
        emit connectRequested();
    }
}

bool ConnectionWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_portCombo && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick)) {
        emit refreshPortsRequested();
    }
    return QWidget::eventFilter(watched, event);
}
