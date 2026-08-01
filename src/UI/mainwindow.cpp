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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    m_serialController = new SerialPortController(this);
    m_periodicTimer = new QTimer(this);

    setupToolBar();
    setupCentralWidget();
    setupDockWidgets();

    // Connect controller signals
    connect(m_serialController, &SerialPortController::dataReceived, this, &MainWindow::onDataReceived);
    connect(m_serialController, &SerialPortController::connectionStateChanged, this, &MainWindow::onConnectionStateChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar("Main ToolBar");
    toolBar->setMovable(false);

    m_btnConnect = new QToolButton();
    m_btnConnect->setText("Connect");
    m_btnConnect->setObjectName("connectBtn");
    m_btnConnect->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_actionConnectToggle = toolBar->addWidget(m_btnConnect);

    toolBar->addSeparator();

    // Placeholder actions
    toolBar->addAction(QIcon(), "Settings");
    toolBar->addAction(QIcon(), "Log");
    toolBar->addAction(QIcon(), "Clear");
    toolBar->addAction(QIcon(), "Find");
    toolBar->addAction(QIcon(), "Macro");

    connect(m_btnConnect, &QToolButton::clicked, this, &MainWindow::onToggleConnectClicked);
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
    
    m_timestampCb = new QCheckBox("Timestamp");
    m_timestampCb->setChecked(true);
    topBar->addWidget(m_timestampCb);
    
    m_btnAscii = new QPushButton("ASCII");
    m_btnAscii->setCheckable(true);
    m_btnHex = new QPushButton("HEX");
    m_btnHex->setCheckable(true);
    m_btnBoth = new QPushButton("Both");
    m_btnBoth->setCheckable(true);
    m_btnBoth->setChecked(true); // Default
    
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
    
    m_autoRecCb = new QCheckBox("Auto Reconnect");
    m_autoRecCb->setChecked(true);
    connLayout->addRow(m_autoRecCb);
    
    connWidget->setLayout(connLayout);
    connDock->setWidget(connWidget);
    addDockWidget(Qt::LeftDockWidgetArea, connDock);

    // --- Logging Dock ---
    QDockWidget *logDock = new QDockWidget("Logging", this);
    logDock->setFeatures(dockFeatures);
    QWidget *logWidget = new QWidget(logDock);
    logWidget->setObjectName("dockContent");
    QFormLayout *logLayout = new QFormLayout(logWidget);
    
    QHBoxLayout* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(new QLineEdit("baudix_log.txt"));
    QPushButton* browseBtn = new QPushButton("...");
    browseBtn->setFixedWidth(30);
    fileLayout->addWidget(browseBtn);
    logLayout->addRow("Filename", fileLayout);
    
    QComboBox* formatCombo = new QComboBox();
    formatCombo->addItems({"TXT+HEX", "TXT", "CSV"});
    logLayout->addRow("Format", formatCombo);
    
    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->addWidget(new QPushButton("Import"));
    actionLayout->addWidget(new QPushButton("Export"));
    logLayout->addRow("Action", actionLayout);
    
    logLayout->addRow("Status", new QLabel("Idle"));
    logWidget->setLayout(logLayout);
    logDock->setWidget(logWidget);
    addDockWidget(Qt::LeftDockWidgetArea, logDock);

    // --- Send Dock ---
    QDockWidget *sendDock = new QDockWidget("Send", this);
    sendDock->setFeatures(dockFeatures);
    QWidget *sendWidget = new QWidget(sendDock);
    sendWidget->setObjectName("dockContent");
    QVBoxLayout *sendLayout = new QVBoxLayout(sendWidget);
    
    // Top Row: Input, Send Button, Selectors
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel("Input"));
    
    m_inputField = new QLineEdit("");
    inputLayout->addWidget(m_inputField, 1);
    
    inputLayout->addWidget(new QLabel("Selected:"));
    QComboBox* selCombo = new QComboBox();
    selCombo->addItems({"HEX", "ASCII"});
    inputLayout->addWidget(selCombo);
    
    m_sendButton = new QPushButton("Send");
    m_sendButton->setObjectName("sendButton");
    inputLayout->addWidget(m_sendButton);
    
    inputLayout->addWidget(new QLabel("Send As:"));
    m_sendAsCombo = new QComboBox();
    m_sendAsCombo->addItems({"HEX | ASCII", "HEX", "ASCII"});
    inputLayout->addWidget(m_sendAsCombo);
    
    sendLayout->addLayout(inputLayout);
    
    // Middle Row: History
    QHBoxLayout *historyLayout = new QHBoxLayout();
    historyLayout->addWidget(new QLabel("History"));
    m_historyCombo = new QComboBox();
    m_historyCombo->addItem("Previous commands");
    historyLayout->addWidget(m_historyCombo, 1);
    sendLayout->addLayout(historyLayout);
    
    // Bottom Row: Periodic Send & Send File
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    QCheckBox* sendFileCb = new QCheckBox("Send File...");
    bottomLayout->addWidget(sendFileCb);
    
    bottomLayout->addStretch();
    
    bottomLayout->addWidget(new QLabel("Periodic Send"));
    m_periodicMsBox = new QSpinBox();
    m_periodicMsBox->setRange(1, 10000);
    m_periodicMsBox->setValue(100);
    m_periodicMsBox->setSuffix("ms");
    bottomLayout->addWidget(m_periodicMsBox);
    
    bottomLayout->addWidget(new QLabel("Burst:"));
    m_burstBox = new QSpinBox();
    m_burstBox->setRange(1, 100);
    m_burstBox->setValue(1);
    bottomLayout->addWidget(m_burstBox);
    
    m_periodicSendCb = new QCheckBox("");
    connect(m_periodicSendCb, &QCheckBox::toggled, this, &MainWindow::onPeriodicSendToggled);
    connect(m_periodicTimer, &QTimer::timeout, this, &MainWindow::onPeriodicTimerTimeout);
    bottomLayout->addWidget(m_periodicSendCb);
    
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
    
    toolsLayout->addWidget(new QLabel("Highlight Rules"));
    QFormLayout* highlightLayout = new QFormLayout();
    m_hlHeader = new QLineEdit("0xAA");
    highlightLayout->addRow("Header", m_hlHeader);
    m_hlPayload = new QLineEdit("0xCC");
    highlightLayout->addRow("Payload", m_hlPayload);
    toolsLayout->addLayout(highlightLayout);
    
    toolsLayout->addWidget(new QLabel("Macro List"));
    QHBoxLayout* macroBtns = new QHBoxLayout();
    
    QPushButton* btnReset = new QPushButton("Reset");
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::onMacroResetClicked);
    macroBtns->addWidget(btnReset);
    
    QPushButton* btnBoot = new QPushButton("Boot");
    connect(btnBoot, &QPushButton::clicked, this, &MainWindow::onMacroBootClicked);
    macroBtns->addWidget(btnBoot);
    
    QPushButton* btnVer = new QPushButton("Ver");
    connect(btnVer, &QPushButton::clicked, this, &MainWindow::onMacroVerClicked);
    macroBtns->addWidget(btnVer);
    
    toolsLayout->addLayout(macroBtns);
    
    toolsLayout->addWidget(new QLabel("Macros"));
    m_macrosList = new QListWidget();
    m_macrosList->addItem("AT+RESET");
    m_macrosList->addItem("0xAA 0xBB 0xCC (Test)");
    toolsLayout->addWidget(m_macrosList, 1);
    
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Find text/HEX");
    connect(m_searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    toolsLayout->addWidget(new QLabel("Search"));
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
    bool isHex = (m_sendAsCombo->currentText() == "HEX" || m_sendAsCombo->currentText() == "HEX | ASCII");

    if (isHex) {
        // Parse HEX string (e.g. "AA BB 0xCC")
        QString cleanText = text;
        cleanText.replace("0x", "", Qt::CaseInsensitive);
        cleanText.remove(QRegularExpression("\\s+")); // Remove all spaces
        
        if (cleanText.length() % 2 != 0) {
            cleanText.prepend("0"); // Pad if odd number of characters
        }
        
        for (int i = 0; i < cleanText.length(); i += 2) {
            bool ok;
            uint byteVal = cleanText.mid(i, 2).toUInt(&ok, 16);
            if (ok) {
                data.append((char)byteVal);
            }
        }
    } else {
        data = text.toUtf8();
    }

    if (data.isEmpty()) return; // Invalid parsing

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
