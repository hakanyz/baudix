#include "MacroWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QAbstractItemView>

MacroWidget::MacroWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void MacroWidget::setupUI()
{
    QVBoxLayout *toolsLayout = new QVBoxLayout(this);
    toolsLayout->setSpacing(8);
    toolsLayout->setContentsMargins(8, 8, 8, 8);

    // --- Macros ---
    QLabel* macroTitle = new QLabel("Macros");
    macroTitle->setStyleSheet("color: #abb2bf; font-weight: bold;");
    toolsLayout->addWidget(macroTitle);

    // Target selector, only shown when Dual Mode is active (two ports connected).
    m_targetRow = new QWidget();
    QHBoxLayout* targetLayout = new QHBoxLayout(m_targetRow);
    targetLayout->setContentsMargins(0, 0, 0, 0);
    targetLayout->setSpacing(4);
    QLabel* targetLabel = new QLabel("Send to:");
    targetLabel->setStyleSheet("color: #abb2bf; font-size: 11px;");
    m_targetCombo = new QComboBox();
    m_targetCombo->addItem("Auto (active port)", "Auto");
    m_targetCombo->addItem("Port A", "A");
    m_targetCombo->addItem("Port B", "B");
    m_targetCombo->addItem("Both", "Both");
    targetLayout->addWidget(targetLabel);
    targetLayout->addWidget(m_targetCombo, 1);
    toolsLayout->addWidget(m_targetRow);
    m_targetRow->setVisible(false);

    m_macrosList = new QListWidget();
    m_macrosList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_macrosList->setWordWrap(true);
    // Double-click should only send the macro, never enter inline edit.
    // Editing is still reachable via the right-click "Edit" action below.
    m_macrosList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(m_macrosList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item){
        if (!item->text().isEmpty()) emit macroSendRequested(item->text());
    });
    
    connect(m_macrosList, &QListWidget::customContextMenuRequested, [this](const QPoint& pos){
        QListWidgetItem* sel = m_macrosList->currentItem();
        QMenu menu(this);
        QAction* actAdd    = menu.addAction("Add");
        QAction* actEdit   = sel ? menu.addAction("Edit")   : nullptr;
        QAction* actRemove = sel ? menu.addAction("Remove") : nullptr;
        QAction* chosen = menu.exec(m_macrosList->mapToGlobal(pos));
        if (chosen == actAdd) {
            QListWidgetItem* newItem = new QListWidgetItem("");
            newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
            m_macrosList->addItem(newItem);
            m_macrosList->editItem(newItem);
        } else if (actEdit && chosen == actEdit) {
            m_macrosList->editItem(sel);
        } else if (actRemove && chosen == actRemove) {
            delete sel;
        }
    });
    toolsLayout->addWidget(m_macrosList, 1);
    
    // Macro Action Buttons
    QHBoxLayout* macroOps = new QHBoxLayout();
    macroOps->setSpacing(4);
    
    QPushButton* btnAddMacro = new QPushButton("+ Add");
    btnAddMacro->setObjectName("smallBtn");
    connect(btnAddMacro, &QPushButton::clicked, [this](){
        QListWidgetItem* newItem = new QListWidgetItem("");
        newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
        m_macrosList->addItem(newItem);
        m_macrosList->scrollToItem(newItem);
        m_macrosList->editItem(newItem);
    });
    macroOps->addWidget(btnAddMacro);
    
    QPushButton* btnRemoveMacro = new QPushButton("- Remove");
    btnRemoveMacro->setObjectName("smallBtn");
    connect(btnRemoveMacro, &QPushButton::clicked, [this](){
        QListWidgetItem* sel = m_macrosList->currentItem();
        if (sel) delete sel;
    });
    macroOps->addWidget(btnRemoveMacro);
    
    toolsLayout->addLayout(macroOps);

    // Send Selected button
    QPushButton* macroSendBtn = new QPushButton("Send Selected");
    macroSendBtn->setObjectName("sendButton"); // Style like the main send button
    connect(macroSendBtn, &QPushButton::clicked, [this](){
        QListWidgetItem* sel = m_macrosList->currentItem();
        if (sel && !sel->text().isEmpty()) emit macroSendRequested(sel->text());
    });
    toolsLayout->addWidget(macroSendBtn);

    toolsLayout->addSpacing(8);
    toolsLayout->addStretch();
}

void MacroWidget::setDualModeVisible(bool visible)
{
    m_targetRow->setVisible(visible);
}

void MacroWidget::setActiveLabel(const QString& label)
{
    m_targetCombo->setItemText(0, QString("Auto (active: Port %1)").arg(label));
}

QString MacroWidget::targetSelection() const
{
    return m_targetCombo->currentData().toString();
}

void MacroWidget::saveSettings(QSettings& settings)
{
    settings.beginGroup("MacroWidget");
    
    int count = m_macrosList->count();
    settings.setValue("MacroCount", count);
    for (int i = 0; i < count; ++i) {
        settings.setValue(QString("Macro_%1").arg(i), m_macrosList->item(i)->text());
    }
    
    settings.endGroup();
}

void MacroWidget::loadSettings(QSettings& settings)
{
    settings.beginGroup("MacroWidget");
    
    m_macrosList->clear();
    int count = settings.value("MacroCount", 0).toInt();
    for (int i = 0; i < count; ++i) {
        QString text = settings.value(QString("Macro_%1").arg(i), "").toString();
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_macrosList->addItem(item);
    }
    
    settings.endGroup();
}
