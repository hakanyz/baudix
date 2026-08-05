#include "SendWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QStyle>
#include <QLineEdit>
#include <QRegularExpression>

SendWidget::SendWidget(QWidget *parent)
    : QWidget(parent)
{
    m_periodicTimer = new QTimer(this);
    setupUI();
}

void SendWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *sendFrame = new QFrame(this);
    sendFrame->setObjectName("dockContent");
    sendFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum); // Hugs contents vertically
    QVBoxLayout *sendLayout = new QVBoxLayout(sendFrame);
    sendLayout->setContentsMargins(4, 4, 4, 4);
    sendLayout->setSpacing(4);
    
    // Top Row: Input only
    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_inputCombo = new QComboBox();
    m_inputCombo->setEditable(true);
    m_inputCombo->setInsertPolicy(QComboBox::NoInsert);
    m_inputCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_inputCombo->lineEdit()->setPlaceholderText("Type text or HEX bytes (e.g. AA BB CC)");
    m_inputCombo->addItem(""); // Default empty
    connect(m_inputCombo->lineEdit(), &QLineEdit::returnPressed, this, &SendWidget::onSendClicked);
    inputLayout->addWidget(m_inputCombo, 1);
    sendLayout->addLayout(inputLayout);
    
    // Middle Row: Action Tools
    QHBoxLayout *sendActionLayout = new QHBoxLayout();
    
    int actionHeight = 28;
    
    QPushButton* btnSendFile = new QPushButton("Send File");
    btnSendFile->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    btnSendFile->setObjectName("smallBtn");
    btnSendFile->setFixedHeight(actionHeight);
    connect(btnSendFile, &QPushButton::clicked, this, &SendWidget::onSendFileClicked);
    sendActionLayout->addWidget(btnSendFile);
    
    m_cbHistoryOn = new QCheckBox("Save History");
    m_cbHistoryOn->setChecked(true);
    m_cbHistoryOn->setFixedHeight(actionHeight);
    connect(m_cbHistoryOn, &QCheckBox::toggled, this, &SendWidget::onHistoryToggled);
    sendActionLayout->addWidget(m_cbHistoryOn);
    
    QPushButton* btnClearHistory = new QPushButton("Clear History");
    btnClearHistory->setObjectName("smallBtn");
    btnClearHistory->setFixedHeight(actionHeight);
    connect(btnClearHistory, &QPushButton::clicked, this, &SendWidget::onClearHistoryClicked);
    sendActionLayout->addWidget(btnClearHistory);
    
    QLabel* lblAppend = new QLabel("Append:");
    lblAppend->setFixedHeight(actionHeight);
    sendActionLayout->addWidget(lblAppend);
    
    m_appendCombo = new QComboBox();
    m_appendCombo->addItems({"None", "CR", "LF", "CRLF"});
    m_appendCombo->setFixedHeight(actionHeight);
    sendActionLayout->addWidget(m_appendCombo);
    
    QLabel* lblFormat = new QLabel("Format:");
    lblFormat->setFixedHeight(actionHeight);
    sendActionLayout->addWidget(lblFormat);
    
    m_sendAsCombo = new QComboBox();
    m_sendAsCombo->addItems({"ASCII", "HEX"});
    m_sendAsCombo->setToolTip("ASCII: send as plain text\nHEX: parse as hex bytes");
    m_sendAsCombo->setFixedHeight(actionHeight);
    sendActionLayout->addWidget(m_sendAsCombo);
    
    sendActionLayout->addStretch();
    
    m_sendButton = new QPushButton("Send");
    m_sendButton->setObjectName("sendButton");
    m_sendButton->setFixedHeight(actionHeight);
    connect(m_sendButton, &QPushButton::clicked, this, &SendWidget::onSendClicked);
    sendActionLayout->addWidget(m_sendButton);
    
    sendLayout->addLayout(sendActionLayout);
    
    // Bottom Row: Periodic Send
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    
    m_periodicSendCb = new QCheckBox("Periodic Send");
    m_periodicSendCb->setToolTip("Automatically send the input at a fixed interval");
    connect(m_periodicSendCb, &QCheckBox::toggled, this, &SendWidget::onPeriodicSendToggled);
    connect(m_periodicTimer, &QTimer::timeout, this, &SendWidget::onPeriodicTimerTimeout);
    bottomLayout->addWidget(m_periodicSendCb);
    
    m_periodicMsBox = new QSpinBox();
    m_periodicMsBox->setRange(1, 100000);
    m_periodicMsBox->setValue(100);
    m_periodicMsBox->setSuffix(" ms");
    m_periodicMsBox->setFixedWidth(80);
    m_periodicMsBox->setToolTip("Interval between sends");
    bottomLayout->addWidget(m_periodicMsBox);
    
    bottomLayout->addWidget(new QLabel("Burst:"));
    m_burstBox = new QSpinBox();
    m_burstBox->setRange(1, 1000);
    m_burstBox->setValue(1);
    m_burstBox->setFixedWidth(60);
    m_burstBox->setToolTip("How many times to send per interval");
    bottomLayout->addWidget(m_burstBox);
    
    bottomLayout->addStretch();
    sendLayout->addLayout(bottomLayout);
    
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
