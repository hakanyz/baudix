#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QTableView>
#include "TerminalModel.h"
#include "BadgeDelegate.h"
#include <QPushButton>
#include <QComboBox>

class TerminalWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    ~TerminalWidget() = default;

    QString appendData(const QString& prefix, const QByteArray& data, const QString& color, const QString& highlightFilter);
    
    void clearTerminal();
    QString getTerminalText() const;
    void setBufferLimit(int limit);
    void setFontSize(int size);

private slots:
    void onSearchTextChanged(const QString &text);
    void onFindPrev();
    void onFindNext();
    void onClearClicked();
    void showContextMenu(const QPoint &pos);
    void copySelection();

private:
    QTableView* m_tableView;
    TerminalModel* m_model;
    BadgeDelegate* m_delegate;
    QPushButton* m_timestampCb;
    QComboBox* m_viewModeCombo;
    QLineEdit* m_searchBox;

    void setupUI();
};

#endif // TERMINALWIDGET_H
