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
#include <QToolButton>
#include "ConnectionWidget.h"
#include "TerminalWidget.h"
#include "SendWidget.h"
#include "LoggingWidget.h"
#include "MacroWidget.h"
#include "PortSession.h"
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

private slots:
    void onExportTerminal();
    void onSendFileClicked();
    void onMacroSendRequested(const QString& text);

private:
    Ui::MainWindow *ui;
    void setupMenus();
    void setupToolBar();
    void setupCentralWidget();
    void refreshPorts();
    void wireSession(PortSession* session, int index);
    void setActiveSession(int index);
    PortSession* activeSession() const;
    void updateWindowTitle();
    void onDualModeToggled(bool checked);

    Updater* m_updater;
    bool m_isUpdating = false;
    qint64 m_lastFileTotalBytes = 0;

    // --- UI Elements ---

    // Port A / Port B sessions (each bundles Connection + Terminal + Send + transport)
    PortSession* m_sessionA;
    PortSession* m_sessionB;
    int m_activeSession = 0; // 0 = A, 1 = B
    bool m_dualMode = false;
    int m_singleModeWidth = 820; // window width to restore when leaving Dual Mode

    QToolButton* m_dualModeBtn;
    QPushButton* m_btnLaneA;
    QPushButton* m_btnLaneB;
    QWidget* m_laneBContainer;
    QSplitter* m_laneSplitter;

    // Logging Dock (shared, operates on the active session)
    LoggingWidget* m_loggingWidget;

    // Tools Dock (shared, target selectable in Dual Mode)
    MacroWidget* m_macroWidget;

    // Tray and Updater UI
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    QProgressDialog* m_downloadProgressDialog;

    // Status Bar & File Transfer UI
    class QLabel* m_lblTxBytes;
    class QLabel* m_lblRxBytes;
    class QLabel* m_lblErrBytes;
    class QLabel* m_lblTxBytesB;
    class QLabel* m_lblRxBytesB;
    class QLabel* m_lblErrBytesB;
    QProgressDialog* m_fileProgressDialog = nullptr;

private slots:
    void updateCountersA(quint64 tx, quint64 rx, quint64 err);
    void updateCountersB(quint64 tx, quint64 rx, quint64 err);
    void onFileTransferProgress(qint64 bytesSent, qint64 bytesTotal);
    void onFileTransferFinished();
    void onFileTransferError(const QString& error);
};
#endif // MAINWINDOW_H
