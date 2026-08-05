#ifndef MACROWIDGET_H
#define MACROWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QSettings>

class MacroWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MacroWidget(QWidget *parent = nullptr);
    ~MacroWidget() = default;

    QString highlightFilter() const;

    void loadSettings(QSettings& settings);
    void saveSettings(QSettings& settings);

signals:
    void macroSendRequested(const QString& text);

private:
    QLineEdit* m_hlFilter;
    QListWidget* m_macrosList;

    void setupUI();
};

#endif // MACROWIDGET_H
