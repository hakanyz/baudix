#include "SendWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QStyle>
#include <QLineEdit>
#include <QLineEdit>
#include <QSplitter>
#include <QSplitterHandle>
#include <QRegularExpression>
#include <QSettings>
#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QToolButton>
#include <QPushButton>
#include <QTimer>

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
    sendLayout->setContentsMargins(0, 0, 0, 0); 
    sendLayout->setSpacing(0);
    
    m_internalSplitter = new QSplitter(Qt::Horizontal, sendFrame);
    m_internalSplitter->setHandleWidth(4); // Same as main splitter
    m_internalSplitter->setStyleSheet("QSplitter::handle { background: transparent; }");
    
    // Left pane should not stretch automatically, right pane should take extra space
    m_internalSplitter->setStretchFactor(0, 0);
    m_internalSplitter->setStretchFactor(1, 1);
    
    sendLayout->addWidget(m_internalSplitter);
    
    // LEFT PANE
    QWidget* leftWidget = new QWidget();
    QHBoxLayout* leftLayout = new QHBoxLayout(leftWidget);
    leftLayout->setContentsMargins(10, 5, 2, 5); // 2px right margin perfectly aligns textbox right edge with Terminal's 2px padding
    leftLayout->setSpacing(15);
    
    // Raw Command (Input)
    m_inputCombo = new QComboBox();
    m_inputCombo->setEditable(true);
    m_inputCombo->setInsertPolicy(QComboBox::NoInsert);
    m_inputCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_inputCombo->lineEdit()->setPlaceholderText("Type text or HEX bytes...");
    m_inputCombo->addItem("");
    connect(m_inputCombo->lineEdit(), &QLineEdit::returnPressed, this, &SendWidget::onSendClicked);
    leftLayout->addWidget(m_inputCombo, 1);
    
    m_internalSplitter->addWidget(leftWidget);
    
    // RIGHT PANE
    QWidget* rightWidget = new QWidget();
    QHBoxLayout* rightLayout = new QHBoxLayout(rightWidget);
    rightLayout->setContentsMargins(4, 5, 1, 5); // Left 4 matches LoggingWidget's 4. Right 1 matches the 1px difference at the window edge.
    rightLayout->setSpacing(5);
    
    // History in Popup
    m_settingsPopup = new QDialog(this, Qt::Popup | Qt::FramelessWindowHint);
    m_settingsPopup->setObjectName("settingsPopup");
    m_settingsPopup->setStyleSheet("#settingsPopup { background-color: #282c34; border: 1px solid #181a1f; border-radius: 4px; } QLabel { color: #abb2bf; }");
    QVBoxLayout* popupLayout = new QVBoxLayout(m_settingsPopup);
    popupLayout->setSpacing(4);
    popupLayout->setContentsMargins(6, 6, 6, 6);
    
    // Format moved to Popup
    QHBoxLayout* formatLayout = new QHBoxLayout();
    m_sendAsCombo = new QComboBox();
    m_sendAsCombo->addItems({"ASCII", "HEX"});
    formatLayout->addWidget(new QLabel("Format:"));
    formatLayout->addWidget(m_sendAsCombo);
    formatLayout->addStretch();
    popupLayout->addLayout(formatLayout);
    
    m_cbHistoryOn = new QCheckBox("Save History");
    m_cbHistoryOn->setChecked(true);
    m_cbHistoryOn->setToolTip("Save sent commands to history. Uncheck to clear.");
    connect(m_cbHistoryOn, &QCheckBox::toggled, this, &SendWidget::onHistoryToggled);
    popupLayout->addWidget(m_cbHistoryOn);

    // Auto-send in Popup
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
    pLayout->addStretch();
    popupLayout->addWidget(periodicWidget);
    
    // Burst in Popup
    QHBoxLayout* burstLayout = new QHBoxLayout();
    m_burstBox = new QSpinBox();
    m_burstBox->setRange(1, 1000);
    m_burstBox->setValue(1);
    m_burstBox->setFixedWidth(80);
    burstLayout->addWidget(new QLabel("Burst:"));
    burstLayout->addWidget(m_burstBox);
    burstLayout->addStretch();
    popupLayout->addLayout(burstLayout);

    // Line-ending in Popup
    QHBoxLayout* appendLayout = new QHBoxLayout();
    m_appendCombo = new QComboBox();
    m_appendCombo->addItems({"None", "CR", "LF", "CRLF"});
    appendLayout->addWidget(new QLabel("Line-ending:"));
    appendLayout->addWidget(m_appendCombo);
    appendLayout->addStretch();
    popupLayout->addLayout(appendLayout);
    
    // Settings Button for main bar
    QToolButton* settingsBtn = new QToolButton(this);
    settingsBtn->setText("⚙️");
    settingsBtn->setToolTip("Transmission Settings");
    settingsBtn->setFixedHeight(28);
    settingsBtn->setFixedWidth(36); // Slightly wider to prevent "..." truncation of the emoji
    settingsBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    settingsBtn->setStyleSheet("QToolButton { font-size: 16px; background-color: transparent; border: 1px solid #181a1f; border-radius: 4px; padding: 0 5px; } QToolButton:hover { background-color: #3b4048; }");
    connect(settingsBtn, &QToolButton::clicked, this, [this, settingsBtn](){
        QPoint pos = settingsBtn->mapToGlobal(QPoint(0, settingsBtn->height() + 2));
        m_settingsPopup->move(pos);
        m_settingsPopup->show();
    });
    // Send Button
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setObjectName("sendButton");
    m_sendButton->setFixedHeight(28);
    m_sendButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_sendButton, &QPushButton::clicked, this, &SendWidget::onSendClicked);
    rightLayout->addWidget(m_sendButton);
    
    // Add Settings button after Send button so it sits on the far right
    rightLayout->addWidget(settingsBtn);
    
    m_internalSplitter->addWidget(rightWidget);
    
    // Prevent internal splitter from being dragged by user, it strictly follows the main splitter
    for (int i = 0; i < m_internalSplitter->count(); ++i) {
        QSplitterHandle *handle = m_internalSplitter->handle(i);
        if (handle) {
            handle->setAttribute(Qt::WA_TransparentForMouseEvents);
        }
    }
    
    mainLayout->addWidget(sendFrame);
}

void SendWidget::syncSplitterSizes(int mainLeftSize)
{
    if (m_internalSplitter) {
        // Use singleShot to wait for the layout to finish updating (crucial for window maximize/restore)
        QTimer::singleShot(0, m_internalSplitter, [this, mainLeftSize]() {
            // The main splitter starts at X=2. This internal splitter is inside a cardFrame which starts at X=2,
            // and has a 2px padding + 1px border. So this internal splitter starts at absolute X=5.
            // To align their handles perfectly, this internal splitter's left size must be 3px smaller!
            int leftSize = mainLeftSize - 3;
            if (leftSize < 0) leftSize = 0;
            
            int rightSize = m_internalSplitter->width() - leftSize - m_internalSplitter->handleWidth();
            if (rightSize < 0) rightSize = 0;
            
            m_internalSplitter->setSizes({leftSize, rightSize}); 
        });
    }
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
    // If repeat is active, the send button acts as a cancel button
    if (m_periodicTimer->isActive()) {
        m_periodicTimer->stop();
        m_sendButton->setText("Send");
        m_sendButton->setStyleSheet("");
        return;
    }

    QString text = m_inputCombo->currentText();
    if (text.isEmpty()) return;

    QByteArray data = formatData(text);
    if (!data.isEmpty()) {
        emit sendDataRequested(data);
    }
    
    if (m_periodicSendCb->isChecked()) {
        m_periodicText = text;
        m_periodicTimer->start(m_periodicMsBox->value());
        m_sendButton->setText("Stop"); // Just "Stop" so it doesn't stretch
        m_sendButton->setStyleSheet("background-color: #d15656; color: white; font-weight: bold; border-color: #b03a3a;");
    } else {
        m_inputCombo->setEditText("");
    }
    
    if (m_cbHistoryOn->isChecked()) {
        if (m_inputCombo->findText(text) == -1) {
            m_inputCombo->insertItem(1, text);
        }
    }
}

void SendWidget::onPeriodicSendToggled(bool checked)
{
    if (!checked && m_periodicTimer->isActive()) {
        m_periodicTimer->stop();
        m_sendButton->setText("Send");
        m_sendButton->setStyleSheet("");
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

void SendWidget::loadSettings(QSettings& settings)
{
    bool historyOn = settings.value("SendWidget/HistoryOn", true).toBool();
    m_cbHistoryOn->setChecked(historyOn);
}

void SendWidget::saveSettings(QSettings& settings)
{
    settings.setValue("SendWidget/HistoryOn", m_cbHistoryOn->isChecked());
}
