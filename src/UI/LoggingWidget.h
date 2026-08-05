#ifndef LOGGINGWIDGET_H
#define LOGGINGWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QFile>
#include <QTextStream>

class LoggingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoggingWidget(QWidget *parent = nullptr);
    ~LoggingWidget();

    // Appends a formatted line to the log file (if logging is active)
    void appendLog(const QString& prefix, const QString& formattedData);

signals:
    void exportTerminalRequested();

private slots:
    void onToggleLogging(bool checked);
    void onBrowseClicked();

private:
    QLineEdit* m_logFilename;
    QPushButton* m_btnLog;
    QPushButton* m_btnPauseLog;

    QFile* m_logFile;
    QTextStream* m_logStream;

    void setupUI();
};

#endif // LOGGINGWIDGET_H
