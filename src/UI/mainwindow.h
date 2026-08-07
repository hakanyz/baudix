#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>

#include <QToolBar>
#include <QAction>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QIcon>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QApplication>
#include <QSettings>
#include <QInputDialog>
#include <QTabWidget>
#include <QCloseEvent>
#include <QProgressDialog>
#include <QTimer>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QSplitter>
#include "ConnectionWidget.h"
#include "TerminalWidget.h"
#include "SendWidget.h"
#include "LoggingWidget.h"
#include "MacroWidget.h"
#include "../Communication/SerialPortController.h"
#include "../Core/Updater.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onConnectRequested();
    void onDisconnectRequested();
    void onDataReceived(const QByteArray& data);
    void onDataSent(const QByteArray& data);
    void onConnectionStateChanged(bool isOpen, const QString& errorMsg);
    void sendDataToController(const QByteArray& data);
    void onExportTerminal();
    
    // Core Feature Slots
    void onMacroResetClicked();
    void onMacroBootClicked();
    void onMacroVerClicked();
    void onSendFileClicked();

private:
    Ui::MainWindow *ui;
    void setupMenus();
    void setupToolBar();
    void setupCentralWidget();
    void refreshPorts();
    void performSend(const QString& text); // Helper for Macros

    // Core Controller
    ISerialTransport* m_serialController;
    Updater* m_updater;
    bool m_isUpdating = false;

    // --- UI Elements ---

    // Connection Dock
    ConnectionWidget* m_connectionWidget;

    // Terminal View Settings
    TerminalWidget* m_terminalWidget;

    // Send Dock
    SendWidget* m_sendWidget;

    // Logging Dock
    LoggingWidget* m_loggingWidget;

    // Tools Dock
    MacroWidget* m_macroWidget;

    // Tray and Updater UI
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    QProgressDialog* m_downloadProgressDialog;
    
    // Status Bar & File Transfer UI
    class QLabel* m_lblTxBytes;
    class QLabel* m_lblRxBytes;
    class QLabel* m_lblErrBytes;
    QProgressDialog* m_fileProgressDialog = nullptr;
    
private slots:
    void updateCounters(quint64 tx, quint64 rx, quint64 err);
    void onFileTransferProgress(qint64 bytesSent, qint64 bytesTotal);
    void onFileTransferFinished();
    void onFileTransferError(const QString& error);
};
#endif // MAINWINDOW_H
