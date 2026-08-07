#include "LoggingWidget.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QStyle>
#include <QLabel>

LoggingWidget::LoggingWidget(QWidget *parent)
    : QWidget(parent), m_logFile(nullptr), m_logStream(nullptr)
{
    setupUI();
}

LoggingWidget::~LoggingWidget()
{
    if (m_logStream) {
        delete m_logStream;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
}

void LoggingWidget::setupUI()
{
    QVBoxLayout *logLayout = new QVBoxLayout(this);
    logLayout->setContentsMargins(4, 4, 4, 4);

    QLabel* saveLabel = new QLabel("Save to:");
    saveLabel->setStyleSheet("color: #abb2bf; font-size: 11px;");
    logLayout->addWidget(saveLabel);

    // Save to row: text field + Browse button side by side
    QHBoxLayout* fileLayout = new QHBoxLayout();
    fileLayout->setSpacing(4);
    fileLayout->setContentsMargins(0,0,0,0);
    
    m_logFilename = new QLineEdit("baudix_log");
    m_logFilename->setMinimumWidth(80); // Ensure it doesn't get completely squished
    fileLayout->addWidget(m_logFilename);

    QPushButton* browseBtn = new QPushButton("Browse...");
    browseBtn->setObjectName("smallBtn");
    browseBtn->setToolTip("Choose save location");
    connect(browseBtn, &QPushButton::clicked, this, &LoggingWidget::onBrowseClicked);
    fileLayout->addWidget(browseBtn);
    
    logLayout->addLayout(fileLayout);
    
    logLayout->addSpacing(5);

    QHBoxLayout* liveActionLayout = new QHBoxLayout();
    m_btnLog = new QPushButton("⏺ Record");
    m_btnLog->setObjectName("connectBtn");
    m_btnLog->setCheckable(true);
    connect(m_btnLog, &QPushButton::toggled, this, &LoggingWidget::onToggleLogging);
    liveActionLayout->addWidget(m_btnLog);

    m_btnPauseLog = new QPushButton("⏸ Pause");
    m_btnPauseLog->setCheckable(true);
    m_btnPauseLog->setEnabled(false); // Only enable when recording
    liveActionLayout->addWidget(m_btnPauseLog);

    logLayout->addLayout(liveActionLayout);

    logLayout->addSpacing(5);

    QPushButton* btnExportTxt = new QPushButton("📥 Save All");
    connect(btnExportTxt, &QPushButton::clicked, this, &LoggingWidget::exportTerminalRequested);
    logLayout->addWidget(btnExportTxt);
}

void LoggingWidget::onBrowseClicked()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save Log File", m_logFilename->text(), "Text Files (*.txt)");
    if (!filename.isEmpty()) {
        m_logFilename->setText(filename);
    }
}

void LoggingWidget::onToggleLogging(bool checked)
{
    if (checked) {
        QString filename = m_logFilename->text();
        if (filename.isEmpty()) filename = "baudix_log";
        if (!filename.endsWith(".txt", Qt::CaseInsensitive)) filename += ".txt";
        
        m_logFile = new QFile(filename);
        if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_logStream = new QTextStream(m_logFile);
            
            // Touch up: Change text and switch to "Stop/Destructive" red outline style
            m_btnLog->setText("⏹ Stop");
            m_btnLog->setObjectName("disconnectBtn");
            m_btnLog->style()->unpolish(m_btnLog);
            m_btnLog->style()->polish(m_btnLog);
            
            m_btnPauseLog->setEnabled(true);
            m_logFilename->setEnabled(false);
        } else {
            QMessageBox::warning(this, "Log Error", "Could not open log file for writing.");
            m_btnLog->setChecked(false);
            delete m_logFile;
            m_logFile = nullptr;
        }
    } else {
        if (m_logStream) {
            delete m_logStream;
            m_logStream = nullptr;
        }
        if (m_logFile) {
            m_logFile->close();
            delete m_logFile;
            m_logFile = nullptr;
        }
        
        // Touch up: Revert to "Start/Primary" blue style
        m_btnLog->setText("⏺ Record");
        m_btnLog->setObjectName("connectBtn");
        m_btnLog->style()->unpolish(m_btnLog);
        m_btnLog->style()->polish(m_btnLog);
        
        m_btnPauseLog->setChecked(false);
        m_btnPauseLog->setEnabled(false);
        
        m_logFilename->setEnabled(true);
    }
}

void LoggingWidget::appendLog(const QString& prefix, const QString& formattedData)
{
    if (!formattedData.isEmpty() && m_logStream && m_logFile && m_logFile->isOpen()) {
        if (!m_btnPauseLog->isChecked()) {
            QString rawTimestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            *m_logStream << "[" << rawTimestamp << "] " << prefix << " " << formattedData << "\n";
            m_logStream->flush();
        }
    }
}
