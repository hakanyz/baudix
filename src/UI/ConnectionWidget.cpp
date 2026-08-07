#include "ConnectionWidget.h"
#include <QFormLayout>
#include <QEvent>
#include <QSpacerItem>
#include <QLabel>
#include <QVBoxLayout>
#include <QStyledItemDelegate>
#include <QStylePainter>
#include <QSettings>

#include <QListView>

ConnectionWidget::ConnectionWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("dockContent");
    setupUI();
}

void ConnectionWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(5);

    // Header Layout (Always visible)
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(5, 5, 5, 5);

    m_toggleBtn = new QPushButton("▲");
    m_toggleBtn->setFixedSize(24, 24);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    m_toggleBtn->setStyleSheet(R"(
        QPushButton { border: none; font-weight: bold; color: #abb2bf; background: transparent; padding: 0px; margin: 0px; } 
        QPushButton:hover { color: #ffffff; background-color: #3b5978; border-radius: 12px; }
    )");
    connect(m_toggleBtn, &QPushButton::clicked, this, &ConnectionWidget::toggleSettings);

    m_statusLabel = new QLabel("Disconnected");
    m_statusLabel->setStyleSheet("color: #e06c75; font-weight: bold; font-size: 11px;");

    headerLayout->addWidget(m_toggleBtn);
    headerLayout->addWidget(m_statusLabel);
    headerLayout->addStretch();

    // Connect button moves to the header
    m_btnConnect = new QPushButton("Connect", this);
    m_btnConnect->setObjectName("connectBtn");
    m_btnConnect->setMinimumWidth(80);
    m_btnConnect->setFixedHeight(24);
    m_btnConnect->setStyleSheet("background-color: #3b5978; color: white; font-weight: bold; font-size: 11px; border-radius: 12px;");
    connect(m_btnConnect, &QPushButton::clicked, this, &ConnectionWidget::onConnectButtonClicked);
    headerLayout->addWidget(m_btnConnect);

    mainLayout->addLayout(headerLayout);

    // Settings Container (Collapsible)
    m_settingsContainer = new QWidget(this);
    QHBoxLayout *connLayout = new QHBoxLayout(m_settingsContainer);
    connLayout->setContentsMargins(10, 0, 10, 5);
    connLayout->setSpacing(15);
    
    // Helper lambda to add labelled widgets
    auto addLabelledWidget = [connLayout](const QString& labelText, QWidget* widget) {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);
        vLayout->setSpacing(2);
        QLabel* lbl = new QLabel(labelText);
        lbl->setStyleSheet("color: #abb2bf; font-size: 11px;");
        vLayout->addWidget(lbl);
        vLayout->addWidget(widget);
        connLayout->addLayout(vLayout);
    };

    m_portCombo = new QComboBox(this);
    m_portCombo->installEventFilter(this);
    m_portCombo->setMinimumWidth(160); // Wider to fit description
    // Increase popup font and item height
    m_portCombo->setStyleSheet("QComboBox QAbstractItemView { font-size: 13px; outline: none; } QComboBox QAbstractItemView::item { min-height: 24px; padding: 4px; }");
    addLabelledWidget("Port", m_portCombo);
    
    m_baudCombo = new QComboBox(this);
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200", "921600"});
    m_baudCombo->setCurrentText("115200");
    addLabelledWidget("Baud", m_baudCombo);
    
    m_dataBitsCombo = new QComboBox(this);
    m_dataBitsCombo->addItems({"5", "6", "7", "8"});
    m_dataBitsCombo->setCurrentText("8");
    addLabelledWidget("Data bits", m_dataBitsCombo);
    
    m_parityCombo = new QComboBox(this);
    m_parityCombo->addItems({"None", "Even", "Odd", "Space", "Mark"});
    addLabelledWidget("Parity", m_parityCombo);

    m_stopBitsCombo = new QComboBox(this);
    m_stopBitsCombo->addItems({"1", "1.5", "2"});
    addLabelledWidget("Stop bits", m_stopBitsCombo);
    
    m_flowControlCombo = new QComboBox(this);
    m_flowControlCombo->addItems({"None", "Hardware", "Software"});
    addLabelledWidget("Flow control", m_flowControlCombo);
    
    connLayout->addStretch(); // Push everything to the left
    
    mainLayout->addWidget(m_settingsContainer);
    updateStatusLabel();
}

QString ConnectionWidget::portName() const
{
    // The actual port name needed to connect is the short one or maybe the controller needs it.
    // Wait, the SerialPortController connects via name (e.g. "COM3"). The short name is sufficient.
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
        existingPorts << m_portCombo->itemData(i, Qt::UserRole).toString();
    }

    if (existingPorts != ports) {
        m_portCombo->clear();
        for (const QString& portDesc : ports) {
            m_portCombo->addItem(portDesc);
        }
        
        // Remove explicit delegate so stylesheet can take over rendering cleanly
        m_portCombo->setItemDelegate(new QStyledItemDelegate(m_portCombo));
        
        int idx = m_portCombo->findText(current);
        if (idx >= 0) {
            m_portCombo->setCurrentIndex(idx);
        }
    }
}

void ConnectionWidget::setConnectedState(bool isConnected)
{
    m_isConnected = isConnected;
    updateStatusLabel();
    
    if (isConnected) {
        m_btnConnect->setText("Disconnect");
        m_btnConnect->setStyleSheet("background-color: #8f3b43; color: white; font-weight: bold; font-size: 11px; border-radius: 12px;");
        m_portCombo->setEnabled(false);
        m_baudCombo->setEnabled(false);
        m_dataBitsCombo->setEnabled(false);
        m_stopBitsCombo->setEnabled(false);
        m_parityCombo->setEnabled(false);
        m_flowControlCombo->setEnabled(false);
        
        // Auto-collapse when connected
        m_settingsContainer->hide();
        m_toggleBtn->setText("▼");
    } else {
        m_btnConnect->setText("Connect");
        m_btnConnect->setStyleSheet("background-color: #3b5978; color: white; font-weight: bold; font-size: 11px; border-radius: 12px;");
        m_portCombo->setEnabled(true);
        m_baudCombo->setEnabled(true);
        m_dataBitsCombo->setEnabled(true);
        m_stopBitsCombo->setEnabled(true);
        m_parityCombo->setEnabled(true);
        m_flowControlCombo->setEnabled(true);
        
        // Auto-expand when disconnected
        m_settingsContainer->show();
        m_toggleBtn->setText("▲");
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

void ConnectionWidget::loadSettings()
{
    QSettings settings("hakanyz", "Baudix");
    settings.beginGroup("Connection");

    QString baud = settings.value("BaudRate", "115200").toString();
    int idx = m_baudCombo->findText(baud);
    if (idx >= 0) m_baudCombo->setCurrentIndex(idx);

    QString dataBits = settings.value("DataBits", "8").toString();
    idx = m_dataBitsCombo->findText(dataBits);
    if (idx >= 0) m_dataBitsCombo->setCurrentIndex(idx);

    QString stopBits = settings.value("StopBits", "1").toString();
    idx = m_stopBitsCombo->findText(stopBits);
    if (idx >= 0) m_stopBitsCombo->setCurrentIndex(idx);

    QString parity = settings.value("Parity", "None").toString();
    idx = m_parityCombo->findText(parity);
    if (idx >= 0) m_parityCombo->setCurrentIndex(idx);

    QString flowControl = settings.value("FlowControl", "None").toString();
    idx = m_flowControlCombo->findText(flowControl);
    if (idx >= 0) m_flowControlCombo->setCurrentIndex(idx);

    settings.endGroup();
}

void ConnectionWidget::saveSettings()
{
    QSettings settings("hakanyz", "Baudix");
    settings.beginGroup("Connection");
    settings.setValue("BaudRate", m_baudCombo->currentText());
    settings.setValue("DataBits", m_dataBitsCombo->currentText());
    settings.setValue("StopBits", m_stopBitsCombo->currentText());
    settings.setValue("Parity", m_parityCombo->currentText());
    settings.setValue("FlowControl", m_flowControlCombo->currentText());
    settings.endGroup();
}

void ConnectionWidget::toggleSettings()
{
    if (m_settingsContainer->isVisible()) {
        m_settingsContainer->hide();
        m_toggleBtn->setText("▼");
    } else {
        m_settingsContainer->show();
        m_toggleBtn->setText("▲");
    }
}

void ConnectionWidget::updateStatusLabel()
{
    if (m_isConnected) {
        // e.g. "COM3 (USB Serial) - 115200 Baud - Connected"
        m_statusLabel->setText(QString("%1 - %2 Baud - Connected").arg(m_portCombo->currentText()).arg(m_baudCombo->currentText()));
        m_statusLabel->setStyleSheet("color: #98c379; font-weight: bold; font-size: 11px;"); // Green
    } else {
        m_statusLabel->setText("Disconnected");
        m_statusLabel->setStyleSheet("color: #e06c75; font-weight: bold; font-size: 11px;"); // Red
    }
}
