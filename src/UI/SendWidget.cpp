#include "SendWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QStyle>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSettings>

SendWidget::SendWidget(QWidget *parent)
    : QWidget(parent)
{
    m_periodicTimer = new QTimer(this);
    setupUI();
}

void SendWidget::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *sendFrame = new QFrame(this);
    sendFrame->setObjectName("dockContent");
    sendFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); 
    
    QHBoxLayout *sendLayout = new QHBoxLayout(sendFrame);
    sendLayout->setContentsMargins(10, 5, 10, 5);
    sendLayout->setSpacing(15);
    
    auto addLabelledWidget = [sendLayout](const QString& labelText, QWidget* widget) {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);
        vLayout->setSpacing(2);
        if (!labelText.isEmpty()) {
            QLabel* lbl = new QLabel(labelText);
            lbl->setStyleSheet("color: #abb2bf; font-size: 11px;");
            vLayout->addWidget(lbl);
        }
        vLayout->addWidget(widget);
        sendLayout->addLayout(vLayout);
    };

    // Format
    m_sendAsCombo = new QComboBox();
    m_sendAsCombo->addItems({"ASCII", "HEX"});
    addLabelledWidget("Format", m_sendAsCombo);

    // Raw Command (Input)
    m_inputCombo = new QComboBox();
    m_inputCombo->setEditable(true);
    m_inputCombo->setInsertPolicy(QComboBox::NoInsert);
    m_inputCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_inputCombo->lineEdit()->setPlaceholderText("Type text or HEX bytes...");
    m_inputCombo->addItem("");
    connect(m_inputCombo->lineEdit(), &QLineEdit::returnPressed, this, &SendWidget::onSendClicked);
    
    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(0,0,0,0);
    inputLayout->setSpacing(5);
    inputLayout->addWidget(m_inputCombo, 1);
    
    m_cbHistoryOn = new QCheckBox("History");
    m_cbHistoryOn->setChecked(true);
    m_cbHistoryOn->setToolTip("Save sent commands to history. Uncheck to clear.");
    connect(m_cbHistoryOn, &QCheckBox::toggled, this, &SendWidget::onHistoryToggled);
    inputLayout->addWidget(m_cbHistoryOn);
    
    QWidget* inputContainer = new QWidget();
    inputContainer->setLayout(inputLayout);
    addLabelledWidget("Raw command", inputContainer);

    // Periodic wrapper
    QWidget* periodicWidget = new QWidget();
    QHBoxLayout* pLayout = new QHBoxLayout(periodicWidget);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(5);
    
    m_periodicSendCb = new QCheckBox("Repeat");
    connect(m_periodicSendCb, &QCheckBox::toggled, this, &SendWidget::onPeriodicSendToggled);
    connect(m_periodicTimer, &QTimer::timeout, this, &SendWidget::onPeriodicTimerTimeout);
    
    m_periodicMsBox = new QSpinBox();
    m_periodicMsBox->setRange(1, 100000);
    m_periodicMsBox->setValue(100);
    m_periodicMsBox->setSuffix(" ms");
    m_periodicMsBox->setFixedWidth(80);
    
    pLayout->addWidget(m_periodicSendCb);
    pLayout->addWidget(m_periodicMsBox);
    addLabelledWidget("Auto-send", periodicWidget);
    
    m_burstBox = new QSpinBox(); // Keep this hidden or add it? Let's add it cleanly
    m_burstBox->setRange(1, 1000);
    m_burstBox->setValue(1);
    m_burstBox->setFixedWidth(60);
    addLabelledWidget("Burst", m_burstBox);

    // Append
    m_appendCombo = new QComboBox();
    m_appendCombo->addItems({"None", "CR", "LF", "CRLF"});
    addLabelledWidget("Line-ending", m_appendCombo);

    // Send Button
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setObjectName("sendButton");
    m_sendButton->setMinimumWidth(80);
    m_sendButton->setFixedHeight(28);
    connect(m_sendButton, &QPushButton::clicked, this, &SendWidget::onSendClicked);
    
    // Send File Button
    QPushButton* sendFileBtn = new QPushButton("📄 File...", this);
    sendFileBtn->setObjectName("smallBtn");
    sendFileBtn->setToolTip("Send a file over the serial port");
    sendFileBtn->setFixedHeight(28);
    connect(sendFileBtn, &QPushButton::clicked, this, &SendWidget::onSendFileClicked);
    
    QHBoxLayout* btnHLayout = new QHBoxLayout();
    btnHLayout->setContentsMargins(0, 0, 0, 0);
    btnHLayout->setSpacing(5);
    btnHLayout->addWidget(sendFileBtn);
    btnHLayout->addWidget(m_sendButton);
    
    QVBoxLayout* btnLayout = new QVBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->addStretch();
    btnLayout->addLayout(btnHLayout);
    btnLayout->addStretch();
    sendLayout->addLayout(btnLayout);

    mainLayout->addWidget(sendFrame);
}

void SendWidget::setInputText(const QString& text)
{
    m_inputCombo->setEditText(text);
}

QByteArray SendWidget::formatData(const QString& text) const
{
    QByteArray data;
    if (text.isEmpty()) return data;

    bool isHex = (m_sendAsCombo->currentText() == "HEX");

    if (isHex) {
        QString cleanText = text;
        cleanText.replace("0x", "", Qt::CaseInsensitive);
        cleanText.remove(QRegularExpression("[^0-9a-fA-F]"));
        
        if (cleanText.length() % 2 != 0) cleanText.prepend("0");
        
        for (int i = 0; i < cleanText.length(); i += 2) {
            bool ok;
            uint byteVal = cleanText.mid(i, 2).toUInt(&ok, 16);
            if (ok) data.append((char)byteVal);
        }
    } else {
        data = text.toUtf8();
    }
    
    QString appendMode = m_appendCombo->currentText();
    if (appendMode == "CR") data.append('\r');
    else if (appendMode == "LF") data.append('\n');
    else if (appendMode == "CRLF") { data.append('\r'); data.append('\n'); }
    
    return data;
}

void SendWidget::onSendClicked()
{
    QString text = m_inputCombo->currentText();
    if (text.isEmpty()) return;

    QByteArray data = formatData(text);
    if (!data.isEmpty()) {
        emit sendDataRequested(data);
    }
    
    if (m_periodicSendCb->isChecked()) {
        m_periodicText = text;
        m_periodicTimer->start(m_periodicMsBox->value());
    }
    
    m_inputCombo->setEditText("");
    
    if (m_cbHistoryOn->isChecked()) {
        if (m_inputCombo->findText(text) == -1) {
            m_inputCombo->insertItem(1, text);
        }
    }
}

void SendWidget::onPeriodicSendToggled(bool checked)
{
    if (!checked) {
        m_periodicTimer->stop();
    }
}

void SendWidget::onPeriodicTimerTimeout()
{
    int bursts = m_burstBox->value();
    for(int i=0; i<bursts; i++) {
        if (!m_periodicText.isEmpty()) {
            QByteArray data = formatData(m_periodicText);
            if (!data.isEmpty()) {
                emit sendDataRequested(data);
            }
        }
    }
}

void SendWidget::onHistoryToggled(bool checked)
{
    if (!checked) {
        m_inputCombo->setStyleSheet("QComboBox::drop-down { border: none; width: 0px; }");
        m_inputCombo->clear();
        m_inputCombo->addItem("");
    } else {
        m_inputCombo->setStyleSheet("");
    }
}

void SendWidget::onClearHistoryClicked()
{
    m_inputCombo->clear();
    m_inputCombo->addItem("");
}

void SendWidget::onSendFileClicked()
{
    emit sendFileRequested();
}

void SendWidget::loadSettings(QSettings& settings)
{
    bool historyOn = settings.value("SendWidget/HistoryOn", true).toBool();
    m_cbHistoryOn->setChecked(historyOn);
    
    if (historyOn) {
        QStringList history = settings.value("SendWidget/History").toStringList();
        m_inputCombo->clear();
        m_inputCombo->addItem(""); // always keep an empty item
        for (const QString& item : history) {
            if (!item.isEmpty()) {
                m_inputCombo->addItem(item);
            }
        }
    }
}

void SendWidget::saveSettings(QSettings& settings)
{
    settings.setValue("SendWidget/HistoryOn", m_cbHistoryOn->isChecked());
    
    QStringList history;
    // Limit history to 100 items to avoid bloating config
    int count = qMin(m_inputCombo->count(), 100);
    for (int i = 0; i < count; ++i) {
        QString item = m_inputCombo->itemText(i);
        if (!item.isEmpty()) {
            history.append(item);
        }
    }
    settings.setValue("SendWidget/History", history);
}
