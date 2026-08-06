#include "ConnectionWidget.h"
#include <QFormLayout>
#include <QEvent>
#include <QSpacerItem>
#include <QLabel>
#include <QVBoxLayout>
#include <QStyledItemDelegate>
#include <QStylePainter>
#include <QSettings>

class PortComboBoxDelegate : public QStyledItemDelegate {
public:
    explicit PortComboBoxDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    
    // We will use an editable combo box trick instead of drawing manually,
    // so we don't strictly need this delegate for drawing the closed state,
    // but we can use it to draw the popup if needed. Actually, standard item delegate is fine.
};

class ShortTextComboBox : public QComboBox {
public:
    explicit ShortTextComboBox(QWidget* parent = nullptr) : QComboBox(parent) {}
protected:
    void paintEvent(QPaintEvent* event) override {
        QStylePainter painter(this);
        painter.setPen(palette().color(QPalette::Text));
        QStyleOptionComboBox opt;
        initStyleOption(&opt);
        
        // Override the displayed text to be just the short name
        QString fullText = opt.currentText;
        opt.currentText = fullText.split(" - ").first();
        
        painter.drawComplexControl(QStyle::CC_ComboBox, opt);
        painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
    }
};

class PortPopupDelegate : public QStyledItemDelegate {
public:
    explicit PortPopupDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QStyleOptionViewItem opt = option;
        // Force background and text colors to bypass Windows native styling
        if (opt.state & QStyle::State_Selected) {
            painter->fillRect(opt.rect, QColor("#282c34")); // Dark background for selection
            opt.palette.setColor(QPalette::Text, QColor("#61afef")); // Blue text
            opt.palette.setColor(QPalette::HighlightedText, QColor("#61afef")); // Blue text
        } else {
            painter->fillRect(opt.rect, QColor("#21252b")); // Normal background
            opt.palette.setColor(QPalette::Text, QColor("#abb2bf")); // Gray text
            opt.palette.setColor(QPalette::HighlightedText, QColor("#abb2bf")); // Gray text
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

ConnectionWidget::ConnectionWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("dockContent");
    setupUI();
}

void ConnectionWidget::setupUI()
{
    QHBoxLayout *connLayout = new QHBoxLayout(this);
    connLayout->setContentsMargins(10, 5, 10, 5);
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

    m_portCombo = new ShortTextComboBox(this);
    m_portCombo->installEventFilter(this);
    m_portCombo->setMinimumWidth(100);
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
    
    // We will add the stretch AFTER the connect button

    m_btnConnect = new QPushButton("Connect", this);
    m_btnConnect->setObjectName("connectBtn");
    m_btnConnect->setMinimumWidth(90);
    m_btnConnect->setFixedHeight(26);
    m_btnConnect->setStyleSheet("background-color: #3b5978; color: white; font-weight: bold; border-radius: 13px;");
    connect(m_btnConnect, &QPushButton::clicked, this, &ConnectionWidget::onConnectButtonClicked);
    
    // Add a wrapper to perfectly align the button vertically with the combo boxes
    QVBoxLayout* btnLayout = new QVBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(2);
    QLabel* dummyLbl = new QLabel(" ");
    dummyLbl->setStyleSheet("font-size: 11px;");
    btnLayout->addWidget(dummyLbl);
    btnLayout->addWidget(m_btnConnect);
    connLayout->addLayout(btnLayout);
    
    connLayout->addStretch(); // Push everything to the left
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
        m_portCombo->setItemDelegate(new PortPopupDelegate(m_portCombo)); // Force the custom delegate
        
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
        m_btnConnect->setStyleSheet("background-color: #8f3b43; color: white; font-weight: bold; border-radius: 13px;");
        m_portCombo->setEnabled(false);
        m_baudCombo->setEnabled(false);
        m_dataBitsCombo->setEnabled(false);
        m_stopBitsCombo->setEnabled(false);
        m_parityCombo->setEnabled(false);
        m_flowControlCombo->setEnabled(false);
    } else {
        m_btnConnect->setText("Connect");
        m_btnConnect->setStyleSheet("background-color: #3b5978; color: white; font-weight: bold; border-radius: 13px;");
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
