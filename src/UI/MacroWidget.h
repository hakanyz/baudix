#ifndef MACROWIDGET_H
#define MACROWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QSettings>
#include <QComboBox>

class MacroWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MacroWidget(QWidget *parent = nullptr);
    ~MacroWidget() = default;


    void loadSettings(QSettings& settings);
    void saveSettings(QSettings& settings);

    // Shows/hides the "Send to: A / B / Both" target row (only relevant in Dual Mode).
    void setDualModeVisible(bool visible);
    // Updates the "Auto" option's label to reflect which port is currently active.
    void setActiveLabel(const QString& label);
    // Returns "Auto", "A", "B" or "Both".
    QString targetSelection() const;

signals:
    void macroSendRequested(const QString& text);

private:
    QListWidget* m_macrosList;
    QWidget* m_targetRow;
    QComboBox* m_targetCombo;

    void setupUI();
};

#endif // MACROWIDGET_H
