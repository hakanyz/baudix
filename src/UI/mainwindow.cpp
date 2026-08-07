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
#include <QDesktopServices>
#include <QUrl>
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
    
    // Set default window size based on the user's manual adjustment (reduced width)
    resize(820, 768);
    
    m_serialController = new SerialPortController(this);

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
#ifdef Q_OS_WIN
                QProcess::startDetached(filePath, {"/SILENT", "/RESTART"});
#else
                QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
#endif
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
    setupMenus();

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

    // Initialize Status Bar
    m_lblTxBytes = new QLabel("TX: 0 B", this);
    m_lblRxBytes = new QLabel("RX: 0 B", this);
    m_lblErrBytes = new QLabel("ERR: 0", this);
    m_lblTxBytes->setStyleSheet("color: #61afef; padding: 0 10px; font-weight: bold;");
    m_lblRxBytes->setStyleSheet("color: #98c379; padding: 0 10px; font-weight: bold;");
    m_lblErrBytes->setStyleSheet("color: #e06c75; padding: 0 10px; font-weight: bold;");
    
    statusBar()->addPermanentWidget(m_lblTxBytes);
    statusBar()->addPermanentWidget(m_lblRxBytes);
    statusBar()->addPermanentWidget(m_lblErrBytes);
    statusBar()->setStyleSheet("background-color: #21252b; color: #abb2bf; border-top: 1px solid #181a1f;");

    // Connect controller signals
    connect(m_serialController, &ISerialTransport::dataReceived, this, &MainWindow::onDataReceived);
    connect(m_serialController, &ISerialTransport::dataSent, this, &MainWindow::onDataSent);
    connect(m_serialController, &ISerialTransport::connectionStateChanged, this, &MainWindow::onConnectionStateChanged);
    connect(m_serialController, &ISerialTransport::countersUpdated, this, &MainWindow::updateCounters);
    connect(m_serialController, &ISerialTransport::fileTransferProgress, this, &MainWindow::onFileTransferProgress);
    connect(m_serialController, &ISerialTransport::fileTransferFinished, this, &MainWindow::onFileTransferFinished);
    connect(m_serialController, &ISerialTransport::fileTransferError, this, &MainWindow::onFileTransferError);
    
    // Automatically check for updates silently 2 seconds after startup
    QTimer::singleShot(2000, this, [this](){
        m_updater->checkForUpdates(true); // true = silent
    });

    // Auto-scan COM ports every 2 seconds when disconnected so USB hotplug is detected automatically
    QTimer *portCheckTimer = new QTimer(this);
    connect(portCheckTimer, &QTimer::timeout, this, [this](){
        if (m_serialController && !m_serialController->isOpen()) {
            refreshPorts();
        }
    });
    portCheckTimer->start(2000);
    
    QSettings settings("hakanyz", "Baudix");
    if (m_macroWidget) {
        m_macroWidget->loadSettings(settings);
    }
    if (m_connectionWidget) {
        m_connectionWidget->loadSettings();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isUpdating) {
        event->accept();
        return;
    }
    
    QSettings settings("hakanyz", "Baudix");
    if (m_macroWidget) {
        m_macroWidget->saveSettings(settings);
    }
    if (m_connectionWidget) {
        m_connectionWidget->saveSettings();
    }
    QString behavior = settings.value("System/CloseBehavior", "").toString();

    if (behavior == "Tray") {
        hide();
        event->ignore();
    } else if (behavior == "Exit") {
        event->accept();
        qApp->quit();
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
            qApp->quit();
        } else {
            event->ignore();
        }
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    return QMainWindow::eventFilter(watched, event);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::setupCentralWidget()
{
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(4);

    auto wrapInCard = [](QWidget* child) -> QFrame* {
        QFrame* frame = new QFrame();
        frame->setObjectName("cardFrame");
        frame->setStyleSheet("#cardFrame { background-color: #282c34; border: 1px solid #181a1f; border-radius: 4px; }");
        QVBoxLayout* layout = new QVBoxLayout(frame);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->addWidget(child);
        return frame;
    };

    // Top Bar (Connection)
    m_connectionWidget = new ConnectionWidget();
    connect(m_connectionWidget, &ConnectionWidget::connectRequested, this, &MainWindow::onConnectRequested);
    connect(m_connectionWidget, &ConnectionWidget::disconnectRequested, this, &MainWindow::onDisconnectRequested);
    connect(m_connectionWidget, &ConnectionWidget::refreshPortsRequested, this, &MainWindow::refreshPorts);
    mainLayout->addWidget(wrapInCard(m_connectionWidget));

    // The Send Bar will be created here but added to the layout AFTER the splitter
    m_sendWidget = new SendWidget();
    connect(m_sendWidget, &SendWidget::sendDataRequested, this, &MainWindow::sendDataToController);
    connect(m_sendWidget, &SendWidget::sendFileRequested, this, &MainWindow::onSendFileClicked);

    // Splitter for Terminal and Macros/Logging
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(2);
    splitter->setStyleSheet("QSplitter::handle { background: #181a1f; }");
    
    m_terminalWidget = new TerminalWidget();
    
    // Right panel (Macros & Logging)
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(5, 5, 5, 5);
    
    m_macroWidget = new MacroWidget();
    connect(m_macroWidget, &MacroWidget::macroSendRequested, this, &MainWindow::performSend);
    rightLayout->addWidget(m_macroWidget, 1);
    
    // Logging can go below macros
    m_loggingWidget = new LoggingWidget();
    connect(m_loggingWidget, &LoggingWidget::exportTerminalRequested, this, &MainWindow::onExportTerminal);
    rightLayout->addWidget(m_loggingWidget, 0);

    splitter->addWidget(m_terminalWidget);
    splitter->addWidget(rightPanel);
    
    // Set stretch factors (Terminal gets more space)
    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter, 1); // Add splitter with stretch factor 1
    
    // Add Send Bar at the very bottom
    mainLayout->addWidget(wrapInCard(m_sendWidget), 0); // Stretch factor 0
    
    setCentralWidget(centralWidget);
    refreshPorts();
}

void MainWindow::setupMenus()
{
    QMenu *fileMenu = menuBar()->addMenu("File");
    
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
        
        QComboBox* bufferCombo = new QComboBox(&dialog);
        bufferCombo->addItem("5,000 Lines", 5000);
        bufferCombo->addItem("10,000 Lines", 10000);
        bufferCombo->addItem("50,000 Lines", 50000);
        bufferCombo->addItem("Unlimited", 0);
        
        int currentLimit = settings.value("System/BufferLimit", 5000).toInt();
        int bufIdx = bufferCombo->findData(currentLimit);
        if (bufIdx >= 0) bufferCombo->setCurrentIndex(bufIdx);
        form->addRow("Terminal Buffer Limit:", bufferCombo);
        
        QComboBox* fontSizeCombo = new QComboBox(&dialog);
        fontSizeCombo->addItem("8 pt", 8);
        fontSizeCombo->addItem("9 pt", 9);
        fontSizeCombo->addItem("10 pt", 10);
        fontSizeCombo->addItem("11 pt (Default)", 11);
        fontSizeCombo->addItem("12 pt", 12);
        fontSizeCombo->addItem("13 pt", 13);
        fontSizeCombo->addItem("14 pt", 14);
        fontSizeCombo->addItem("16 pt", 16);
        
        int currentFontSize = settings.value("UI/TerminalFontSize", 11).toInt();
        int fontIdx = fontSizeCombo->findData(currentFontSize);
        if (fontIdx >= 0) fontSizeCombo->setCurrentIndex(fontIdx);
        form->addRow("Terminal Font Size:", fontSizeCombo);
        
        layout->addLayout(form);
        
        layout->addSpacing(20);
        
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, Qt::Horizontal, &dialog);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttons);
        
        if (dialog.exec() == QDialog::Accepted) {
            settings.setValue("System/CloseBehavior", behaviorCombo->currentData().toString());
            int newLimit = bufferCombo->currentData().toInt();
            settings.setValue("System/BufferLimit", newLimit);
            
            // Apply buffer limit immediately to the live terminal
            if (m_terminalWidget) {
                m_terminalWidget->setBufferLimit(newLimit);
            }
            
            // Apply terminal font size immediately
            int newFontSize = fontSizeCombo->currentData().toInt();
            settings.setValue("UI/TerminalFontSize", newFontSize);
            if (m_terminalWidget) {
                m_terminalWidget->setFontSize(newFontSize);
            }
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
    connect(clearAct, &QAction::triggered, [this](){
        if (m_terminalWidget) m_terminalWidget->clearTerminal();
    });
    
    termMenu->addSeparator();
    
    // Toggle Timestamps removed from here because it's inside TerminalWidget now.
    // If you need global toggle, you can add a method to TerminalWidget.
    
    // Format menus removed from here since they are in TerminalWidget combo.

    // --- Help Menu ---
    QMenu *helpMenu = menuBar()->addMenu("Help");
    
    QAction* checkUpdateAct = helpMenu->addAction("Check for Updates");
    connect(checkUpdateAct, &QAction::triggered, m_updater, &Updater::checkForUpdates);
    
    QAction* aboutAct = helpMenu->addAction("About Baudix");
    connect(aboutAct, &QAction::triggered, [this](){
        QMessageBox::about(this, "About Baudix", QString("<b>Baudix</b><br>Professional Serial Terminal & Modbus Utility<br><br>Version: %1<br>Developer: hakanyz<br><br>A Qt-based modern tool for embedded engineers.").arg(BAUDIX_VERSION_STR));
    });
}

void MainWindow::refreshPorts()
{
    QStringList newPorts = m_serialController->getAvailablePorts();
    if (m_connectionWidget) {
        m_connectionWidget->setAvailablePorts(newPorts);
    }
}

void MainWindow::onConnectRequested()
{
    QString port = m_connectionWidget->portName();
    if(port.isEmpty()) return;
    
    int baud = m_connectionWidget->baudRate();
    QSerialPort::DataBits dataBits = static_cast<QSerialPort::DataBits>(m_connectionWidget->dataBits());
    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
    QSerialPort::Parity parity = QSerialPort::NoParity;
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;
    
    m_serialController->connectDevice(port, baud, dataBits, parity, stopBits, flowControl);
}

void MainWindow::onDisconnectRequested()
{
    if (m_serialController->isOpen()) {
        m_serialController->disconnectDevice();
    }
}

// appendToTerminal removed, now using TerminalWidget

void MainWindow::onDataReceived(const QByteArray& data)
{
    if (!m_terminalWidget) return;
    QString formattedStr = m_terminalWidget->appendData("< RX:", data);
    
    if (m_loggingWidget) {
        m_loggingWidget->appendLog("< RX:", formattedStr);
    }
}

void MainWindow::onDataSent(const QByteArray& data)
{
    if (!m_terminalWidget) return;
    QString formattedStr = m_terminalWidget->appendData("> TX:", data);
    
    if (m_loggingWidget) {
        m_loggingWidget->appendLog("> TX:", formattedStr);
    }
}

void MainWindow::performSend(const QString& text)
{
    if (!m_sendWidget) return;
    QByteArray data = m_sendWidget->formatData(text);
    sendDataToController(data);
}

void MainWindow::sendDataToController(const QByteArray& data)
{
    if (!m_serialController->isOpen() || data.isEmpty()) return;

    // We just write to the controller. The controller will emit dataSent() 
    // when it successfully writes the data, which triggers onDataSent to update the UI.
    m_serialController->writeData(data);
}

void MainWindow::onMacroResetClicked()
{
    performSend("AT+RESET\r\n"); // Or whatever default the user wants later
}

void MainWindow::onMacroBootClicked()
{
    performSend("0x00 0xFF 0x55 0xAA"); // Default HEX test for Boot
}

void MainWindow::onMacroVerClicked()
{
    performSend("AT+GMR\\r\\n"); // Version standard command
}

// onSearchTextChanged and onClearTerminalClicked removed, handled by TerminalWidget

void MainWindow::onExportTerminal()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save Terminal Buffer", "terminal_export.txt", "Text Files (*.txt)");
    if (!filename.isEmpty()) {
        if (!filename.endsWith(".txt", Qt::CaseInsensitive)) filename += ".txt";
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            if (m_terminalWidget) {
                out << m_terminalWidget->getTerminalText();
            }
            file.close();
        }
    }
}

// onToggleLogging removed, logic moved to LoggingWidget

void MainWindow::onConnectionStateChanged(bool isOpen, const QString& errorMsg)
{
    if (isOpen) {
        if (m_connectionWidget) {
            m_connectionWidget->setConnectedState(true);
            setWindowTitle(QString("Baudix | %1 - %2 Connected").arg(m_connectionWidget->portName().split(" - ").first(), QString::number(m_connectionWidget->baudRate())));
        }
        m_serialController->resetCounters();
    } else {
        if (m_connectionWidget) {
            m_connectionWidget->setConnectedState(false);
        }
        setWindowTitle("Baudix | Disconnected");
        refreshPorts();
        
        if (!errorMsg.isEmpty()) {
            QMessageBox::warning(this, "Connection Error", errorMsg);
        }
    }
}

void MainWindow::onSendFileClicked()
{
    if (!m_serialController->isOpen()) {
        QMessageBox::warning(this, "Not Connected", "Please connect to a serial port first.");
        return;
    }

    QString filename = QFileDialog::getOpenFileName(this, "Select File to Send", "", "All Files (*.*)");
    if (filename.isEmpty()) return;

    QFileInfo fileInfo(filename);
    if (fileInfo.size() == 0) {
        QMessageBox::information(this,"Empty File", "The selected file is empty.");
        return; 
    }
    
    m_lastFileTotalBytes = fileInfo.size();

    if (m_serialController->sendFile(filename)) {
        if (m_terminalWidget) {
            QString formattedStr = m_terminalWidget->appendData(
                "INFO",
                QString("Sending File: %1").arg(QFileInfo(filename).fileName()).toUtf8());
            if (m_loggingWidget) {
                m_loggingWidget->appendLog("INFO", formattedStr);
            }
        }
        
        m_fileProgressDialog = new QProgressDialog("Sending file...", "Cancel", 0, 100, this);
        m_fileProgressDialog->setWindowTitle("File Transfer");
        m_fileProgressDialog->setWindowModality(Qt::WindowModal);
        // We cannot easily cancel the current QSerialPort write if it's already queued,
        // but for chunks, we could add cancel logic. For now, disable cancel button.
        m_fileProgressDialog->setCancelButton(nullptr); 
        m_fileProgressDialog->show();
    } else {
        QMessageBox::critical(this, "Error", "Could not start file transfer.");
    }
}

void MainWindow::updateCounters(quint64 tx, quint64 rx, quint64 err)
{
    // Format numbers with commas (e.g., 1,024 B)
    m_lblTxBytes->setText(QString("TX: %L1 B").arg(tx));
    m_lblRxBytes->setText(QString("RX: %L1 B").arg(rx));
    m_lblErrBytes->setText(QString("ERR: %L1").arg(err));
}

void MainWindow::onFileTransferProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (m_fileProgressDialog && bytesTotal > 0) {
        m_fileProgressDialog->setMaximum(bytesTotal);
        m_fileProgressDialog->setValue(bytesSent);
    }
}

void MainWindow::onFileTransferFinished()
{
    if (m_fileProgressDialog) {
        m_fileProgressDialog->close();
        m_fileProgressDialog->deleteLater();
        m_fileProgressDialog = nullptr;
    }
    
    if (m_terminalWidget) {
        QString sizeStr;
        if (m_lastFileTotalBytes > 1024 * 1024) {
            sizeStr = QString::number(m_lastFileTotalBytes / (1024.0 * 1024.0), 'f', 2) + " MB";
        } else if (m_lastFileTotalBytes > 1024) {
            sizeStr = QString::number(m_lastFileTotalBytes / 1024.0, 'f', 2) + " KB";
        } else {
            sizeStr = QString::number(m_lastFileTotalBytes) + " B";
        }
        m_terminalWidget->appendData("INFO", QString("File Transfer Complete. (%1)").arg(sizeStr).toUtf8());
    }
}

void MainWindow::onFileTransferError(const QString& error)
{
    if (m_fileProgressDialog) {
        m_fileProgressDialog->close();
        m_fileProgressDialog->deleteLater();
        m_fileProgressDialog = nullptr;
    }
    
    QMessageBox::critical(this, "Transfer Error", "File transfer failed: " + error);
    
    if (m_terminalWidget) {
        m_terminalWidget->appendData("ERR:", QByteArray("File Transfer Failed."));
    }
}

