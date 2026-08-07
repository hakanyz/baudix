#ifndef CONNECTIONWIDGET_H
#define CONNECTIONWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class ConnectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionWidget(QWidget *parent = nullptr);
    ~ConnectionWidget() = default;

    QString portName() const;
    int baudRate() const;
    int dataBits() const;
    QString stopBits() const;
    QString parity() const;
    QString flowControl() const;

    void setAvailablePorts(const QStringList& ports);
    void setConnectedState(bool isConnected);
    void loadSettings();
    void saveSettings();

signals:
    void connectRequested();
    void disconnectRequested();
    void refreshPortsRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onConnectButtonClicked();
    void toggleSettings();

private:
    QWidget* m_settingsContainer;
    QPushButton* m_toggleBtn;
    QLabel* m_statusLabel;
    
    QComboBox* m_portCombo;
    QComboBox* m_baudCombo;
    QComboBox* m_dataBitsCombo;
    QComboBox* m_stopBitsCombo;
    QComboBox* m_parityCombo;
    QComboBox* m_flowControlCombo;
    QPushButton* m_btnConnect;

    bool m_isConnected = false;

    void setupUI();
    void updateStatusLabel();
};

#endif // CONNECTIONWIDGET_H
