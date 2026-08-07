#include "TerminalModel.h"
#include <QColor>
#include <QFont>

TerminalModel::TerminalModel(QObject *parent)
    : QAbstractTableModel(parent), m_maxRows(5000), m_viewMode("Both")
{
}

int TerminalModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_entries.count();
}

int TerminalModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return 4; // Timestamp, Direction, Length, Data (Dynamic)
}

QVariant TerminalModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.count())
        return QVariant();

    const LogEntry &entry = m_entries.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return entry.timestamp;
            case 1: return entry.direction;
            case 2: return (entry.direction == "I" || entry.direction == "E") ? QString("-") : QString::number(entry.length);
            case 3: {
                QString full;
                if (m_viewMode == "ASCII")     full = entry.asciiData;
                else if (m_viewMode == "HEX")  full = entry.hexData;
                else                           full = entry.hexData + " [" + entry.asciiData + "]";
                // Truncate ridiculously long data to avoid freezing Qt's text measurement,
                // but let QTableView handle the visual ellipsis (Qt::ElideRight).
                constexpr int kMaxDisplay = 2048;
                if (full.length() > kMaxDisplay)
                    return full.left(kMaxDisplay);
                return full;
            }
        }
    }
    else if (role == Qt::ToolTipRole && index.column() == 3) {
        // Full string on hover (no truncation)
        if (m_viewMode == "ASCII")    return entry.asciiData;
        if (m_viewMode == "HEX")      return entry.hexData;
        return entry.hexData + " [" + entry.asciiData + "]";
    }
    else if (role == Qt::ForegroundRole) {
        // Highlight matches or errors
        if (entry.direction == "E") return QColor("#e06c75"); // Red for error
        
        // Dim timestamp and length
        if (index.column() == 0 || index.column() == 2) return QColor("#7f848e");
        
        if (entry.isMatch) return QColor("#ffcc00"); // Bright yellow for match
        
        return QColor("#abb2bf"); // Default text color
    }
    else if (role == Qt::BackgroundRole) {
        if (entry.direction == "E") return QColor(100, 20, 20); // Dark red background for errors
        if (entry.isMatch) return QColor("#333322"); // Subtle dark yellow background for match
    }
    else if (role == Qt::TextAlignmentRole) {
        if (index.column() == 0) return static_cast<int>(Qt::AlignCenter); // Center timestamp
        if (index.column() == 1) return static_cast<int>(Qt::AlignCenter); // Center direction
        if (index.column() == 2) return static_cast<int>(Qt::AlignCenter); // Center length
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }
    else if (role == Qt::FontRole) {
        QFont font("Consolas", m_fontSize);
        return font;
    }

    return QVariant();
}

QVariant TerminalModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return "Timestamp";
            case 1: return "Direction";
            case 2: return "Length";
            case 3: return "Data";
        }
    }
    else if (role == Qt::TextAlignmentRole && orientation == Qt::Horizontal) {
        return static_cast<int>(Qt::AlignCenter);
    }
    return QVariant();
}

void TerminalModel::addEntry(const LogEntry& entry)
{
    LogEntry finalEntry = entry;
    finalEntry.isMatch = checkMatch(finalEntry, m_filter);

    // Auto trim if we exceed max rows
    if (m_maxRows > 0 && m_entries.count() >= m_maxRows) {
        int rowsToRemove = qMax(1, m_maxRows / 10);
        beginRemoveRows(QModelIndex(), 0, rowsToRemove - 1);
        m_entries.erase(m_entries.begin(), m_entries.begin() + rowsToRemove);
        endRemoveRows();
    }

    beginInsertRows(QModelIndex(), m_entries.count(), m_entries.count());
    m_entries.append(finalEntry);
    endInsertRows();
}

void TerminalModel::clearData()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

void TerminalModel::setMaxRows(int maxRows)
{
    m_maxRows = maxRows;
}

void TerminalModel::setViewMode(const QString& mode)
{
    if (m_viewMode != mode) {
        m_viewMode = mode;
        if (!m_entries.isEmpty()) {
            emit dataChanged(index(0, 3), index(m_entries.count() - 1, 3));
        }
    }
}

void TerminalModel::setFilter(const QString& filterText)
{
    if (m_filter != filterText) {
        m_filter = filterText;
        reevaluateFilter();
    }
}

void TerminalModel::setFontSize(int size)
{
    if (m_fontSize != size) {
        m_fontSize = size;
        if (!m_entries.isEmpty()) {
            emit dataChanged(index(0, 0), index(m_entries.count() - 1, columnCount() - 1),
                             {Qt::FontRole});
        }
    }
}

void TerminalModel::reevaluateFilter()
{
    if (m_entries.isEmpty()) return;
    
    bool changed = false;
    for (int i = 0; i < m_entries.count(); ++i) {
        bool newMatch = checkMatch(m_entries[i], m_filter);
        if (m_entries[i].isMatch != newMatch) {
            m_entries[i].isMatch = newMatch;
            changed = true;
        }
    }
    
    if (changed) {
        emit dataChanged(index(0, 0), index(m_entries.count() - 1, columnCount() - 1));
    }
}

bool TerminalModel::checkMatch(const LogEntry& entry, const QString& filterText) const
{
    if (filterText.isEmpty()) return false;
    
    if (entry.asciiData.contains(filterText, Qt::CaseInsensitive)) return true;
    if (entry.hexData.contains(filterText, Qt::CaseInsensitive)) return true;
    
    return false;
}
