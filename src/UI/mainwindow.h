#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QTextEdit>
#include <QToolBar>
#include <QAction>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QTabWidget>
#include <QTimer>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include "../Communication/SerialPortController.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onToggleConnectClicked();
    void onDataReceived(const QByteArray& data);
    void onConnectionStateChanged(bool isOpen, const QString& errorMsg);
    void onSendClicked();
    void onClearTerminalClicked();
    
    // Core Feature Slots
    void onPeriodicSendToggled(bool checked);
    void onPeriodicTimerTimeout();
    void onMacroResetClicked();
    void onMacroBootClicked();
    void onMacroVerClicked();
    void onSearchTextChanged(const QString &text);
    void onToggleLogging(bool checked);

private:
    Ui::MainWindow *ui;
    void setupDockWidgets();
    void setupToolBar();
    void setupCentralWidget();
    void refreshPorts();
    void appendToTerminal(const QString& prefix, const QByteArray& data, const QString& color);
    void performSend(const QString& text); // Helper

    // Core Controller
    SerialPortController* m_serialController;
    QTimer* m_periodicTimer;

    // --- UI Elements ---
    QPushButton* m_btnConnect;
    QPushButton* m_btnLog;
    QPushButton* m_btnPauseLog;

    // Connection Dock
    QComboBox* m_portCombo;
    QComboBox* m_baudCombo;
    QComboBox* m_dataBitsCombo;
    QComboBox* m_stopBitsCombo;
    QComboBox* m_parityCombo;
    QComboBox* m_flowControlCombo;
    QCheckBox* m_autoRecCb;

    // Logging Dock
    QLineEdit* m_logFilename;
    QComboBox* m_logFormat;
    QFile* m_logFile;
    QTextStream* m_logStream;

    // Send Dock
    QComboBox* m_inputCombo;
    QCheckBox* m_cbHistoryOn;
    QPushButton* m_sendButton;
    QComboBox* m_sendAsCombo;
    QCheckBox* m_periodicSendCb;
    QSpinBox* m_periodicMsBox;
    QSpinBox* m_burstBox;

    // Terminal (Central)
    QTextEdit* m_terminalOutput;
    QPushButton* m_timestampCb; // checkable toggle button
    QPushButton* m_btnAscii;
    QPushButton* m_btnHex;
    QPushButton* m_btnBoth;

    // Tools Dock
    QLineEdit* m_hlHeader;
    QLineEdit* m_hlPayload;
    QListWidget* m_macrosList;
    QLineEdit* m_searchBox;
};
#endif // MAINWINDOW_H
