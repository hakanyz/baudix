#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDockWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QTabWidget>
#include <QToolButton>
#include <QListWidget>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QInputDialog>
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    m_serialController = new SerialPortController(this);
    m_periodicTimer = new QTimer(this);
    m_logFile = nullptr;
    m_logStream = nullptr;

    setupCentralWidget();
    setupDockWidgets();

    menuBar()->hide(); // Remove the top View menu bar
    statusBar()->addPermanentWidget(new QLabel("v1.0.1 ")); // Add version to bottom right

    // Connect controller signals
    connect(m_serialController, &SerialPortController::dataReceived, this, &MainWindow::onDataReceived);
    connect(m_serialController, &SerialPortController::connectionStateChanged, this, &MainWindow::onConnectionStateChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::setupCentralWidget()
{
    QTabWidget* tabWidget = new QTabWidget(this);
    
    QWidget* terminalTab = new QWidget();
    QVBoxLayout* tabLayout = new QVBoxLayout(terminalTab);
    tabLayout->setContentsMargins(5, 5, 5, 5);
    
    // Top Bar of Terminal
    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel("Terminal Output (COMx - 115200, 8N1)"));
    topBar->addStretch();
    
    m_timestampCb = new QPushButton("Timestamp");
    m_timestampCb->setCheckable(true);
    m_timestampCb->setChecked(true);
    // No objectName - uses same blue :checked style as ASCII/HEX/Both buttons
    topBar->addWidget(m_timestampCb);
    
    m_btnAscii = new QPushButton("ASCII");
    m_btnAscii->setCheckable(true);
    m_btnAscii->setChecked(true); // Default
    m_btnHex = new QPushButton("HEX");
    m_btnHex->setCheckable(true);
    m_btnBoth = new QPushButton("Both");
    m_btnBoth->setCheckable(true);
    
    connect(m_btnAscii, &QPushButton::clicked, [this](){ m_btnHex->setChecked(false); m_btnBoth->setChecked(false); });
    connect(m_btnHex, &QPushButton::clicked, [this](){ m_btnAscii->setChecked(false); m_btnBoth->setChecked(false); });
    connect(m_btnBoth, &QPushButton::clicked, [this](){ m_btnAscii->setChecked(false); m_btnHex->setChecked(false); });
    
    topBar->addWidget(m_btnAscii);
    topBar->addWidget(m_btnHex);
    topBar->addWidget(m_btnBoth);
    
    topBar->addSpacing(15);
    QPushButton* clearBtn = new QPushButton("Clear");
    clearBtn->setObjectName("clearTerminalBtn");
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearTerminalClicked);
    topBar->addWidget(clearBtn);
    
    tabLayout->addLayout(topBar);
    
    // Terminal Output
    m_terminalOutput = new QTextEdit();
    m_terminalOutput->setObjectName("terminalOutput");
    m_terminalOutput->setReadOnly(true);
    tabLayout->addWidget(m_terminalOutput);
    
    tabWidget->addTab(terminalTab, "Terminal");
    setCentralWidget(tabWidget);
}

void MainWindow::setupDockWidgets()
{
    // Common features for all docks: No Close Button
    QDockWidget::DockWidgetFeatures dockFeatures = QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable;

    // --- Connection Dock ---
    QDockWidget *connDock = new QDockWidget("Connection", this);
    connDock->setFeatures(dockFeatures);
    QWidget *connWidget = new QWidget(connDock);
    connWidget->setObjectName("dockContent");
    QFormLayout *connLayout = new QFormLayout(connWidget);
    
    m_portCombo = new QComboBox();
    refreshPorts();
    connLayout->addRow("COM Port", m_portCombo);
    
    m_baudCombo = new QComboBox();
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200", "921600"});
    m_baudCombo->setCurrentText("115200");
    connLayout->addRow("Baud Rate", m_baudCombo);
    
    m_dataBitsCombo = new QComboBox();
    m_dataBitsCombo->addItems({"5", "6", "7", "8"});
    m_dataBitsCombo->setCurrentText("8");
    connLayout->addRow("Data Bits", m_dataBitsCombo);
    
    m_stopBitsCombo = new QComboBox();
    m_stopBitsCombo->addItems({"1", "1.5", "2"});
    connLayout->addRow("Stop Bits", m_stopBitsCombo);
    
    m_parityCombo = new QComboBox();
    m_parityCombo->addItems({"None", "Even", "Odd", "Space", "Mark"});
    connLayout->addRow("Parity", m_parityCombo);
    
    m_flowControlCombo = new QComboBox();
    m_flowControlCombo->addItems({"None", "Hardware", "Software"});
    connLayout->addRow("Flow Control", m_flowControlCombo);
    
    connLayout->addItem(new QSpacerItem(0, 5, QSizePolicy::Minimum, QSizePolicy::Fixed));

    // Put Auto Reconnect and Connect buttons in a single row
    QHBoxLayout* actionLayout = new QHBoxLayout();
    
    m_autoRecCb = new QCheckBox("Auto Reconnect");
    m_autoRecCb->setChecked(true);
    // Uses iOS Style Checkbox from QSS
    actionLayout->addWidget(m_autoRecCb);
    
    m_btnConnect = new QPushButton("Connect");
    m_btnConnect->setObjectName("connectBtn");
    m_btnConnect->setMinimumHeight(30);
    m_btnConnect->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_btnConnect, &QPushButton::clicked, this, &MainWindow::onToggleConnectClicked);
    actionLayout->addWidget(m_btnConnect);

    connLayout->addRow(actionLayout);

    connWidget->setLayout(connLayout);
    connDock->setWidget(connWidget);
    addDockWidget(Qt::LeftDockWidgetArea, connDock);

    // --- Logging Dock ---
    QDockWidget *logDock = new QDockWidget("Logging", this);
    logDock->setFeatures(dockFeatures);
    QWidget *logWidget = new QWidget(logDock);
    logWidget->setObjectName("dockContent");
    QFormLayout *logLayout = new QFormLayout(logWidget);
    
    // Filename row: text field + Browse button
    m_logFilename = new QLineEdit("baudix_log.txt");
    logLayout->addRow("Save to", m_logFilename);

    QPushButton* browseBtn = new QPushButton("Browse...");
    browseBtn->setToolTip("Choose save location");
    connect(browseBtn, &QPushButton::clicked, this, [this](){
        QString filename = QFileDialog::getSaveFileName(
            this, "Save Log File", m_logFilename->text(),
            "Text Files (*.txt);;CSV Files (*.csv);;All Files (*)"
        );
        if (!filename.isEmpty()) m_logFilename->setText(filename);
    });
    logLayout->addRow("", browseBtn);

    m_logFormat = new QComboBox();
    m_logFormat->addItems({"TXT+HEX", "TXT", "CSV"});
    logLayout->addRow("Format", m_logFormat);
    
    logLayout->addItem(new QSpacerItem(0, 10, QSizePolicy::Minimum, QSizePolicy::Fixed));

    m_btnLog = new QPushButton("Start Logging");
    m_btnLog->setCheckable(true);
    m_btnLog->setMinimumHeight(30);
    // Will style based on check state
    connect(m_btnLog, &QPushButton::toggled, this, &MainWindow::onToggleLogging);
    logLayout->addRow(m_btnLog);

    logWidget->setLayout(logLayout);
    logDock->setWidget(logWidget);
    addDockWidget(Qt::LeftDockWidgetArea, logDock);

    // --- Send Dock ---
    QDockWidget *sendDock = new QDockWidget("Send", this);
    sendDock->setFeatures(dockFeatures);
    QWidget *sendWidget = new QWidget(sendDock);
    sendWidget->setObjectName("dockContent");
    QVBoxLayout *sendLayout = new QVBoxLayout(sendWidget);
    
    // Top Row: Input + Format + Send Button
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel("Input"));
    
    m_inputField = new QLineEdit("");
    m_inputField->setPlaceholderText("Type text or HEX bytes (e.g. AA BB CC)");
    inputLayout->addWidget(m_inputField, 1);
    
    // Single format selector
    m_sendAsCombo = new QComboBox();
    m_sendAsCombo->addItems({"ASCII", "HEX"});
    m_sendAsCombo->setToolTip("ASCII: send as plain text\nHEX: parse as hex bytes (e.g. AA BB CC)");
    inputLayout->addWidget(m_sendAsCombo);
    
    m_sendButton = new QPushButton("Send");
    m_sendButton->setObjectName("sendButton");
    inputLayout->addWidget(m_sendButton);
    
    sendLayout->addLayout(inputLayout);
    
    // Middle Row: History
    QHBoxLayout *historyLayout = new QHBoxLayout();
    historyLayout->addWidget(new QLabel("History"));
    m_historyCombo = new QComboBox();
    m_historyCombo->addItem("-- Previous commands --");
    connect(m_historyCombo, QOverload<int>::of(&QComboBox::activated), [this](int idx){
        if (idx > 0) m_inputField->setText(m_historyCombo->itemText(idx));
    });
    historyLayout->addWidget(m_historyCombo, 1);
    sendLayout->addLayout(historyLayout);
    
    // Bottom Row: Periodic Send
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    
    m_periodicSendCb = new QCheckBox("Periodic Send");
    m_periodicSendCb->setToolTip("Automatically send the input at a fixed interval");
    connect(m_periodicSendCb, &QCheckBox::toggled, this, &MainWindow::onPeriodicSendToggled);
    connect(m_periodicTimer, &QTimer::timeout, this, &MainWindow::onPeriodicTimerTimeout);
    bottomLayout->addWidget(m_periodicSendCb);
    
    m_periodicMsBox = new QSpinBox();
    m_periodicMsBox->setRange(1, 10000);
    m_periodicMsBox->setValue(100);
    m_periodicMsBox->setSuffix(" ms");
    m_periodicMsBox->setToolTip("Interval between sends");
    bottomLayout->addWidget(m_periodicMsBox);
    
    bottomLayout->addWidget(new QLabel("Burst:"));
    m_burstBox = new QSpinBox();
    m_burstBox->setRange(1, 100);
    m_burstBox->setValue(1);
    m_burstBox->setToolTip("How many times to send per interval");
    bottomLayout->addWidget(m_burstBox);
    
    bottomLayout->addStretch();
    sendLayout->addLayout(bottomLayout);
    
    sendWidget->setLayout(sendLayout);
    sendDock->setWidget(sendWidget);
    addDockWidget(Qt::BottomDockWidgetArea, sendDock);

    // --- Tools Dock ---
    QDockWidget *toolsDock = new QDockWidget("Tools", this);
    toolsDock->setFeatures(dockFeatures);
    QWidget *toolsWidget = new QWidget(toolsDock);
    toolsWidget->setObjectName("dockContent");
    QVBoxLayout *toolsLayout = new QVBoxLayout(toolsWidget);
    toolsLayout->setSpacing(8);
    toolsLayout->setContentsMargins(8, 8, 8, 8);

    // --- Highlight Rules ---
    QLabel* hlTitle = new QLabel("Highlight Rules");
    hlTitle->setStyleSheet("color: #abb2bf; font-size: 13px;");
    toolsLayout->addWidget(hlTitle);

    QFormLayout* highlightLayout = new QFormLayout();
    highlightLayout->setSpacing(4);
    m_hlHeader = new QLineEdit("0xAA");
    highlightLayout->addRow("Header", m_hlHeader);
    m_hlPayload = new QLineEdit("0xCC");
    highlightLayout->addRow("Payload", m_hlPayload);
    toolsLayout->addLayout(highlightLayout);

    toolsLayout->addSpacing(8);

    // --- Macros ---
    QLabel* macroTitle = new QLabel("Macros");
    macroTitle->setStyleSheet("color: #abb2bf; font-size: 13px;");
    toolsLayout->addWidget(macroTitle);

    m_macrosList = new QListWidget();
    m_macrosList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_macrosList->setWordWrap(true);

    connect(m_macrosList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item){
        if (!item->text().isEmpty()) performSend(item->text());
    });
    connect(m_macrosList, &QListWidget::customContextMenuRequested, [this](const QPoint& pos){
        QListWidgetItem* sel = m_macrosList->currentItem();
        QMenu menu(this);
        QAction* actAdd    = menu.addAction("Add");
        QAction* actEdit   = sel ? menu.addAction("Edit")   : nullptr;
        QAction* actRemove = sel ? menu.addAction("Remove") : nullptr;
        QAction* chosen = menu.exec(m_macrosList->mapToGlobal(pos));
        if (chosen == actAdd) {
            QListWidgetItem* newItem = new QListWidgetItem("");
            newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
            m_macrosList->addItem(newItem);
            m_macrosList->editItem(newItem);
        } else if (actEdit && chosen == actEdit) {
            m_macrosList->editItem(sel);
        } else if (actRemove && chosen == actRemove) {
            delete sel;
        }
    });
    toolsLayout->addWidget(m_macrosList, 1); // stretch = 1 so it grows
    
    // Macro Action Buttons
    QHBoxLayout* macroOps = new QHBoxLayout();
    macroOps->setSpacing(4);
    
    QPushButton* btnAddMacro = new QPushButton("+ Add");
    connect(btnAddMacro, &QPushButton::clicked, [this](){
        QListWidgetItem* newItem = new QListWidgetItem("");
        newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
        m_macrosList->addItem(newItem);
        m_macrosList->scrollToItem(newItem);
        m_macrosList->editItem(newItem);
    });
    macroOps->addWidget(btnAddMacro);
    
    QPushButton* btnRemoveMacro = new QPushButton("- Remove");
    connect(btnRemoveMacro, &QPushButton::clicked, [this](){
        QListWidgetItem* sel = m_macrosList->currentItem();
        if (sel) delete sel;
    });
    macroOps->addWidget(btnRemoveMacro);
    
    toolsLayout->addLayout(macroOps);

    // Send Selected button
    QPushButton* macroSendBtn = new QPushButton("Send Selected");
    macroSendBtn->setObjectName("sendButton"); // Style like the main send button
    connect(macroSendBtn, &QPushButton::clicked, [this](){
        QListWidgetItem* sel = m_macrosList->currentItem();
        if (sel && !sel->text().isEmpty()) performSend(sel->text());
    });
    toolsLayout->addWidget(macroSendBtn);

    toolsLayout->addSpacing(8);

    // --- Search ---
    QLabel* searchTitle = new QLabel("Search");
    searchTitle->setStyleSheet("color: #abb2bf; font-size: 13px;");
    toolsLayout->addWidget(searchTitle);

    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Find text/HEX");
    connect(m_searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    toolsLayout->addWidget(m_searchBox);

    toolsWidget->setLayout(toolsLayout);
    toolsDock->setWidget(toolsWidget);
    addDockWidget(Qt::RightDockWidgetArea, toolsDock);

    // --- View Menu for Docks ---
    QMenu *viewMenu = menuBar()->addMenu("View");
    viewMenu->addAction(connDock->toggleViewAction());
    viewMenu->addAction(logDock->toggleViewAction());
    viewMenu->addAction(sendDock->toggleViewAction());
    viewMenu->addAction(toolsDock->toggleViewAction());

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_inputField, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);
}

void MainWindow::refreshPorts()
{
    m_portCombo->clear();
    m_portCombo->addItems(m_serialController->getAvailablePorts());
}

void MainWindow::onToggleConnectClicked()
{
    if (m_serialController->isOpen()) {
        m_serialController->disconnectDevice();
    } else {
        QString port = m_portCombo->currentText();
        if(port.isEmpty()) return;
        
        int baud = m_baudCombo->currentText().toInt();
        QSerialPort::DataBits dataBits = static_cast<QSerialPort::DataBits>(m_dataBitsCombo->currentText().toInt());
        QSerialPort::StopBits stopBits = QSerialPort::OneStop;
        QSerialPort::Parity parity = QSerialPort::NoParity;
        QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;
        
        m_serialController->connectDevice(port, baud, dataBits, parity, stopBits, flowControl);
    }
}

void MainWindow::appendToTerminal(const QString& prefix, const QByteArray& data, const QString& color)
{
    if (!m_terminalOutput) return;

    QString timestampStr = "";
    if (m_timestampCb->isChecked()) {
        timestampStr = QString("<span style='color:#5c6370;'>[%1]</span> ")
                       .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
    }

    // Check Highlight Rules
    QString headerRule = m_hlHeader->text().trimmed();
    QString payloadRule = m_hlPayload->text().trimmed();
    
    QString finalColor = color;
    QString bgColor = "transparent";

    // Very basic highlight logic: if data starts with the header byte
    if (!headerRule.isEmpty() && data.size() > 0) {
        bool ok;
        int headerByte = headerRule.toInt(&ok, 16); // e.g. "0xAA" -> 170
        if (ok && (quint8)data.at(0) == (quint8)headerByte) {
            bgColor = "#3e4452"; // Highlight background slightly
            finalColor = "#e5c07b"; // Highlight text yellow
        }
    }

    // Prepare HEX
    QString hexStr;
    for (char c : data) {
        hexStr += QString("0x%1 ").arg((quint8)c, 2, 16, QChar('0')).toUpper();
    }
    
    // Prepare ASCII (filter non-printables)
    QString asciiStr;
    for (char c : data) {
        if (c >= 32 && c <= 126) asciiStr += c;
        else asciiStr += ".";
    }

    QString finalDataStr = "";
    if (m_btnBoth->isChecked()) {
        finalDataStr = hexStr + " [" + asciiStr + "]";
    } else if (m_btnHex->isChecked()) {
        finalDataStr = hexStr;
    } else {
        finalDataStr = asciiStr;
    }

    QString htmlLine = QString("%1<span style='background-color:%2; color:%3;'>%4 %5</span>")
                       .arg(timestampStr, bgColor, finalColor, prefix, finalDataStr.toHtmlEscaped());

    m_terminalOutput->append(htmlLine);
    
    // Save to log if active
    if (m_logStream && m_logFile && m_logFile->isOpen()) {
        QString rawTimestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        *m_logStream << "[" << rawTimestamp << "] " << prefix << " " << finalDataStr << "\n";
        m_logStream->flush();
    }
}

void MainWindow::onDataReceived(const QByteArray& data)
{
    // #98c379 is hacker green for RX
    appendToTerminal("&lt; RX:", data, "#98c379");
}

void MainWindow::performSend(const QString& text)
{
    if (!m_serialController->isOpen() || text.isEmpty()) return;

    QByteArray data;
    // Check if HEX mode is selected
    bool isHex = (m_sendAsCombo->currentText() == "HEX");

    if (isHex) {
        // Parse HEX: "AA BB 0xCC" -> bytes
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
        // ASCII mode: send as plain UTF-8
        data = text.toUtf8();
    }

    if (data.isEmpty()) return;

    if (m_serialController->writeData(data)) {
        appendToTerminal("&gt; TX:", data, "#61afef");
    }
}

void MainWindow::onSendClicked()
{
    QString text = m_inputField->text();
    if (text.isEmpty()) return;

    performSend(text);
    
    m_inputField->selectAll();
    if (m_historyCombo->findText(text) == -1) {
        m_historyCombo->insertItem(1, text);
    }
}

void MainWindow::onPeriodicSendToggled(bool checked)
{
    if (checked) {
        m_periodicTimer->start(m_periodicMsBox->value());
    } else {
        m_periodicTimer->stop();
    }
}

void MainWindow::onPeriodicTimerTimeout()
{
    int bursts = m_burstBox->value();
    for(int i=0; i<bursts; i++) {
        QString text = m_inputField->text();
        if (!text.isEmpty()) {
            performSend(text);
        }
    }
}

void MainWindow::onMacroResetClicked()
{
    performSend("AT+RESET\\r\\n"); // Or whatever default the user wants later
}

void MainWindow::onMacroBootClicked()
{
    performSend("0x00 0xFF 0x55 0xAA"); // Default HEX test for Boot
}

void MainWindow::onMacroVerClicked()
{
    performSend("AT+GMR\\r\\n"); // Version standard command
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    if (!m_terminalOutput) return;
    
    // Find functionality
    m_terminalOutput->moveCursor(QTextCursor::Start);
    if (!text.isEmpty()) {
        m_terminalOutput->find(text); // Basic find, moves cursor to match
    }
}

void MainWindow::onClearTerminalClicked()
{
    if (m_terminalOutput) {
        m_terminalOutput->clear();
    }
}

void MainWindow::onToggleLogging(bool checked)
{
    if (checked) {
        QString filename = m_logFilename->text();
        if (filename.isEmpty()) filename = "baudix_log.txt";
        
        m_logFile = new QFile(filename);
        if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_logStream = new QTextStream(m_logFile);
            // Show recording state via Log button color
            m_btnLog->setStyleSheet("color: #e06c75; font-weight: bold;");
            m_logFilename->setEnabled(false);
            m_logFormat->setEnabled(false);
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
        m_btnLog->setStyleSheet(""); // Reset to default style
        m_logFilename->setEnabled(true);
        m_logFormat->setEnabled(true);
    }
}

void MainWindow::onConnectionStateChanged(bool isOpen, const QString& errorMsg)
{
    if (isOpen) {
        if (m_btnConnect) {
            m_btnConnect->setText("Disconnect");
            m_btnConnect->setObjectName("disconnectBtn");
            m_btnConnect->style()->unpolish(m_btnConnect);
            m_btnConnect->style()->polish(m_btnConnect);
        }
        m_portCombo->setEnabled(false);
        setWindowTitle(QString("Baudix | %1 - %2 Connected").arg(m_portCombo->currentText().split(" - ").first(), m_baudCombo->currentText()));
    } else {
        if (m_btnConnect) {
            m_btnConnect->setText("Connect");
            m_btnConnect->setObjectName("connectBtn");
            m_btnConnect->style()->unpolish(m_btnConnect);
            m_btnConnect->style()->polish(m_btnConnect);
        }
        m_portCombo->setEnabled(true);
        setWindowTitle("Baudix | Disconnected");
        refreshPorts();
        
        if (!errorMsg.isEmpty()) {
            QMessageBox::warning(this, "Connection Error", errorMsg);
        }
    }
}
