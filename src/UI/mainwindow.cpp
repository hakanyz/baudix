#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDockWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QRadioButton>
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
#include <QSettings>
#include <QTimer>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QProcess>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    m_serialController = new SerialPortController(this);
    m_periodicTimer = new QTimer(this);
    m_logFile = nullptr;
    m_logStream = nullptr;

    // Setup Updater first so it can be connected in menus
    m_updater = new Updater(this);
    connect(m_updater, &Updater::updateAvailable, this, [this](const QString& version, const QString& url, bool isSilent){
        QSettings settings("hakanyz", "Baudix");
        QString skippedVersion = settings.value("Updates/SkippedVersion", "").toString();
        
        // If this is a silent check on startup and the user previously skipped this exact version, ignore it.
        if (isSilent && version == skippedVersion) {
            return;
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Update Available");
        msgBox.setText(QString("A new version of Baudix (%1) is available!\n\nWould you like to download it now?").arg(version));
        
        QPushButton *downloadBtn = msgBox.addButton("Download && Install", QMessageBox::AcceptRole);
        QPushButton *remindBtn = msgBox.addButton("Remind Me Later", QMessageBox::RejectRole);
        QPushButton *skipBtn = msgBox.addButton("Skip This Version", QMessageBox::DestructiveRole);
        
        msgBox.exec();
        
        if (msgBox.clickedButton() == downloadBtn) {
            m_downloadProgressDialog = new QProgressDialog("Downloading update...", "Cancel", 0, 100, this);
            m_downloadProgressDialog->setWindowTitle("Baudix Updater");
            m_downloadProgressDialog->setWindowModality(Qt::WindowModal);
            m_downloadProgressDialog->show();

            connect(m_updater, &Updater::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal){
                if (bytesTotal > 0) {
                    m_downloadProgressDialog->setMaximum(bytesTotal);
                    m_downloadProgressDialog->setValue(bytesReceived);
                }
            });

            connect(m_updater, &Updater::downloadFinished, this, [this](const QString& filePath){
                m_downloadProgressDialog->close();
                m_downloadProgressDialog->deleteLater();
                m_isUpdating = true; // Bypass exit dialog
                QProcess::startDetached(filePath, {"/SILENT", "/RESTART"});
                qApp->quit();
            });

            m_updater->downloadUpdate(url);
        } else if (msgBox.clickedButton() == skipBtn) {
            settings.setValue("Updates/SkippedVersion", version);
        }
        // "Remind Me Later" does nothing, it will just ask again next time.
    });
    connect(m_updater, &Updater::noUpdateAvailable, this, [this](){
        QMessageBox::information(this, "Up to Date", "You are using the latest version of Baudix.");
    });
    connect(m_updater, &Updater::errorOccurred, this, [this](const QString& errorMsg){
        QMessageBox::warning(this, "Update Error", "Failed to check for updates:\n" + errorMsg);
    });

    setupCentralWidget();
    setupDockWidgets();

    // Initialize System Tray
    m_trayIcon = new QSystemTrayIcon(QIcon(":/baudix_icon.svg"), this);
    m_trayMenu = new QMenu(this);
    
    QAction* restoreAct = m_trayMenu->addAction("Show/Restore");
    connect(restoreAct, &QAction::triggered, this, &MainWindow::showNormal);
    
    m_trayMenu->addSeparator();
    
    QAction* quitAct = m_trayMenu->addAction("Quit Baudix");
    connect(quitAct, &QAction::triggered, qApp, &QCoreApplication::quit);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason){
        if (reason == QSystemTrayIcon::DoubleClick) {
            this->showNormal();
            this->activateWindow();
        }
    });

    statusBar()->hide(); // Hide empty status bar space

    // Connect controller signals
    connect(m_serialController, &SerialPortController::dataReceived, this, &MainWindow::onDataReceived);
    connect(m_serialController, &SerialPortController::connectionStateChanged, this, &MainWindow::onConnectionStateChanged);
    
    // Automatically check for updates silently 2 seconds after startup
    QTimer::singleShot(2000, this, [this](){
        m_updater->checkForUpdates(true); // true = silent
    });
    connect(m_serialController, &SerialPortController::connectionStateChanged, this, &MainWindow::onConnectionStateChanged);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isUpdating) {
        event->accept();
        return;
    }

    QSettings settings("hakanyz", "Baudix");
    QString behavior = settings.value("System/CloseBehavior", "").toString();

    if (behavior == "Tray") {
        hide();
        event->ignore();
    } else if (behavior == "Exit") {
        event->accept();
    } else {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Exit Baudix");
        msgBox.setText("What do you want to do when closing the window?");
        
        QPushButton* trayBtn = msgBox.addButton("Minimize to Tray", QMessageBox::ActionRole);
        QPushButton* exitBtn = msgBox.addButton("Exit Application", QMessageBox::DestructiveRole);
        QPushButton* cancelBtn = msgBox.addButton(QMessageBox::Cancel);
        cancelBtn->hide();

        QCheckBox* rememberCb = new QCheckBox("Remember my choice (can be changed in Settings)", &msgBox);
        msgBox.setCheckBox(rememberCb);

        msgBox.exec();

        if (msgBox.clickedButton() == trayBtn) {
            if (rememberCb->isChecked()) settings.setValue("System/CloseBehavior", "Tray");
            hide();
            event->ignore();
            m_trayIcon->showMessage("Baudix", "Application is still running in the background.", QSystemTrayIcon::Information, 2000);
        } else if (msgBox.clickedButton() == exitBtn) {
            if (rememberCb->isChecked()) settings.setValue("System/CloseBehavior", "Exit");
            event->accept();
        } else {
            event->ignore();
        }
    }
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
    m_searchBox->setMaximumWidth(200);
    connect(m_searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_searchBox, &QLineEdit::returnPressed, [this](){
        if (!m_searchBox->text().isEmpty()) m_terminalOutput->find(m_searchBox->text());
    });
    topBar->addWidget(m_searchBox);

    QPushButton* btnFindPrev = new QPushButton("▲");
    btnFindPrev->setObjectName("iconBtn");
    btnFindPrev->setFixedWidth(28);
    connect(btnFindPrev, &QPushButton::clicked, [this](){
        if (!m_searchBox->text().isEmpty()) m_terminalOutput->find(m_searchBox->text(), QTextDocument::FindBackward);
    });
    topBar->addWidget(btnFindPrev);

    QPushButton* btnFindNext = new QPushButton("▼");
    btnFindNext->setObjectName("iconBtn");
    btnFindNext->setFixedWidth(28);
    connect(btnFindNext, &QPushButton::clicked, [this](){
        if (!m_searchBox->text().isEmpty()) m_terminalOutput->find(m_searchBox->text());
    });
    topBar->addWidget(btnFindNext);
    
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
    
    // Modbus Tab (Placeholder)
    QWidget* modbusTab = new QWidget();
    QVBoxLayout* modbusLayout = new QVBoxLayout(modbusTab);
    QLabel* modbusLabel = new QLabel("Modbus UI (Coming Soon...)");
    modbusLabel->setAlignment(Qt::AlignCenter);
    modbusLabel->setStyleSheet("color: #61afef; font-size: 16px;");
    modbusLayout->addWidget(modbusLabel);
    tabWidget->addTab(modbusTab, "Modbus");
    
    setCentralWidget(tabWidget);
}

void MainWindow::setupDockWidgets()
{
    // Configure dock corners so side docks extend all the way to the bottom,
    // squishing the bottom Send dock into the center perfectly underneath the terminal.
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Common features for all docks: No Close Button
    QDockWidget::DockWidgetFeatures dockFeatures = QDockWidget::DockWidgetMovable;

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
    QFormLayout *logLayout = new QFormLayout(logWidget);
    
    // Save to row: text field + Browse button side by side
    QHBoxLayout* fileLayout = new QHBoxLayout();
    fileLayout->setSpacing(4);
    fileLayout->setContentsMargins(0,0,0,0);
    
    m_logFilename = new QLineEdit("baudix_log");
    fileLayout->addWidget(m_logFilename);

    QPushButton* browseBtn = new QPushButton("Browse...");
    browseBtn->setObjectName("smallBtn");
    browseBtn->setToolTip("Choose save location");
    connect(browseBtn, &QPushButton::clicked, [this](){
        QString filename = QFileDialog::getSaveFileName(this, "Save Log File", m_logFilename->text(), "Text Files (*.txt)");
        if (!filename.isEmpty()) m_logFilename->setText(filename);
    });
    fileLayout->addWidget(browseBtn);
    
    logLayout->addRow("Save to", fileLayout);
    
    logLayout->addItem(new QSpacerItem(0, 5, QSizePolicy::Minimum, QSizePolicy::Fixed));

    QHBoxLayout* liveActionLayout = new QHBoxLayout();
    m_btnLog = new QPushButton("⏺ Record");
    m_btnLog->setObjectName("connectBtn");
    m_btnLog->setCheckable(true);
    m_btnLog->setMinimumHeight(30);
    connect(m_btnLog, &QPushButton::toggled, this, &MainWindow::onToggleLogging);
    liveActionLayout->addWidget(m_btnLog);

    m_btnPauseLog = new QPushButton("⏸ Pause");
    m_btnPauseLog->setCheckable(true);
    m_btnPauseLog->setMinimumHeight(30);
    m_btnPauseLog->setEnabled(false); // Only enable when recording
    liveActionLayout->addWidget(m_btnPauseLog);

    logLayout->addRow(liveActionLayout);

    logLayout->addItem(new QSpacerItem(0, 5, QSizePolicy::Minimum, QSizePolicy::Fixed));

    QPushButton* btnExportTxt = new QPushButton("📥 Save All");
    btnExportTxt->setMinimumHeight(28);
    connect(btnExportTxt, &QPushButton::clicked, this, &MainWindow::onExportTerminal);
    logLayout->addRow(btnExportTxt);

    logWidget->setLayout(logLayout);
    logDock->setWidget(logWidget);
    addDockWidget(Qt::LeftDockWidgetArea, logDock);

    // --- Send Dock ---
    QDockWidget *sendDock = new QDockWidget("Send", this);
    sendDock->setFeatures(dockFeatures);
    
    // UI/UX Improvement: Remove Title Bar from Send dock to save vertical space
    QWidget* emptyTitle = new QWidget();
    emptyTitle->setFixedHeight(0);
    sendDock->setTitleBarWidget(emptyTitle);

    QWidget *sendWidget = new QWidget(sendDock);
    sendWidget->setObjectName("dockContent");
    QVBoxLayout *sendLayout = new QVBoxLayout(sendWidget);
    
    // Top Row: Input (History Combo) + History Controls + Format + Send
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel("Input"));
    
    m_inputCombo = new QComboBox();
    m_inputCombo->setEditable(true);
    m_inputCombo->lineEdit()->setPlaceholderText("Type text or HEX bytes (e.g. AA BB CC)");
    m_inputCombo->addItem(""); // Default empty
    // Enter sends the command
    connect(m_inputCombo->lineEdit(), &QLineEdit::returnPressed, this, &MainWindow::onSendClicked);
    inputLayout->addWidget(m_inputCombo, 1);
    
    m_cbHistoryOn = new QCheckBox("ON");
    m_cbHistoryOn->setChecked(true);
    m_cbHistoryOn->setToolTip("Save sent commands to history");
    inputLayout->addWidget(m_cbHistoryOn);
    
    QPushButton* btnClearHistory = new QPushButton("Clear");
    btnClearHistory->setObjectName("smallBtn");
    connect(btnClearHistory, &QPushButton::clicked, [this](){
        m_inputCombo->clear();
        m_inputCombo->addItem("");
    });
    inputLayout->addWidget(btnClearHistory);
    
    // Single format selector
    m_sendAsCombo = new QComboBox();
    m_sendAsCombo->addItems({"ASCII", "HEX"});
    m_sendAsCombo->setToolTip("ASCII: send as plain text\nHEX: parse as hex bytes");
    inputLayout->addWidget(m_sendAsCombo);
    
    m_sendButton = new QPushButton("Send");
    m_sendButton->setObjectName("sendButton");
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    inputLayout->addWidget(m_sendButton);
    
    sendLayout->addLayout(inputLayout);
    
    // Bottom Row: Periodic Send
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    
    bottomLayout->addWidget(new QLabel("Append:"));
    
    m_appendCombo = new QComboBox();
    m_appendCombo->addItems({"None", "CR", "LF", "CRLF"});
    bottomLayout->addWidget(m_appendCombo);
    
    bottomLayout->addSpacing(15);
    
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
    
    // Add Send Button to the far right of the bottom row to save space
    m_sendButton = new QPushButton("Send");
    m_sendButton->setObjectName("sendButton");
    m_sendButton->setMinimumWidth(120);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    bottomLayout->addWidget(m_sendButton);
    
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
    btnAddMacro->setObjectName("smallBtn");
    connect(btnAddMacro, &QPushButton::clicked, [this](){
        QListWidgetItem* newItem = new QListWidgetItem("");
        newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
        m_macrosList->addItem(newItem);
        m_macrosList->scrollToItem(newItem);
        m_macrosList->editItem(newItem);
    });
    macroOps->addWidget(btnAddMacro);
    
    QPushButton* btnRemoveMacro = new QPushButton("- Remove");
    btnRemoveMacro->setObjectName("smallBtn");
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
    toolsLayout->addStretch();
    toolsWidget->setLayout(toolsLayout);
    toolsDock->setWidget(toolsWidget);
    addDockWidget(Qt::RightDockWidgetArea, toolsDock);

    // --- File Menu ---
    QMenu *fileMenu = menuBar()->addMenu("File");
    
    QAction* toggleLogAct = fileMenu->addAction("Start/Stop Logging");
    toggleLogAct->setShortcut(QKeySequence("Ctrl+R"));
    connect(toggleLogAct, &QAction::triggered, m_btnLog, &QPushButton::click);
    
    QAction* exportAct = fileMenu->addAction("Export Terminal");
    exportAct->setShortcut(QKeySequence("Ctrl+S"));
    connect(exportAct, &QAction::triggered, this, &MainWindow::onExportTerminal);
    QAction* settingsAct = fileMenu->addAction("Settings...");
    connect(settingsAct, &QAction::triggered, this, [this](){
        QDialog dialog(this);
        dialog.setWindowTitle("Application Settings");
        dialog.setMinimumWidth(350);
        
        QVBoxLayout* layout = new QVBoxLayout(&dialog);
        
        QFormLayout* form = new QFormLayout();
        QComboBox* behaviorCombo = new QComboBox(&dialog);
        behaviorCombo->addItem("Ask me every time", "");
        behaviorCombo->addItem("Minimize to Tray", "Tray");
        behaviorCombo->addItem("Exit Application", "Exit");
        
        QSettings settings("hakanyz", "Baudix");
        QString currentBehavior = settings.value("System/CloseBehavior", "").toString();
        int idx = behaviorCombo->findData(currentBehavior);
        if (idx >= 0) behaviorCombo->setCurrentIndex(idx);
        
        form->addRow("Close Behavior:", behaviorCombo);
        layout->addLayout(form);
        
        layout->addSpacing(20);
        
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, Qt::Horizontal, &dialog);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttons);
        
        if (dialog.exec() == QDialog::Accepted) {
            settings.setValue("System/CloseBehavior", behaviorCombo->currentData().toString());
        }
    });
    
    fileMenu->addSeparator();
    
    QAction* exitAct = fileMenu->addAction("Exit");
    exitAct->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    // --- Terminal Menu ---
    QMenu *termMenu = menuBar()->addMenu("Terminal");
    
    QAction* clearAct = termMenu->addAction("Clear Screen");
    clearAct->setShortcut(QKeySequence("Ctrl+L"));
    connect(clearAct, &QAction::triggered, this, &MainWindow::onClearTerminalClicked);
    
    termMenu->addSeparator();
    
    QAction* toggleTimeAct = termMenu->addAction("Toggle Timestamps");
    toggleTimeAct->setShortcut(QKeySequence("Ctrl+T"));
    connect(toggleTimeAct, &QAction::triggered, m_timestampCb, &QPushButton::click);
    
    QMenu *formatMenu = termMenu->addMenu("Format");
    QAction* formatAsciiAct = formatMenu->addAction("ASCII");
    connect(formatAsciiAct, &QAction::triggered, [this](){ m_viewModeCombo->setCurrentText("ASCII"); });
    
    QAction* formatHexAct = formatMenu->addAction("HEX");
    connect(formatHexAct, &QAction::triggered, [this](){ m_viewModeCombo->setCurrentText("HEX"); });
    
    QAction* formatBothAct = formatMenu->addAction("Both");
    connect(formatBothAct, &QAction::triggered, [this](){ m_viewModeCombo->setCurrentText("Both"); });

    // --- Help Menu ---
    QMenu *helpMenu = menuBar()->addMenu("Help");
    
    QAction* checkUpdateAct = helpMenu->addAction("Check for Updates");
    connect(checkUpdateAct, &QAction::triggered, m_updater, &Updater::checkForUpdates);
    
    QAction* aboutAct = helpMenu->addAction("About Baudix");
    connect(aboutAct, &QAction::triggered, [this](){
        QMessageBox::about(this, "About Baudix", "<b>Baudix</b><br>Professional Serial Terminal & Modbus Utility<br><br>Version: 1.2.2<br>Developer: hakanyz<br><br>A Qt-based modern tool for embedded engineers.");
    });
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

    QString htmlLine = QString("%1<span style='background-color:%2; color:%3;'>%4 %5</span>")
                       .arg(timestampStr, bgColor, finalColor, prefix, finalDataStr.toHtmlEscaped());

    m_terminalOutput->append(htmlLine);
    
    // Save to log if active and NOT paused
    if (m_logStream && m_logFile && m_logFile->isOpen()) {
        if (!m_btnPauseLog->isChecked()) {
            QString rawTimestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            *m_logStream << "[" << rawTimestamp << "] " << prefix << " " << finalDataStr << "\n";
            m_logStream->flush();
        }
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
    QString appendMode = m_appendCombo->currentText();
    if (appendMode == "CR") data.append('\r');
    else if (appendMode == "LF") data.append('\n');
    else if (appendMode == "CRLF") { data.append('\r'); data.append('\n'); }

    if (data.isEmpty()) return;

    if (m_serialController->writeData(data)) {
        appendToTerminal("&gt; TX:", data, "#61afef");
    }
}

void MainWindow::onSendClicked()
{
    QString text = m_inputCombo->currentText();
    if (text.isEmpty()) return;

    performSend(text);
    
    m_inputCombo->lineEdit()->selectAll();
    
    if (m_cbHistoryOn->isChecked()) {
        if (m_inputCombo->findText(text) == -1) {
            m_inputCombo->insertItem(1, text);
        }
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
        QString text = m_inputCombo->currentText();
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

void MainWindow::onExportTerminal()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save Terminal Buffer", "terminal_export.txt", "Text Files (*.txt)");
    if (!filename.isEmpty()) {
        if (!filename.endsWith(".txt", Qt::CaseInsensitive)) filename += ".txt";
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << m_terminalOutput->toPlainText();
            file.close();
        }
    }
}

void MainWindow::onToggleLogging(bool checked)
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
