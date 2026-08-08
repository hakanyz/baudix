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

    QLabel* saveLabel = new QLabel("File & Logging:");
    saveLabel->setStyleSheet("color: #abb2bf; font-size: 11px;");
    logLayout->addWidget(saveLabel);

    // Save to row: text field + Browse button side by side
    QHBoxLayout* fileLayout = new QHBoxLayout();
    fileLayout->setSpacing(4);
    fileLayout->setContentsMargins(0,0,0,0);
    
    m_logFilename = new QLineEdit("baudix_log");
    m_logFilename->setMinimumWidth(80); // Ensure it doesn't get completely squished
    fileLayout->addWidget(m_logFilename);

    QPushButton* browseBtn = new QPushButton("📁");
    browseBtn->setObjectName("smallBtn");
    browseBtn->setFixedWidth(32);
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

    logLayout->addLayout(liveActionLayout);

    logLayout->addSpacing(5);

    QHBoxLayout* fileOpsLayout = new QHBoxLayout();
    fileOpsLayout->setContentsMargins(0, 0, 0, 0);
    fileOpsLayout->setSpacing(5);

    QPushButton* btnSendFile = new QPushButton("📄 Send File");
    btnSendFile->setToolTip("Send a file over the serial port");
    connect(btnSendFile, &QPushButton::clicked, this, &LoggingWidget::onSendFileClicked);
    
    QPushButton* btnExportTxt = new QPushButton("📥 Save All");
    connect(btnExportTxt, &QPushButton::clicked, this, &LoggingWidget::exportTerminalRequested);

    fileOpsLayout->addWidget(btnSendFile);
    fileOpsLayout->addWidget(btnExportTxt);
    
    logLayout->addLayout(fileOpsLayout);
}

void LoggingWidget::onSendFileClicked()
{
    emit sendFileRequested();
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
            
            m_logFilename->setEnabled(false);
        } else {
            // Uncheck if file couldn't be opened
            m_btnLog->blockSignals(true);
            m_btnLog->setChecked(false);
            m_btnLog->blockSignals(false);
            QMessageBox::warning(this, "Error", "Could not open log file for writing.");
        }
    } else {
        // Stop Logging
        if (m_logStream) {
            delete m_logStream;
            m_logStream = nullptr;
        }
        if (m_logFile) {
            m_logFile->close();
            delete m_logFile;
            m_logFile = nullptr;
        }
        
        m_btnLog->setText("⏺ Record");
        m_btnLog->setObjectName("connectBtn");
        m_btnLog->style()->unpolish(m_btnLog);
        m_btnLog->style()->polish(m_btnLog);
        
        m_logFilename->setEnabled(true);
    }
}

void LoggingWidget::appendLog(const QString& prefix, const QString& formattedData)
{
    if (m_logStream && m_btnLog->isChecked()) {
        QString rawTimestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        *m_logStream << "[" << rawTimestamp << "] " << prefix << " " << formattedData << "\n";
        m_logStream->flush();
    }
}
