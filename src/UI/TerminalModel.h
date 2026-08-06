#ifndef TERMINALMODEL_H
#define TERMINALMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QString>
#include <QColor>

struct LogEntry {
    QString timestamp;
    QString direction; // "TX", "RX", or "E"
    int length;
    QString hexData;
    QString asciiData;
    QByteArray rawData;  // original bytes for detail panel / hex dump
    bool isMatch; // for filtering/highlighting
    QColor color; // Override background color if needed
};

class TerminalModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TerminalModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void addEntry(const LogEntry& entry);
    void clearData();
    void setMaxRows(int maxRows);

    void setViewMode(const QString& mode);
    void setFilter(const QString& filterText);
    void setFontSize(int size);

    const QList<LogEntry>& entries() const { return m_entries; }

private:
    void reevaluateFilter();
    bool checkMatch(const LogEntry& entry, const QString& filterText) const;

    QList<LogEntry> m_entries;
    int m_maxRows;
    QString m_viewMode = "Both";
    QString m_filter;
    int m_fontSize = 10;
};

#endif // TERMINALMODEL_H
