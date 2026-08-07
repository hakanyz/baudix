#ifndef SENDWIDGET_H
#define SENDWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QSettings>
#include <QDialog>
#include <QToolButton>

class SendWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SendWidget(QWidget *parent = nullptr);
    ~SendWidget() = default;

    QByteArray formatData(const QString& text) const;
    void setInputText(const QString& text);
    
    void loadSettings(QSettings& settings);
    void saveSettings(QSettings& settings);

signals:
    void sendDataRequested(const QByteArray& data);
    void sendFileRequested();

private slots:
    void onSendClicked();
    void onPeriodicSendToggled(bool checked);
    void onPeriodicTimerTimeout();
    void onHistoryToggled(bool checked);
    void onClearHistoryClicked();
    void onSendFileClicked();

private:
    QComboBox* m_inputCombo;
    QCheckBox* m_cbHistoryOn;
    QComboBox* m_appendCombo;
    QComboBox* m_sendAsCombo;
    QPushButton* m_sendButton;
    QCheckBox* m_periodicSendCb;
    QSpinBox* m_periodicMsBox;
    QSpinBox* m_burstBox;
    QTimer* m_periodicTimer;
    QString m_periodicText;
    QDialog* m_settingsPopup;

    void setupUI();
};

#endif // SENDWIDGET_H
