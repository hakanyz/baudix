#ifndef PORTSESSION_H
#define PORTSESSION_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include "ConnectionWidget.h"
#include "TerminalWidget.h"
#include "SendWidget.h"
#include "../Communication/ISerialTransport.h"

class QSettings;

// Bundles everything one serial connection needs: its own connection bar,
// terminal view, send bar and transport. MainWindow owns one or two of these
// (Port A / Port B) depending on whether Dual Mode is enabled.
class PortSession : public QObject
{
    Q_OBJECT

public:
    // label should be "A" or "B". "A" keeps the original QSettings group/key names
    // so single-port users aren't affected by this refactor.
    explicit PortSession(const QString& label, QWidget* parent = nullptr);

    QString label() const { return m_label; }
    ConnectionWidget* connectionWidget() const { return m_connectionWidget; }
    TerminalWidget* terminalWidget() const { return m_terminalWidget; }
    SendWidget* sendWidget() const { return m_sendWidget; }
    ISerialTransport* controller() const { return m_controller; }

    bool isOpen() const { return m_controller->isOpen(); }
    void refreshPorts();
    // Formats text via this session's SendWidget and writes it to this session's port.
    void performSend(const QString& text);

    void loadSettings(QSettings& settings);
    void saveSettings(QSettings& settings);

signals:
    // A fully formatted line ready to be appended to the (shared) log file.
    void logLine(const QString& prefix, const QString& formattedData);
    void countersUpdated(quint64 tx, quint64 rx, quint64 err);
    void stateChanged(bool isOpen, const QString& errorMsg);
    void fileTransferProgress(qint64 bytesSent, qint64 bytesTotal);
    void fileTransferFinished();
    void fileTransferError(const QString& error);
    // Emitted whenever the user interacts with this session (connect/disconnect/send),
    // so MainWindow can track which port is the "active" one for Macros etc.
    void activated();

private slots:
    void onConnectRequested();
    void onDisconnectRequested();
    void onSendDataRequested(const QByteArray& data);
    void onControllerDataReceived(const QByteArray& data);
    void onControllerDataSent(const QByteArray& data);
    void onControllerStateChanged(bool isOpen, const QString& errorMsg);

private:
    QString m_label;
    ISerialTransport* m_controller;
    ConnectionWidget* m_connectionWidget;
    TerminalWidget* m_terminalWidget;
    SendWidget* m_sendWidget;
};

#endif // PORTSESSION_H
