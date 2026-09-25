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
#include <QPlainTextEdit>
#include <QStringList>

class SendWidget : public QWidget
{
    Q_OBJECT

public:
    // settingsKey lets two SendWidget instances (e.g. Port A / Port B) persist their
    // own settings (history on/off, etc.) independently.
    explicit SendWidget(const QString& settingsKey = "SendWidget", QWidget *parent = nullptr);
    ~SendWidget() = default;

    QByteArray formatData(const QString& text) const;
    void setInputText(const QString& text);

    void loadSettings(QSettings& settings);
    void saveSettings(QSettings& settings);

signals:
    void sendDataRequested(const QByteArray& data);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onSendClicked();
    void onPeriodicSendToggled(bool checked);
    void onPeriodicTimerTimeout();
    void onHistoryToggled(bool checked);

private:
    QPlainTextEdit* m_inputEdit;
    QToolButton* m_historyBtn;
    QStringList m_history;
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
    QString m_settingsKey;

    void setupUI();
    void adjustInputHeight();
    void rebuildHistoryMenu();
    void updatePlaceholder();
};

#endif // SENDWIDGET_H
