#include "TerminalWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QSettings>
#include <QRegularExpression>
#include <QLineEdit>
#include <QHeaderView>
#include <QScrollBar>
#include <QShortcut>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QSplitter>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void TerminalWidget::setupUI()
{
    QVBoxLayout* tabLayout = new QVBoxLayout(this);
    tabLayout->setContentsMargins(5, 5, 5, 5);
    tabLayout->setSpacing(12); // Add elegant breathing room instead of a crude line
    
    // Top Bar of Terminal
    QHBoxLayout* topBar = new QHBoxLayout();
    
    m_timestampCb = new QPushButton("Timestamp");
    m_timestampCb->setCheckable(true);
    m_timestampCb->setChecked(true);
    m_timestampCb->setObjectName("smallBtn");
    connect(m_timestampCb, &QPushButton::toggled, this, [this](bool checked) {
        if (m_tableView) {
            m_tableView->setColumnHidden(0, !checked);
        }
    });
    topBar->addWidget(m_timestampCb);
    
    m_viewModeCombo = new QComboBox();
    m_viewModeCombo->addItems({"ASCII", "HEX", "Both"});
    m_viewModeCombo->setCurrentText("ASCII");
    connect(m_viewModeCombo, &QComboBox::currentTextChanged, this, [this](const QString& text){
        if (m_model) m_model->setViewMode(text);
    });
    topBar->addWidget(m_viewModeCombo);

    topBar->addSpacing(10);

    // Search bar integrated into Top Bar
    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Search terminal...");
    m_searchBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_searchBox, &QLineEdit::textChanged, this, &TerminalWidget::onSearchTextChanged);
    connect(m_searchBox, &QLineEdit::returnPressed, this, &TerminalWidget::onFindNext);
    topBar->addWidget(m_searchBox);

    // Match count label
    m_matchCountLabel = new QLabel();
    m_matchCountLabel->setMinimumWidth(70);
    m_matchCountLabel->setAlignment(Qt::AlignCenter);
    m_matchCountLabel->setStyleSheet("font-size: 11px;");
    m_matchCountLabel->hide();
    topBar->addWidget(m_matchCountLabel);

    QPushButton* btnFindPrev = new QPushButton("▲");
    btnFindPrev->setObjectName("iconBtn");
    btnFindPrev->setFixedWidth(28);
    connect(btnFindPrev, &QPushButton::clicked, this, &TerminalWidget::onFindPrev);
    topBar->addWidget(btnFindPrev);

    QPushButton* btnFindNext = new QPushButton("▼");
    btnFindNext->setObjectName("iconBtn");
    btnFindNext->setFixedWidth(28);
    connect(btnFindNext, &QPushButton::clicked, this, &TerminalWidget::onFindNext);
    topBar->addWidget(btnFindNext);
    
    topBar->addSpacing(20); // Separate from search controls
    
    QPushButton* clearBtn = new QPushButton("Clear Screen");
    clearBtn->setObjectName("clearTerminalBtn");
    connect(clearBtn, &QPushButton::clicked, this, &TerminalWidget::onClearClicked);
    topBar->addWidget(clearBtn);
    
    tabLayout->addLayout(topBar);
    
    // Terminal Output as Table
    m_tableView = new QTableView();
    m_tableView->setObjectName("terminalTable");
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setShowGrid(true);
    m_tableView->setGridStyle(Qt::SolidLine);
    m_tableView->setAlternatingRowColors(true);
    
    // Context Menu
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QTableView::customContextMenuRequested, this, &TerminalWidget::showContextMenu);
    
    // Add Copy Shortcut (Now opens context menu)
    QShortcut* copyShortcut = new QShortcut(QKeySequence::Copy, m_tableView);
    copyShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(copyShortcut, &QShortcut::activated, this, &TerminalWidget::copySelection);
    
    // Style the table for a modern analyzer look
    m_tableView->setStyleSheet(R"(
        QTableView {
            background-color: #282c34;
            alternate-background-color: #21252b;
            color: #abb2bf;
            border-top: 1px solid #181a1f;
            border-right: none;
            border-bottom: none;
            border-left: 1px solid #181a1f;
            gridline-color: #181a1f;
        }
        QTableView::item {
            padding: 2px 8px;
        }
        QHeaderView::section {
            background-color: #2c313a;
            color: #abb2bf;
            padding: 4px 8px;
            border: none;
            border-right: 1px solid #181a1f;
            border-bottom: 1px solid #181a1f;
            font-weight: 600; /* Bold */
            font-size: 12px; /* Standard size */
        }
    )");

    m_model = new TerminalModel(this);
    m_delegate = new BadgeDelegate(this);
    m_dataDelegate = new DataColumnDelegate(this);
    
    m_model->setViewMode(m_viewModeCombo->currentText()); // Sync model with combobox startup state

    m_tableView->setModel(m_model);
    m_tableView->setItemDelegateForColumn(1, m_delegate); // Column 1 is Direction
    m_tableView->setItemDelegateForColumn(3, m_dataDelegate); // Column 3 is Data

    // Column widths - Fixed mode for fixed columns, Stretch only for Data
    QHeaderView* header = m_tableView->horizontalHeader();
    header->setVisible(false);
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    header->setStretchLastSection(false); // Don't double-stretch, we set it above

    m_tableView->setColumnWidth(0, 115); // Timestamp (Narrowed per user request)
    m_tableView->setColumnWidth(1, 65);  // Direction
    m_tableView->setColumnWidth(2, 55);  // Length

    // Sync column visibility with button state at startup
    m_tableView->setColumnHidden(0, !m_timestampCb->isChecked());

    QSettings settings("hakanyz", "Baudix");
    int bufferLimit = settings.value("System/BufferLimit", 5000).toInt();
    m_model->setMaxRows(bufferLimit);
    
    // Apply saved terminal font size
    int termFontSize = settings.value("UI/TerminalFontSize", 10).toInt();
    setFontSize(termFontSize);
    
    // Vertical splitter: table on top, detail panel on bottom
    QSplitter* vSplitter = new QSplitter(Qt::Vertical);
    vSplitter->setHandleWidth(3);
    vSplitter->setStyleSheet("QSplitter::handle { background: #181a1f; }");
    vSplitter->addWidget(m_tableView);

    // Detail panel container
    m_detailContainer = new QWidget(this);
    QVBoxLayout* detailLayout = new QVBoxLayout(m_detailContainer);
    detailLayout->setContentsMargins(0, 0, 0, 0);
    detailLayout->setSpacing(0);
    
    // Header for detail panel
    QWidget* detailHeader = new QWidget(m_detailContainer);
    detailHeader->setObjectName("detailHeader");
    detailHeader->setStyleSheet("#detailHeader { background-color: #21252b; border-bottom: 1px solid #181a1f; }");
    QHBoxLayout* headerLayout = new QHBoxLayout(detailHeader);
    headerLayout->setContentsMargins(8, 4, 8, 4);
    
    QLabel* detailTitle = new QLabel("Packet Details");
    detailTitle->setStyleSheet("color: #abb2bf; font-weight: bold; font-size: 11px; border: none;");
    
    QPushButton* closeDetailBtn = new QPushButton("X");
    closeDetailBtn->setFixedSize(20, 20);
    closeDetailBtn->setCursor(Qt::PointingHandCursor);
    closeDetailBtn->setStyleSheet(R"(
        QPushButton { 
            color: #ffffff; 
            font-weight: bold; 
            font-size: 10px;
            border: none; 
            background-color: #e06c75; 
            border-radius: 10px;
            padding: 0px;
            margin: 0px;
        } 
        QPushButton:hover { 
            background-color: #be5046; 
        }
    )");
    connect(closeDetailBtn, &QPushButton::clicked, [this]() { m_detailContainer->hide(); });
    
    headerLayout->addWidget(detailTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(closeDetailBtn);
    
    // Detail View Text Edit
    m_detailView = new QPlainTextEdit();
    m_detailView->setReadOnly(true);
    m_detailView->setMaximumBlockCount(0);
    m_detailView->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_detailView->setFont(QFont("Consolas", 9));
    m_detailView->setStyleSheet(R"(
        QPlainTextEdit {
            background-color: #1e2127;
            color: #abb2bf;
            border: none;
        }
    )");
    m_detailView->setFixedHeight(100);
    
    detailLayout->addWidget(detailHeader);
    detailLayout->addWidget(m_detailView);
    
    // Hide initially
    m_detailContainer->hide();
    
    vSplitter->addWidget(m_detailContainer);

    // Table takes most of the space
    vSplitter->setStretchFactor(0, 5);
    vSplitter->setStretchFactor(1, 1);

    tabLayout->addWidget(vSplitter);

    // Connect row selection → detail panel (must be after setModel)
    connect(m_tableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &TerminalWidget::onRowSelectionChanged);
            
    // Double click shows the panel
    connect(m_tableView, &QTableView::doubleClicked, this, &TerminalWidget::onRowDoubleClicked);
}

void TerminalWidget::onRowDoubleClicked(const QModelIndex &index)
{
    updateDetailPanel(index);
    m_detailContainer->show();
}

void TerminalWidget::onSearchTextChanged(const QString &text)
{
    if (m_model) {
        m_model->setFilter(text);
        m_currentMatchRow = -1; // Reset navigation cursor on new search
        updateMatchCount();
    }
}

void TerminalWidget::onFindPrev()
{
    navigateToMatch(m_currentMatchRow, false);
}

void TerminalWidget::onFindNext()
{
    navigateToMatch(m_currentMatchRow, true);
}

void TerminalWidget::onClearClicked()
{
    clearTerminal();
    m_detailView->clear();
    m_detailContainer->hide();
}

void TerminalWidget::clearTerminal()
{
    if (m_model) {
        m_model->clearData();
    }
}

void TerminalWidget::copySelection()
{
    // If user presses Ctrl+C, pop up the context menu in the center of the table
    if (m_tableView) {
        showContextMenu(m_tableView->viewport()->rect().center());
    }
}

void TerminalWidget::showContextMenu(const QPoint &pos)
{
    if (!m_tableView || !m_model) return;
    
    QModelIndexList selectedRows = m_tableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) return;
    
    std::sort(selectedRows.begin(), selectedRows.end());
    
    QMenu menu(this);
    QAction* copyDataAction = menu.addAction("Copy Data Only");
    QAction* copyFullAction = menu.addAction("Copy Timestamp + Data");
    
    QAction* selectedAction = menu.exec(m_tableView->viewport()->mapToGlobal(pos));
    
    if (!selectedAction) return;
    
    QString clipboardText;
    for (const QModelIndex& index : selectedRows) {
        int row = index.row();
        QString data = m_model->data(m_model->index(row, 3)).toString();
        
        // Remove visual tags from copied text so it pastes cleanly
        data.replace("<CR>", "");
        data.replace("<LF>", "");
        data.replace("<TAB>", "\t");
        
        if (selectedAction == copyFullAction) {
            QString timestamp = m_model->data(m_model->index(row, 0)).toString();
            QString direction = m_model->data(m_model->index(row, 1)).toString();
            clipboardText += QString("%1 %2 %3\n").arg(timestamp).arg(direction).arg(data);
        } else {
            clipboardText += data + "\n";
        }
    }
    
    QApplication::clipboard()->setText(clipboardText);
}

QString TerminalWidget::getTerminalText() const
{
    if (!m_model) return QString();

    const QList<LogEntry>& entries = m_model->entries();
    if (entries.isEmpty()) return QString();

    QString result;
    result.reserve(entries.count() * 80); // rough pre-alloc

    for (int i = 0; i < entries.count(); ++i) {
        const LogEntry& entry = entries.at(i);
        QString line;
        if (!entry.timestamp.isEmpty()) {
            line += entry.timestamp + " ";
        }
        line += QString("[%1] ").arg(entry.direction);
        line += QString("Len:%1 ").arg(entry.length);

        // Export data column as displayed in the model
        QString dataCol = m_model->data(m_model->index(i, 3), Qt::DisplayRole).toString();
        line += dataCol;

        result += line + "\n";
    }

    return result;
}

void TerminalWidget::setBufferLimit(int limit)
{
    if (m_model) {
        m_model->setMaxRows(limit);
    }
}

void TerminalWidget::setFontSize(int size)
{
    if (m_model) {
        m_model->setFontSize(size);
    }
    if (m_tableView) {
        QFont f = m_tableView->font();
        f.setPointSize(size);
        m_tableView->setFont(f);
        m_tableView->verticalHeader()->setDefaultSectionSize(size + 14); // Adjust row height
    }
}

QString TerminalWidget::appendData(const QString& prefix, const QByteArray& data)
{
    if (!m_model) return QString();

    QString timestampStr = "";
    if (m_timestampCb->isChecked()) {
        timestampStr = QString("[%1]").arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
    }

    QString hexStr;
    QString asciiStr;

    // Parse Hex
    for (char b : data) {
        hexStr.append(QString::asprintf("%02X ", (unsigned char)b));
    }
    hexStr = hexStr.trimmed();

    // Parse ASCII (Treat as UTF-8 for international characters like ı, ş, ğ, etc.)
    QString tempStr = QString::fromUtf8(data);
    for (int i = 0; i < tempStr.length(); ++i) {
        QChar c = tempStr[i];
        if (c == '\r') {
            asciiStr.append("<CR>");
        } else if (c == '\n') {
            asciiStr.append("<LF>");
        } else if (c == '\t') {
            asciiStr.append("<TAB>");
        } else if (c.unicode() < 32 || (c.unicode() >= 0x7F && c.unicode() <= 0x9F)) {
            // Replace other unprintable control characters with dots
            asciiStr.append('.');
        } else {
            asciiStr.append(c);
        }
    }

    // Determine direction tag
    QString direction = "TX";
    if (prefix.contains("RX")) direction = "RX";
    else if (prefix.contains("Error") || prefix.contains("ERR:")) direction = "E";
    else if (prefix.contains("INFO")) direction = "I"; // For generic info messages

    LogEntry entry;
    entry.timestamp = timestampStr;
    entry.direction = direction;
    entry.length = data.size();
    entry.hexData = hexStr;
    entry.asciiData = asciiStr;
    entry.rawData = data;

    bool wasAtBottom = false;
    if (m_tableView->verticalScrollBar()->value() == m_tableView->verticalScrollBar()->maximum()) {
        wasAtBottom = true;
    }

    m_model->addEntry(entry);

    if (wasAtBottom) {
        m_tableView->scrollToBottom();
    }

    // Update match count if search is active
    if (m_searchBox && !m_searchBox->text().isEmpty()) {
        updateMatchCount();
    }

    // Return the legacy string format for logging
    return QString("%1 %2").arg(prefix).arg(QString::fromUtf8(data));
}

void TerminalWidget::navigateToMatch(int fromRow, bool forward)
{
    if (!m_model || !m_tableView) return;

    const QList<LogEntry>& entries = m_model->entries();
    int count = entries.count();
    if (count == 0) return;

    // Collect all matching row indices
    QList<int> matchRows;
    for (int i = 0; i < count; ++i) {
        if (entries.at(i).isMatch) {
            matchRows.append(i);
        }
    }

    if (matchRows.isEmpty()) return;

    int targetRow = -1;

    if (forward) {
        // Find first match strictly after fromRow
        for (int row : matchRows) {
            if (row > fromRow) {
                targetRow = row;
                break;
            }
        }
        // Wrap around to beginning
        if (targetRow == -1) {
            targetRow = matchRows.first();
        }
    } else {
        // Find last match strictly before fromRow
        for (int i = matchRows.count() - 1; i >= 0; --i) {
            if (matchRows.at(i) < fromRow) {
                targetRow = matchRows.at(i);
                break;
            }
        }
        // Wrap around to end
        if (targetRow == -1) {
            targetRow = matchRows.last();
        }
    }

    m_currentMatchRow = targetRow;
    QModelIndex idx = m_model->index(targetRow, 0);
    m_tableView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    m_tableView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void TerminalWidget::updateMatchCount()
{
    if (!m_model || !m_matchCountLabel || !m_searchBox) return;

    const QString searchText = m_searchBox->text();

    if (searchText.isEmpty()) {
        m_matchCountLabel->hide();
        m_searchBox->setStyleSheet(""); // reset border
        return;
    }

    int count = 0;
    for (const LogEntry& entry : m_model->entries()) {
        if (entry.isMatch) count++;
    }

    m_matchCountLabel->show();

    if (count == 0) {
        m_matchCountLabel->setText("No match");
        m_matchCountLabel->setStyleSheet("font-size: 11px; color: #e06c75;");
        m_searchBox->setStyleSheet("border: 1px solid #e06c75; border-radius: 3px;");
    } else {
        m_matchCountLabel->setText(QString("%1 match%2").arg(count).arg(count > 1 ? "es" : ""));
        m_matchCountLabel->setStyleSheet("font-size: 11px; color: #98c379;");
        m_searchBox->setStyleSheet("border: 1px solid #98c379; border-radius: 3px;");
    }
}

void TerminalWidget::onRowSelectionChanged(const QModelIndex &current, const QModelIndex &/*previous*/)
{
    updateDetailPanel(current);
}

void TerminalWidget::updateDetailPanel(const QModelIndex &index)
{
    if (!m_detailView || !m_model) return;

    if (!index.isValid()) {
        m_detailView->clear();
        return;
    }

    const QList<LogEntry>& entries = m_model->entries();
    int row = index.row();
    if (row < 0 || row >= entries.count()) {
        m_detailView->clear();
        return;
    }

    const LogEntry& entry = entries.at(row);

    if (m_viewModeCombo && m_viewModeCombo->currentText() == "ASCII") {
        // ASCII mode: just show the full text
        m_detailView->setPlainText(entry.asciiData);
    } else {
        // HEX / Both: show a classic hex dump
        m_detailView->setPlainText(formatHexDump(entry.rawData));
    }
}

QString TerminalWidget::formatHexDump(const QByteArray &data)
{
    if (data.isEmpty()) return QString();

    QString result;
    result.reserve((data.size() / 16 + 1) * 78);

    for (int i = 0; i < data.size(); i += 16) {
        // Offset
        result += QString("%1  ").arg(i, 4, 16, QChar('0')).toUpper();

        // Hex bytes (two groups of 8)
        for (int j = 0; j < 16; ++j) {
            if (i + j < data.size()) {
                result += QString("%1 ").arg((unsigned char)data[i + j], 2, 16, QChar('0')).toUpper();
            } else {
                result += "   ";
            }
            if (j == 7) result += " "; // extra space between groups
        }

        result += " ";

        // ASCII column
        for (int j = 0; j < 16 && i + j < data.size(); ++j) {
            char c = data[i + j];
            result += (c >= 32 && c <= 126) ? c : '.';
        }

        result += '\n';
    }

    return result;
}
