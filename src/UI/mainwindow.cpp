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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupDockWidgets();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupDockWidgets()
{
    // --- Connection Dock ---
    QDockWidget *connDock = new QDockWidget("Connection", this);
    QWidget *connWidget = new QWidget(connDock);
    QFormLayout *connLayout = new QFormLayout(connWidget);
    
    QComboBox* portCombo = new QComboBox();
    portCombo->addItem("COM3 - Connected"); // Placeholder
    connLayout->addRow("COM Port", portCombo);
    
    QComboBox* baudCombo = new QComboBox();
    baudCombo->addItems({"9600", "115200", "921600"});
    baudCombo->setCurrentText("115200");
    connLayout->addRow("Baud Rate", baudCombo);
    
    connLayout->addRow("Data Bits", new QComboBox());
    connLayout->addRow("Stop Bits", new QComboBox());
    connLayout->addRow("Parity", new QComboBox());
    connLayout->addRow("Flow Control", new QComboBox());
    
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
    inputLayout->addWidget(new QLineEdit(""));
    QPushButton *sendBtn = new QPushButton("Send");
    sendBtn->setStyleSheet("background-color: #2b5c92; color: white; font-weight: bold;");
    inputLayout->addWidget(sendBtn);
    
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
