#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QToolBar>
#include <QAction>
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

private:
    Ui::MainWindow *ui;
    void setupDockWidgets();
    void setupToolBar();
    void refreshPorts();

    // UI Members
    QComboBox* m_portCombo;
    QComboBox* m_baudCombo;
    QComboBox* m_dataBitsCombo;
    QComboBox* m_stopBitsCombo;
    QComboBox* m_parityCombo;
    QComboBox* m_flowControlCombo;
    QPlainTextEdit* m_terminalOutput;
    
    QLineEdit* m_inputField;
    QPushButton* m_sendButton;
    
    QAction* m_actionConnect;
    QAction* m_actionDisconnect;

    // Controller
    SerialPortController* m_serialController;
};
#endif // MAINWINDOW_H
