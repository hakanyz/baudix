#include "TerminalWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QSettings>
#include <QRegularExpression>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void TerminalWidget::setupUI()
{
    QVBoxLayout* tabLayout = new QVBoxLayout(this);
    tabLayout->setContentsMargins(5, 5, 5, 5);
    
    // Top Bar of Terminal
    QHBoxLayout* topBar = new QHBoxLayout();
    
    m_timestampCb = new QPushButton("Timestamp");
    m_timestampCb->setCheckable(true);
    m_timestampCb->setChecked(true);
    m_timestampCb->setObjectName("smallBtn");
    topBar->addWidget(m_timestampCb);
    
    m_viewModeCombo = new QComboBox();
    m_viewModeCombo->addItems({"ASCII", "HEX", "Both"});
    m_viewModeCombo->setCurrentText("ASCII");
    topBar->addWidget(m_viewModeCombo);

    topBar->addSpacing(10);

    // Search bar integrated into Top Bar
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Search terminal...");
    m_searchBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_searchBox, &QLineEdit::textChanged, this, &TerminalWidget::onSearchTextChanged);
    connect(m_searchBox, &QLineEdit::returnPressed, this, &TerminalWidget::onFindNext);
    topBar->addWidget(m_searchBox);

    QPushButton* btnFindPrev = new QPushButton("▲");
    btnFindPrev->setObjectName("iconBtn");
    btnFindPrev->setFixedWidth(28);
    connect(btnFindPrev, &QPushButton::clicked, this, &TerminalWidget::onFindPrev);
    topBar->addWidget(btnFindPrev);

    QPushButton* btnFindNext = new QPushButton("▼");
    btnFindNext->setObjectName("iconBtn");
    btnFindNext->setFixedWidth(28);
    connect(btnFindNext, &QPushButton::clicked, this, &TerminalWidget::onFindNext);
    topBar->addWidget(btnFindNext);
    
    topBar->addSpacing(20); // Separate from search controls
    
    QPushButton* clearBtn = new QPushButton("Clear Screen");
    clearBtn->setObjectName("clearTerminalBtn");
    connect(clearBtn, &QPushButton::clicked, this, &TerminalWidget::onClearClicked);
    topBar->addWidget(clearBtn);
    
    tabLayout->addLayout(topBar);
    
    // Terminal Output
    m_terminalOutput = new QPlainTextEdit();
    m_terminalOutput->setObjectName("terminalOutput");
    m_terminalOutput->setReadOnly(true);
    
    QSettings settings("hakanyz", "Baudix");
    int bufferLimit = settings.value("System/BufferLimit", 5000).toInt();
    if (bufferLimit > 0) {
        m_terminalOutput->setMaximumBlockCount(bufferLimit);
    }
    
    // Apply saved terminal font size
    int termFontSize = settings.value("UI/TerminalFontSize", 11).toInt();
    QFont termFont = m_terminalOutput->font();
    termFont.setPointSize(termFontSize);
    m_terminalOutput->setFont(termFont);
    
    tabLayout->addWidget(m_terminalOutput);
}

void TerminalWidget::onSearchTextChanged(const QString &text)
{
    if (!m_terminalOutput) return;
    
    m_terminalOutput->moveCursor(QTextCursor::Start);
    if (!text.isEmpty()) {
        m_terminalOutput->find(text);
    }
}

void TerminalWidget::onFindPrev()
{
    if (!m_searchBox->text().isEmpty()) {
        m_terminalOutput->find(m_searchBox->text(), QTextDocument::FindBackward);
    }
}

void TerminalWidget::onFindNext()
{
    if (!m_searchBox->text().isEmpty()) {
        m_terminalOutput->find(m_searchBox->text());
    }
}

void TerminalWidget::onClearClicked()
{
    clearTerminal();
}

void TerminalWidget::clearTerminal()
{
    if (m_terminalOutput) {
        m_terminalOutput->clear();
    }
}

QString TerminalWidget::getTerminalText() const
{
    if (m_terminalOutput) {
        return m_terminalOutput->toPlainText();
    }
    return QString();
}

void TerminalWidget::setBufferLimit(int limit)
{
    if (m_terminalOutput) {
        m_terminalOutput->setMaximumBlockCount(limit > 0 ? limit : 0);
    }
}

void TerminalWidget::setFontSize(int size)
{
    if (m_terminalOutput) {
        QFont termFont = m_terminalOutput->font();
        termFont.setPointSize(size);
        m_terminalOutput->setFont(termFont);
    }
}

QString TerminalWidget::appendData(const QString& prefix, const QByteArray& data, const QString& color, const QString& highlightFilter)
{
    if (!m_terminalOutput) return QString();

    QString timestampStr = "";
    if (m_timestampCb->isChecked()) {
        timestampStr = QString("[%1] ")
                       .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
    }

    QString filterRule = highlightFilter.trimmed();
    QString finalColor = color;
    QString bgColor = "transparent";

    if (!filterRule.isEmpty()) {
        bool isMatch = false;

        // 1. ASCII Match (case-insensitive search in data)
        if (QString::fromUtf8(data).contains(filterRule, Qt::CaseInsensitive)) {
            isMatch = true;
        }

        // 2. HEX Match (parse filter as HEX bytes)
        if (!isMatch) {
            QString cleanHex = filterRule;
            cleanHex.replace("0x", "", Qt::CaseInsensitive);
            cleanHex.remove(QRegularExpression("[^0-9a-fA-F]"));
            if (!cleanHex.isEmpty() && cleanHex.length() % 2 == 0) {
                QByteArray hexBytes;
                for (int i = 0; i < cleanHex.length(); i += 2) {
                    bool ok;
                    uint b = cleanHex.mid(i, 2).toUInt(&ok, 16);
                    if (ok) hexBytes.append((char)b);
                }
                if (!hexBytes.isEmpty() && data.contains(hexBytes)) {
                    isMatch = true;
                }
            }
        }

        if (isMatch) {
            bgColor = "#3e4452";    // Highlight background
            finalColor = "#e5c07b";  // Highlight text yellow
        }
    }

    // Prepare HEX
    QString hexStr;
    hexStr.reserve(data.size() * 5);
    for (char c : data) {
        hexStr += QString("0x%1 ").arg((quint8)c, 2, 16, QChar('0')).toUpper();
    }
    
    // Prepare ASCII (filter non-printables)
    QString asciiStr;
    asciiStr.reserve(data.size() * 4);
    for (char c : data) {
        if (c >= 32 && c <= 126) {
            asciiStr += c;
        } else if (c == '\r') {
            asciiStr += "<CR>";
        } else if (c == '\n') {
            asciiStr += "<LF>";
        } else {
            asciiStr += ".";
        }
    }

    QString finalDataStr = "";
    QString viewMode = m_viewModeCombo->currentText();
    if (viewMode == "Both") {
        finalDataStr = hexStr + " [" + asciiStr + "]";
    } else if (viewMode == "HEX") {
        finalDataStr = hexStr;
    } else {
        finalDataStr = asciiStr;
    }

    // Prepare QTextCursor for insertion
    QTextCursor cursor(m_terminalOutput->document());
    cursor.movePosition(QTextCursor::End);

    // Apply Timestamp if needed
    if (!timestampStr.isEmpty()) {
        QTextCharFormat tsFormat;
        tsFormat.setForeground(QColor("#5c6370"));
        cursor.insertText(timestampStr, tsFormat);
    }

    // Apply Prefix formatting (Always keep original color for TX/RX)
    QTextCharFormat prefixFormat;
    prefixFormat.setForeground(QColor(color));
    cursor.insertText(prefix + " ", prefixFormat);

    // Apply Data formatting
    QTextCharFormat dataFormat;
    if (bgColor != "transparent") {
        dataFormat.setBackground(QColor(bgColor));
        dataFormat.setForeground(QColor(finalColor)); // Vurgulama rengi
    } else {
        dataFormat.setForeground(QColor("#abb2bf")); // Standart terminal metin rengi
    }

    // Insert data
    cursor.insertText(finalDataStr + "\n", dataFormat);

    // Ensure it scrolls down if the user was at the bottom
    m_terminalOutput->ensureCursorVisible();
    
    return finalDataStr;
}
