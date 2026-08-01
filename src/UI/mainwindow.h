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
    void onConnectClicked();
    void onDisconnectClicked();
    void onDataReceived(const QByteArray& data);
    void onConnectionStateChanged(bool isOpen, const QString& errorMsg);
    void onSendClicked();
    void onClearTerminalClicked();

private:
    Ui::MainWindow *ui;
    void setupDockWidgets();
    void setupToolBar();
    void setupCentralWidget();
    void refreshPorts();
    void appendToTerminal(const QString& prefix, const QByteArray& data, const QString& color);

    // Core Controller
    SerialPortController* m_serialController;

    // --- UI Elements ---
    // ToolBar Actions
    QAction* m_actionConnect;
    QAction* m_actionDisconnect;

    // Connection Dock
    QComboBox* m_portCombo;
    QComboBox* m_baudCombo;
    QComboBox* m_dataBitsCombo;
    QComboBox* m_stopBitsCombo;
    QComboBox* m_parityCombo;
    QComboBox* m_flowControlCombo;
    QCheckBox* m_autoRecCb;

    // Send Dock
    QLineEdit* m_inputField;
    QPushButton* m_sendButton;
    QComboBox* m_sendAsCombo;
    QComboBox* m_historyCombo;
    QCheckBox* m_periodicSendCb;
    QSpinBox* m_periodicMsBox;
    QSpinBox* m_burstBox;

    // Terminal (Central)
    QTextEdit* m_terminalOutput;
    QCheckBox* m_timestampCb;
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
