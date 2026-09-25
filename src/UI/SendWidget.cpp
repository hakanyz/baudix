#include "SendWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QStyle>
#include <QRegularExpression>
#include <QSettings>
#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QToolButton>
#include <QPushButton>
#include <QTimer>
#include <QScreen>
#include <QMenu>
#include <QKeyEvent>
#include <QFontMetrics>
#include <QTextDocument>
#include <QTextCursor>
#include <QScrollBar>

SendWidget::SendWidget(const QString& settingsKey, QWidget *parent)
    : QWidget(parent), m_settingsKey(settingsKey)
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
    sendLayout->setContentsMargins(10, 5, 1, 5);
    sendLayout->setSpacing(15);

    // History dropdown (recent sent commands)
    m_historyBtn = new QToolButton();
    m_historyBtn->setText("\U0001F553"); // 🕓
    m_historyBtn->setToolTip("History");
    m_historyBtn->setFixedSize(28, 28);
    m_historyBtn->setPopupMode(QToolButton::InstantPopup);
    m_historyBtn->setStyleSheet("QToolButton { font-size: 13px; background-color: transparent; border: 1px solid #181a1f; border-radius: 4px; } QToolButton::menu-indicator { image: none; } QToolButton:hover { background-color: #3b4048; }");
    m_historyBtn->setMenu(new QMenu(m_historyBtn));
    sendLayout->addWidget(m_historyBtn);
    rebuildHistoryMenu();

    // Raw Command (Input) - multi-line, grows with content (e.g. pasted JSON or HEX dumps)
    m_inputEdit = new QPlainTextEdit();
    m_inputEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_inputEdit->setTabChangesFocus(true);
    m_inputEdit->installEventFilter(this);
    connect(m_inputEdit, &QPlainTextEdit::textChanged, this, &SendWidget::adjustInputHeight);
    sendLayout->addWidget(m_inputEdit, 1);
    adjustInputHeight();

    // RIGHT PANE (Send + Settings buttons; compact, never stretches)
    QWidget* rightWidget = new QWidget();
    QHBoxLayout* rightLayout = new QHBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
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
    connect(m_sendAsCombo, &QComboBox::currentTextChanged, this, &SendWidget::updatePlaceholder);
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
        // Reset any width clamp from a previous open so the natural size is measured fresh.
        m_settingsPopup->setMaximumWidth(QWIDGETSIZE_MAX);
        m_settingsPopup->adjustSize();

        const QRect btnRect(settingsBtn->mapToGlobal(QPoint(0, 0)), settingsBtn->size());
        const QScreen* screen = settingsBtn->screen();
        const QRect screenRect = screen ? screen->availableGeometry() : QRect(QPoint(0, 0), m_settingsPopup->sizeHint());
        // Align to THIS SendWidget's own right edge, not the top-level window's - in Dual Mode
        // there are two SendWidgets side by side, each narrower than the window.
        const int widgetRightEdge = qMin(this->mapToGlobal(QPoint(this->width(), 0)).x(), screenRect.right());

        // Never let the popup cross this SendWidget's right edge; shrink it instead of just moving it.
        const int maxWidth = widgetRightEdge - btnRect.left();
        if (m_settingsPopup->sizeHint().width() > maxWidth) {
            m_settingsPopup->setMaximumWidth(qMax(maxWidth, m_settingsPopup->minimumSizeHint().width()));
            m_settingsPopup->adjustSize();
        }
        const QSize popupSize = m_settingsPopup->size();

        int x = widgetRightEdge - popupSize.width();
        if (x < screenRect.left())
            x = screenRect.left();

        // Prefer opening below the button; flip above it if there's no room.
        int y = btnRect.bottom() + 2;
        if (y + popupSize.height() > screenRect.bottom())
            y = btnRect.top() - popupSize.height() - 2;

        m_settingsPopup->move(x, y);
        m_settingsPopup->show();
    });
    // Send Button (fixed, compact width - it no longer needs to fill a splitter pane)
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setObjectName("sendButton");
    m_sendButton->setFixedHeight(28);
    m_sendButton->setFixedWidth(90);
    m_sendButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_sendButton, &QPushButton::clicked, this, &SendWidget::onSendClicked);
    rightLayout->addWidget(m_sendButton);

    // Add Settings button after Send button so it sits on the far right
    rightLayout->addWidget(settingsBtn);

    sendLayout->addWidget(rightWidget);

    mainLayout->addWidget(sendFrame);

    updatePlaceholder();
}

void SendWidget::setInputText(const QString& text)
{
    m_inputEdit->setPlainText(text);
    QTextCursor cursor = m_inputEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_inputEdit->setTextCursor(cursor);
}

QByteArray SendWidget::formatData(const QString& text) const
{
    QByteArray data;
    if (text.isEmpty()) return data;

    bool isHex = (m_sendAsCombo->currentText() == "HEX");

    if (isHex) {
        // Pad each whitespace/comma-separated token to a whole byte on its own, instead of
        // concatenating everything first and padding once at the front - otherwise a single
        // odd-length token (e.g. one un-padded digit like "A" for 0x0A) shifts every byte
        // boundary after it, corrupting the rest of the payload.
        QString combined;
        const QStringList tokens = text.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
        for (QString token : tokens) {
            if (token.startsWith("0x", Qt::CaseInsensitive)) token.remove(0, 2);
            token.remove(QRegularExpression("[^0-9a-fA-F]"));
            if (token.isEmpty()) continue;
            if (token.length() % 2 != 0) token.prepend('0');
            combined += token;
        }

        for (int i = 0; i < combined.length(); i += 2) {
            bool ok;
            uint byteVal = combined.mid(i, 2).toUInt(&ok, 16);
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

    QString text = m_inputEdit->toPlainText();
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
        m_inputEdit->clear();
    }

    if (m_cbHistoryOn->isChecked()) {
        if (!m_history.contains(text)) {
            m_history.prepend(text);
            rebuildHistoryMenu();
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
    m_historyBtn->setEnabled(checked);
    if (!checked) {
        m_history.clear();
        rebuildHistoryMenu();
    }
}

void SendWidget::updatePlaceholder()
{
    if (m_sendAsCombo->currentText() == "HEX") {
        m_inputEdit->setPlaceholderText("Hex bytes, e.g. 01 02 FF or 0x01 0x02 0xFF... (Shift+Enter for new line)");
    } else {
        m_inputEdit->setPlaceholderText("Type text to send... (Shift+Enter for new line)");
    }
}

void SendWidget::adjustInputHeight()
{
    const int maxVisibleLines = 6;
    QFontMetrics fm(m_inputEdit->font());
    int lineHeight = fm.lineSpacing();
    int frame = m_inputEdit->frameWidth() * 2;
    int padding = 10; // breathing room around the document's own top/bottom margins
    int docHeight = static_cast<int>(m_inputEdit->document()->size().height());

    int singleLineHeight = lineHeight + frame + padding;
    int maxHeight = lineHeight * maxVisibleLines + frame + padding;
    int target = qBound(singleLineHeight, docHeight + frame + padding, maxHeight);

    m_inputEdit->setFixedHeight(target);
    m_inputEdit->setVerticalScrollBarPolicy(target >= maxHeight ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
}

void SendWidget::rebuildHistoryMenu()
{
    // Hidden until there's actually something to show - an always-visible button for an
    // empty history just invites confused clicks.
    m_historyBtn->setVisible(!m_history.isEmpty());

    QMenu* menu = m_historyBtn->menu();
    menu->clear();
    if (m_history.isEmpty()) {
        return;
    }
    for (const QString& entry : m_history) {
        // Keep menu entries readable: collapse to one line and cap the length.
        QString label = entry;
        label.replace('\n', " ↵ ");
        if (label.length() > 60) label = label.left(57) + "...";
        QAction* act = menu->addAction(label);
        connect(act, &QAction::triggered, this, [this, entry](){ setInputText(entry); });
    }
}

bool SendWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_inputEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        bool isEnter = keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter;
        // Enter sends (matches the old single-line box); Shift+Enter inserts a newline
        // for multi-line payloads instead.
        if (isEnter && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            onSendClicked();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SendWidget::loadSettings(QSettings& settings)
{
    bool historyOn = settings.value(m_settingsKey + "/HistoryOn", true).toBool();
    m_cbHistoryOn->setChecked(historyOn);
}

void SendWidget::saveSettings(QSettings& settings)
{
    settings.setValue(m_settingsKey + "/HistoryOn", m_cbHistoryOn->isChecked());
}
