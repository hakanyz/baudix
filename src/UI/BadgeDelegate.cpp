#include "BadgeDelegate.h"
#include <QPainter>
#include <QPainterPath>

BadgeDelegate::BadgeDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void BadgeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString text = index.data(Qt::DisplayRole).toString();
    
    // Only draw badge for TX, RX, E
    if (text != "TX" && text != "RX" && text != "E") {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // --- Fill entire cell background first (fixes alternating row colors) ---
    QColor cellBg;
    if (option.state & QStyle::State_Selected) {
        cellBg = option.palette.highlight().color();
    } else {
        // Use the model's BackgroundRole if present, otherwise alternate
        QVariant bgVariant = index.data(Qt::BackgroundRole);
        if (bgVariant.isValid()) {
            cellBg = bgVariant.value<QColor>();
        } else if (option.features & QStyleOptionViewItem::Alternate) {
            cellBg = QColor("#21252b"); // alternate row color
        } else {
            cellBg = QColor("#282c34"); // normal row color
        }
    }
    painter->fillRect(option.rect, cellBg);

    // Determine badge colors
    QColor bgColor;
    QColor textColor = Qt::white;

    if (text == "TX") {
        bgColor = QColor("#98c379"); // Green
    } else if (text == "RX") {
        bgColor = QColor("#61afef"); // Blue
    } else if (text == "E") {
        bgColor = QColor("#e06c75"); // Red
    }

    // Prepare badge font
    QFont font = option.font;
    font.setBold(true);
    font.setPointSize(qMax(6, font.pointSize() - 1)); // Slightly smaller for badge, min 6pt
    
    // Calculate dynamic badge rectangle
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(text);
    int textHeight = fm.height(); // Normal height for comfortable breathing room
    
    // Comfortable padding
    int hPad = 8; 
    int vPad = 2;
    
    int badgeWidth = textWidth + hPad * 2;  
    int badgeHeight = textHeight + vPad * 2; 
    
    // Ensure absolute minimums so it stays readable at tiny fonts
    badgeWidth = qMax(badgeWidth, 24);
    badgeHeight = qMax(badgeHeight, 16);
    
    QRect badgeRect(0, 0, badgeWidth, badgeHeight);
    badgeRect.moveCenter(option.rect.center());

    // Draw rounded background (matches overall UI style)
    QPainterPath path;
    path.addRoundedRect(badgeRect, 4, 4);
    painter->fillPath(path, bgColor);

    // Draw text
    painter->setPen(textColor);
    painter->setFont(font);
    painter->drawText(badgeRect, Qt::AlignCenter, text);

    painter->restore();
}

QSize BadgeDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(qMax(size.height(), 24)); // Minimum height for rows to look spacious
    return size;
}
