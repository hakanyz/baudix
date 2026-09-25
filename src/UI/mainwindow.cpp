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

    // Initialize Status Bar (Port A labels; Port B labels only shown in Dual Mode)
    m_lblTxBytes = new QLabel("TX: 0 B", this);
    m_lblRxBytes = new QLabel("RX: 0 B", this);
    m_lblErrBytes = new QLabel("ERR: 0", this);
    m_lblTxBytes->setStyleSheet("color: #61afef; padding: 0 10px; font-weight: bold;");
    m_lblRxBytes->setStyleSheet("color: #98c379; padding: 0 10px; font-weight: bold;");
    m_lblErrBytes->setStyleSheet("color: #e06c75; padding: 0 10px; font-weight: bold;");
    m_lblErrBytes->setVisible(false);

    m_lblTxBytesB = new QLabel("B TX: 0 B", this);
    m_lblRxBytesB = new QLabel("B RX: 0 B", this);
    m_lblErrBytesB = new QLabel("B ERR: 0", this);
    m_lblTxBytesB->setStyleSheet("color: #61afef; padding: 0 10px; font-weight: bold;");
    m_lblRxBytesB->setStyleSheet("color: #98c379; padding: 0 10px; font-weight: bold;");
    m_lblErrBytesB->setStyleSheet("color: #e06c75; padding: 0 10px; font-weight: bold;");
    m_lblTxBytesB->setVisible(false);
    m_lblRxBytesB->setVisible(false);
    m_lblErrBytesB->setVisible(false);

    statusBar()->addPermanentWidget(m_lblTxBytes);
    statusBar()->addPermanentWidget(m_lblRxBytes);
    statusBar()->addPermanentWidget(m_lblErrBytes);
    statusBar()->addPermanentWidget(m_lblTxBytesB);
    statusBar()->addPermanentWidget(m_lblRxBytesB);
    statusBar()->addPermanentWidget(m_lblErrBytesB);
    statusBar()->setStyleSheet("background-color: #21252b; color: #abb2bf; border-top: 1px solid #181a1f;");

    // Automatically check for updates silently 2 seconds after startup
    QTimer::singleShot(2000, this, [this](){
        m_updater->checkForUpdates(true); // true = silent
    });

    // Auto-scan COM ports every 2 seconds when disconnected so USB hotplug is detected automatically
    QTimer *portCheckTimer = new QTimer(this);
    connect(portCheckTimer, &QTimer::timeout, this, [this](){
        if (m_sessionA && !m_sessionA->isOpen()) m_sessionA->refreshPorts();
        if (m_sessionB && !m_sessionB->isOpen()) m_sessionB->refreshPorts();
    });
    portCheckTimer->start(2000);

    QSettings settings("hakanyz", "Baudix");
    if (m_macroWidget) {
        m_macroWidget->loadSettings(settings);
    }
    if (m_sessionA) {
        m_sessionA->loadSettings(settings);
    }
    if (m_sessionB) {
        m_sessionB->loadSettings(settings);
    }
    // Restore Dual Mode last: toggling it applies the lane B / status bar / macro target visibility.
    m_dualModeBtn->setChecked(settings.value("System/DualMode", false).toBool());
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
    if (m_sessionA) {
        m_sessionA->saveSettings(settings);
    }
    if (m_sessionB) {
        m_sessionB->saveSettings(settings);
    }
    settings.setValue("System/DualMode", m_dualMode);
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

    auto makeLaneBtn = [](const QString& text) -> QPushButton* {
        QPushButton* btn = new QPushButton(text);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { border: none; background: transparent; font-weight: bold; font-size: 11px; color: #5c6370; padding: 2px 4px; }"
            "QPushButton:hover { color: #abb2bf; }"
        );
        return btn;
    };

    // Port A / Port B sessions (Connection + Terminal + Send + transport, all bundled)
    m_sessionA = new PortSession("A", this);
    m_sessionB = new PortSession("B", this);
    wireSession(m_sessionA, 0);
    wireSession(m_sessionB, 1);

    // A lane is a self-contained column: header, Connection card, Terminal card (stretch), Send card.
    auto buildLane = [&](const QString& label, PortSession* session, QHBoxLayout** outHeader) -> QWidget* {
        QWidget* lane = new QWidget();
        QVBoxLayout* laneLayout = new QVBoxLayout(lane);
        laneLayout->setContentsMargins(0, 0, 0, 0);
        laneLayout->setSpacing(4);

        QHBoxLayout* header = new QHBoxLayout();
        QPushButton* btn = makeLaneBtn(QString("● Port %1").arg(label));
        connect(btn, &QPushButton::clicked, this, [this, label](){ setActiveSession(label == "A" ? 0 : 1); });
        header->addWidget(btn);
        header->addStretch();
        laneLayout->addLayout(header);
        if (label == "A") m_btnLaneA = btn; else m_btnLaneB = btn;
        if (outHeader) *outHeader = header;

        laneLayout->addWidget(wrapInCard(session->connectionWidget()));
        laneLayout->addWidget(wrapInCard(session->terminalWidget()), 1);
        laneLayout->addWidget(wrapInCard(session->sendWidget()), 0);
        return lane;
    };

    QHBoxLayout* laneAHeader = nullptr;
    QWidget* laneA = buildLane("A", m_sessionA, &laneAHeader);

    m_dualModeBtn = new QToolButton();
    m_dualModeBtn->setText("⧉ Dual Mode");
    m_dualModeBtn->setCheckable(true);
    m_dualModeBtn->setCursor(Qt::PointingHandCursor);
    m_dualModeBtn->setToolTip("Connect to a second COM port side by side");
    m_dualModeBtn->setStyleSheet(
        "QToolButton { background-color: transparent; color: #abb2bf; border: 1px solid #181a1f; border-radius: 4px; padding: 3px 8px; font-size: 11px; }"
        "QToolButton:hover { background-color: #3b4048; }"
        "QToolButton:checked { background-color: #3b5978; color: white; border-color: #3b5978; }"
    );
    connect(m_dualModeBtn, &QToolButton::toggled, this, &MainWindow::onDualModeToggled);
    laneAHeader->addWidget(m_dualModeBtn);

    m_laneBContainer = buildLane("B", m_sessionB, nullptr);
    m_laneBContainer->setVisible(false);

    // Right panel (Macros & Logging) - shared between Port A and Port B
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);

    m_macroWidget = new MacroWidget();
    connect(m_macroWidget, &MacroWidget::macroSendRequested, this, &MainWindow::onMacroSendRequested);
    rightLayout->addWidget(m_macroWidget, 1);

    m_loggingWidget = new LoggingWidget();
    connect(m_loggingWidget, &LoggingWidget::exportTerminalRequested, this, &MainWindow::onExportTerminal);
    connect(m_loggingWidget, &LoggingWidget::sendFileRequested, this, &MainWindow::onSendFileClicked);
    rightLayout->addWidget(m_loggingWidget, 0);

    // Lane A | Lane B (hidden unless Dual Mode) | Macros & Logging, all side by side.
    m_laneSplitter = new QSplitter(Qt::Horizontal);
    m_laneSplitter->setHandleWidth(4);
    m_laneSplitter->setStyleSheet("QSplitter::handle { background: transparent; }");
    m_laneSplitter->addWidget(laneA);
    m_laneSplitter->addWidget(m_laneBContainer);
    m_laneSplitter->addWidget(rightPanel);
    m_laneSplitter->setStretchFactor(0, 1);
    m_laneSplitter->setStretchFactor(1, 1);
    m_laneSplitter->setStretchFactor(2, 0);
    m_laneSplitter->setSizes({100000, 100000, 200});

    mainLayout->addWidget(m_laneSplitter, 1);

    setCentralWidget(centralWidget);
    setActiveSession(0);
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
            
            // Apply buffer limit immediately to both terminals (Port A and Port B)
            if (m_sessionA) m_sessionA->terminalWidget()->setBufferLimit(newLimit);
            if (m_sessionB) m_sessionB->terminalWidget()->setBufferLimit(newLimit);

            // Apply terminal font size immediately
            int newFontSize = fontSizeCombo->currentData().toInt();
            settings.setValue("UI/TerminalFontSize", newFontSize);
            if (m_sessionA) m_sessionA->terminalWidget()->setFontSize(newFontSize);
            if (m_sessionB) m_sessionB->terminalWidget()->setFontSize(newFontSize);
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
        activeSession()->terminalWidget()->clearTerminal();
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
        QMessageBox::about(this, "About Baudix", QString("<b>Baudix</b><br>A Modern, Developer-Friendly Serial Terminal for Embedded Systems<br><br>Version: %1<br>Developer: hakanyz<br>GitHub: <a href=\"https://github.com/hakanyz/baudix\">https://github.com/hakanyz/baudix</a>").arg(BAUDIX_VERSION_STR));
    });
}

void MainWindow::refreshPorts()
{
    if (m_sessionA) m_sessionA->refreshPorts();
    if (m_sessionB) m_sessionB->refreshPorts();
}

PortSession* MainWindow::activeSession() const
{
    return (m_dualMode && m_activeSession == 1) ? m_sessionB : m_sessionA;
}

void MainWindow::setActiveSession(int index)
{
    m_activeSession = index;
    QString activeStyle = "QPushButton { border: none; background: transparent; font-weight: bold; font-size: 11px; color: #61afef; padding: 2px 4px; }";
    QString inactiveStyle = "QPushButton { border: none; background: transparent; font-weight: bold; font-size: 11px; color: #5c6370; padding: 2px 4px; } QPushButton:hover { color: #abb2bf; }";
    m_btnLaneA->setStyleSheet(index == 0 ? activeStyle : inactiveStyle);
    m_btnLaneB->setStyleSheet(index == 1 ? activeStyle : inactiveStyle);
    if (m_macroWidget) {
        m_macroWidget->setActiveLabel(index == 0 ? "A" : "B");
    }
}

void MainWindow::wireSession(PortSession* session, int index)
{
    connect(session, &PortSession::logLine, this, [this, session](const QString& prefix, const QString& formattedData){
        if (!m_loggingWidget) return;
        QString taggedPrefix = m_dualMode ? QString("[%1] %2").arg(session->label(), prefix) : prefix;
        m_loggingWidget->appendLog(taggedPrefix, formattedData);
    });

    if (index == 0) {
        connect(session, &PortSession::countersUpdated, this, &MainWindow::updateCountersA);
    } else {
        connect(session, &PortSession::countersUpdated, this, &MainWindow::updateCountersB);
    }

    connect(session, &PortSession::stateChanged, this, [this, session](bool isOpen, const QString& errorMsg){
        updateWindowTitle();
        if (!isOpen && !errorMsg.isEmpty()) {
            QString msg = m_dualMode ? QString("Port %1: %2").arg(session->label(), errorMsg) : errorMsg;
            QMessageBox::warning(this, "Connection Error", msg);
        }
    });

    connect(session, &PortSession::fileTransferProgress, this, &MainWindow::onFileTransferProgress);
    connect(session, &PortSession::fileTransferFinished, this, &MainWindow::onFileTransferFinished);
    connect(session, &PortSession::fileTransferError, this, &MainWindow::onFileTransferError);

    connect(session, &PortSession::activated, this, [this, index](){ setActiveSession(index); });
}

void MainWindow::updateWindowTitle()
{
    auto sessionInfo = [](PortSession* s) -> QString {
        if (!s->isOpen()) return "Disconnected";
        return QString("%1 - %2 Connected").arg(s->connectionWidget()->portName().split(" - ").first(), QString::number(s->connectionWidget()->baudRate()));
    };

    if (m_dualMode) {
        setWindowTitle(QString("Baudix | A: %1 | B: %2").arg(sessionInfo(m_sessionA), sessionInfo(m_sessionB)));
    } else {
        setWindowTitle(m_sessionA->isOpen() ? QString("Baudix | %1").arg(sessionInfo(m_sessionA)) : "Baudix | Disconnected");
    }
}

void MainWindow::onDualModeToggled(bool checked)
{
    // Capture Lane A's current width BEFORE anything changes - it's our estimate of how much
    // extra room Lane B needs (same widget types, so a similar width looks right). Reading this
    // after Lane B becomes visible would be too late: the splitter will have already squeezed
    // Lane A down to share the still-unchanged window width.
    int laneAWidth = (m_laneSplitter && m_laneSplitter->widget(0)) ? m_laneSplitter->widget(0)->width() : width() / 2;

    if (checked) {
        // Remember the width to restore when Dual Mode is turned back off.
        m_singleModeWidth = width();
    }

    m_dualMode = checked;
    m_laneBContainer->setVisible(checked);

    // Deferred: at startup (restoring a saved Dual Mode state) the window hasn't been laid out
    // yet, so widths read here would still be 0.
    QTimer::singleShot(0, this, [this, checked, laneAWidth]() {
        if (!m_laneSplitter) return;

        // Qt does not reliably auto-grow/auto-shrink an already-shown, already-resized window
        // when a child's visibility changes, so the width is set explicitly and deterministically
        // in both directions instead of relying on it. Plain resize() can also silently get
        // clamped by a stale cached minimum/maximum size, so min==max==target is pinned to force
        // the OS window to exactly that width, then the pin is released so the window stays
        // freely resizable afterward.
        int targetWidth = checked ? (m_singleModeWidth + laneAWidth + m_laneSplitter->handleWidth())
                                   : m_singleModeWidth;
        int targetHeight = height();
        setMinimumWidth(targetWidth);
        setMaximumWidth(targetWidth);
        resize(targetWidth, targetHeight);
        setMinimumWidth(0);
        setMaximumWidth(QWIDGETSIZE_MAX);

        if (checked) {
            QList<int> sizes = m_laneSplitter->sizes();
            int rightWidth = sizes.size() >= 3 ? sizes[2] : 200;
            int remaining = m_laneSplitter->width() - rightWidth - 2 * m_laneSplitter->handleWidth();
            if (remaining < 0) remaining = 0;
            m_laneSplitter->setSizes({remaining / 2, remaining - remaining / 2, rightWidth});
        }
    });

    m_lblTxBytesB->setVisible(checked);
    m_lblRxBytesB->setVisible(checked);
    m_lblErrBytesB->setVisible(checked && m_sessionB->controller()->errorCount() > 0);
    if (m_macroWidget) {
        m_macroWidget->setDualModeVisible(checked);
    }
    if (!checked) {
        setActiveSession(0);
    }
    updateWindowTitle();
}

void MainWindow::onMacroSendRequested(const QString& text)
{
    if (!m_dualMode) {
        m_sessionA->performSend(text);
        return;
    }

    QString target = m_macroWidget->targetSelection();
    if (target == "A") {
        m_sessionA->performSend(text);
    } else if (target == "B") {
        m_sessionB->performSend(text);
    } else if (target == "Both") {
        m_sessionA->performSend(text);
        m_sessionB->performSend(text);
    } else { // "Auto"
        activeSession()->performSend(text);
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
            out << activeSession()->terminalWidget()->getTerminalText();
            file.close();
        }
    }
}

// onToggleLogging removed, logic moved to LoggingWidget

void MainWindow::onSendFileClicked()
{
    PortSession* s = activeSession();
    if (!s->isOpen()) {
        QMessageBox::warning(this, "Not Connected", m_dualMode
            ? QString("Port %1 is not connected.").arg(s->label())
            : QString("Please connect to a serial port first."));
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

    if (s->controller()->sendFile(filename)) {
        QString infoPrefix = m_dualMode ? QString("[%1] INFO").arg(s->label()) : "INFO";
        QString formattedStr = s->terminalWidget()->appendData(
            "INFO",
            QString("Sending File: %1").arg(QFileInfo(filename).fileName()).toUtf8());
        if (m_loggingWidget) {
            m_loggingWidget->appendLog(infoPrefix, formattedStr);
        }

        m_fileProgressDialog = new QProgressDialog("Sending file...", "Cancel", 0, 100, this);
        m_fileProgressDialog->setWindowTitle(m_dualMode ? QString("File Transfer - Port %1").arg(s->label()) : "File Transfer");
        m_fileProgressDialog->setWindowModality(Qt::WindowModal);
        // We cannot easily cancel the current QSerialPort write if it's already queued,
        // but for chunks, we could add cancel logic. For now, disable cancel button.
        m_fileProgressDialog->setCancelButton(nullptr);
        m_fileProgressDialog->show();
    } else {
        QMessageBox::critical(this, "Error", "Could not start file transfer.");
    }
}

void MainWindow::updateCountersA(quint64 tx, quint64 rx, quint64 err)
{
    // Format numbers with commas (e.g., 1,024 B)
    m_lblTxBytes->setText(QString(m_dualMode ? "A TX: %L1 B" : "TX: %L1 B").arg(tx));
    m_lblRxBytes->setText(QString(m_dualMode ? "A RX: %L1 B" : "RX: %L1 B").arg(rx));
    m_lblErrBytes->setText(QString(m_dualMode ? "A ERR: %L1" : "ERR: %L1").arg(err));
    m_lblErrBytes->setVisible(err > 0);
}

void MainWindow::updateCountersB(quint64 tx, quint64 rx, quint64 err)
{
    m_lblTxBytesB->setText(QString("B TX: %L1 B").arg(tx));
    m_lblRxBytesB->setText(QString("B RX: %L1 B").arg(rx));
    m_lblErrBytesB->setText(QString("B ERR: %L1").arg(err));
    m_lblErrBytesB->setVisible(m_dualMode && err > 0);
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

    PortSession* s = qobject_cast<PortSession*>(sender());
    if (!s) s = activeSession();

    QString sizeStr;
    if (m_lastFileTotalBytes > 1024 * 1024) {
        sizeStr = QString::number(m_lastFileTotalBytes / (1024.0 * 1024.0), 'f', 2) + " MB";
    } else if (m_lastFileTotalBytes > 1024) {
        sizeStr = QString::number(m_lastFileTotalBytes / 1024.0, 'f', 2) + " KB";
    } else {
        sizeStr = QString::number(m_lastFileTotalBytes) + " B";
    }
    s->terminalWidget()->appendData("INFO", QString("File Transfer Complete. (%1)").arg(sizeStr).toUtf8());
}

void MainWindow::onFileTransferError(const QString& error)
{
    if (m_fileProgressDialog) {
        m_fileProgressDialog->close();
        m_fileProgressDialog->deleteLater();
        m_fileProgressDialog = nullptr;
    }

    QMessageBox::critical(this, "Transfer Error", "File transfer failed: " + error);

    PortSession* s = qobject_cast<PortSession*>(sender());
    if (!s) s = activeSession();
    s->terminalWidget()->appendData("ERR:", QByteArray("File Transfer Failed."));
}

