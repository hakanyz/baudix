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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // Find the terminal output created in UI file
    m_terminalOutput = this->findChild<QPlainTextEdit*>("terminalOutput");
    if (!m_terminalOutput) {
        m_terminalOutput = new QPlainTextEdit(this);
        m_terminalOutput->setReadOnly(true);
        setCentralWidget(m_terminalOutput);
    }

    m_serialController = new SerialPortController(this);

    setupToolBar();
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

    m_actionConnect = new QAction(QIcon(":/baudix_icon.svg"), "Connect", this);
    m_actionDisconnect = new QAction("Disconnect", this);
    m_actionDisconnect->setEnabled(false); // Initially disabled

    toolBar->addAction(m_actionConnect);
    toolBar->addAction(m_actionDisconnect);

    connect(m_actionConnect, &QAction::triggered, this, &MainWindow::onConnectClicked);
    connect(m_actionDisconnect, &QAction::triggered, this, &MainWindow::onDisconnectClicked);
}

void MainWindow::setupDockWidgets()
{
    // --- Connection Dock ---
    QDockWidget *connDock = new QDockWidget("Connection", this);
    QWidget *connWidget = new QWidget(connDock);
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
    
    QCheckBox* autoRecCb = new QCheckBox("Auto Reconnect");
    autoRecCb->setChecked(true);
    connLayout->addRow(autoRecCb);
    
    connWidget->setLayout(connLayout);
    connDock->setWidget(connWidget);
    addDockWidget(Qt::LeftDockWidgetArea, connDock);

    // --- Logging Dock ---
    QDockWidget *logDock = new QDockWidget("Logging", this);
    QWidget *logWidget = new QWidget(logDock);
    QFormLayout *logLayout = new QFormLayout(logWidget);
    logLayout->addRow("Filename", new QLineEdit("baudix_log.txt"));
    logLayout->addRow("Format", new QComboBox());
    logLayout->addRow("Status", new QLabel("Recording..."));
    logWidget->setLayout(logLayout);
    logDock->setWidget(logWidget);
    addDockWidget(Qt::LeftDockWidgetArea, logDock);

    // --- Send Dock ---
    QDockWidget *sendDock = new QDockWidget("Send", this);
    QWidget *sendWidget = new QWidget(sendDock);
    QVBoxLayout *sendLayout = new QVBoxLayout(sendWidget);
    
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel("Input"));
    
    m_inputField = new QLineEdit("");
    inputLayout->addWidget(m_inputField);
    
    m_sendButton = new QPushButton("Send");
    m_sendButton->setStyleSheet("background-color: #2b5c92; color: white; font-weight: bold;");
    inputLayout->addWidget(m_sendButton);
    
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_inputField, &QLineEdit::returnPressed, this, &MainWindow::onSendClicked); // Pressing Enter also sends
    
    sendLayout->addLayout(inputLayout);
    
    QHBoxLayout *historyLayout = new QHBoxLayout();
    historyLayout->addWidget(new QLabel("History"));
    historyLayout->addWidget(new QComboBox());
    sendLayout->addLayout(historyLayout);
    
    sendWidget->setLayout(sendLayout);
    sendDock->setWidget(sendWidget);
    addDockWidget(Qt::BottomDockWidgetArea, sendDock);

    // --- Tools Dock ---
    QDockWidget *toolsDock = new QDockWidget("Tools", this);
    QWidget *toolsWidget = new QWidget(toolsDock);
    QVBoxLayout *toolsLayout = new QVBoxLayout(toolsWidget);
    toolsLayout->addWidget(new QLabel("<b>Highlight Rules</b>"));
    
    QFormLayout* highlightLayout = new QFormLayout();
    highlightLayout->addRow("Header", new QLineEdit("0xAA"));
    highlightLayout->addRow("Payload", new QLineEdit("0xCC"));
    toolsLayout->addLayout(highlightLayout);
    
    toolsLayout->addWidget(new QLabel("<b>Macro List</b>"));
    QHBoxLayout* macroBtns = new QHBoxLayout();
    macroBtns->addWidget(new QPushButton("Reset"));
    macroBtns->addWidget(new QPushButton("Boot"));
    macroBtns->addWidget(new QPushButton("Ver"));
    toolsLayout->addLayout(macroBtns);
    
    toolsLayout->addStretch();
    toolsWidget->setLayout(toolsLayout);
    toolsDock->setWidget(toolsWidget);
    addDockWidget(Qt::RightDockWidgetArea, toolsDock);
}

void MainWindow::refreshPorts()
{
    m_portCombo->clear();
    m_portCombo->addItems(m_serialController->getAvailablePorts());
}

void MainWindow::onConnectClicked()
{
    QString port = m_portCombo->currentText();
    if(port.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a valid COM port.");
        return;
    }
    
    int baud = m_baudCombo->currentText().toInt();
    
    QSerialPort::DataBits dataBits = static_cast<QSerialPort::DataBits>(m_dataBitsCombo->currentText().toInt());
    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
    if (m_stopBitsCombo->currentText() == "2") stopBits = QSerialPort::TwoStop;
    else if (m_stopBitsCombo->currentText() == "1.5") stopBits = QSerialPort::OneAndHalfStop;
    
    QSerialPort::Parity parity = QSerialPort::NoParity;
    if (m_parityCombo->currentText() == "Even") parity = QSerialPort::EvenParity;
    else if (m_parityCombo->currentText() == "Odd") parity = QSerialPort::OddParity;
    
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;
    if (m_flowControlCombo->currentText() == "Hardware") flowControl = QSerialPort::HardwareControl;
    else if (m_flowControlCombo->currentText() == "Software") flowControl = QSerialPort::SoftwareControl;

    m_serialController->connectDevice(port, baud, dataBits, parity, stopBits, flowControl);
}

void MainWindow::onDisconnectClicked()
{
    m_serialController->disconnectDevice();
}

void MainWindow::onDataReceived(const QByteArray& data)
{
    if (m_terminalOutput) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        // Convert to ASCII representation for basic text log
        QString text = QString("[%1] < RX: %2").arg(timestamp, QString::fromLocal8Bit(data));
        m_terminalOutput->appendPlainText(text);
    }
}

void MainWindow::onConnectionStateChanged(bool isOpen, const QString& errorMsg)
{
    if (isOpen) {
        m_actionConnect->setEnabled(false);
        m_actionDisconnect->setEnabled(true);
        m_portCombo->setEnabled(false); // Lock port selection
        setWindowTitle(QString("Baudix | %1 - %2 Connected").arg(m_portCombo->currentText().split(" - ").first(), m_baudCombo->currentText()));
    } else {
        m_actionConnect->setEnabled(true);
        m_actionDisconnect->setEnabled(false);
        m_portCombo->setEnabled(true);
        setWindowTitle("Baudix | Disconnected");
        refreshPorts(); // Refresh list on disconnect in case devices changed
        
        if (!errorMsg.isEmpty()) {
            QMessageBox::warning(this, "Connection Error", errorMsg);
        }
    }
}

void MainWindow::onSendClicked()
{
    if (!m_serialController->isOpen()) {
        QMessageBox::warning(this, "Not Connected", "Please connect to a device first.");
        return;
    }

    QString text = m_inputField->text();
    if (text.isEmpty()) return;

    QByteArray data = text.toUtf8(); // Just UTF-8 string for now
    if (m_serialController->writeData(data)) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        m_terminalOutput->appendPlainText(QString("[%1] > TX: %2").arg(timestamp, text));
        
        // Optional: Select text so user can just type to overwrite, or clear it
        m_inputField->selectAll();
    } else {
        QMessageBox::critical(this, "Send Error", "Failed to write data to serial port.");
    }
}
