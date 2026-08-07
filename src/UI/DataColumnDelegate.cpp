#include "DataColumnDelegate.h"
#include <QPainter>
#include <QApplication>

DataColumnDelegate::DataColumnDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void DataColumnDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Draw background/selection normally
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    
    painter->save();
    
    // Draw background
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    
    QString text = index.data(Qt::DisplayRole).toString();
    
    QRect textRect = opt.rect;
    textRect.adjust(5, 0, -5, 0); // Padding to match default table view padding
    painter->setClipRect(opt.rect); // Ensure text doesn't overflow cell bounds
    
    QFontMetrics fm(opt.font);
    int x = textRect.left();
    int y = textRect.top() + (textRect.height() - fm.height()) / 2 + fm.ascent();
    
    // Elide text if it's too long
    QString elidedText = fm.elidedText(text, Qt::ElideRight, textRect.width());
    
    QColor normalColor = opt.state & QStyle::State_Selected ? opt.palette.highlightedText().color() : opt.palette.text().color();
    QColor tagColor = QColor("#5c6370"); // Muted grey for control tags
    if (opt.state & QStyle::State_Selected) {
        tagColor = normalColor; // Keep same color if selected so it's readable
    }
    
    painter->setFont(opt.font);
    
    // Simple fast tokenizer for rendering
    int currentPos = 0;
    while (currentPos < elidedText.length()) {
        int nextCR = elidedText.indexOf("<CR>", currentPos);
        int nextLF = elidedText.indexOf("<LF>", currentPos);
        int nextTAB = elidedText.indexOf("<TAB>", currentPos);
        
        int nextTag = -1;
        QString tagStr = "";
        
        if (nextCR != -1 && (nextTag == -1 || nextCR < nextTag)) { nextTag = nextCR; tagStr = "<CR>"; }
        if (nextLF != -1 && (nextTag == -1 || nextLF < nextTag)) { nextTag = nextLF; tagStr = "<LF>"; }
        if (nextTAB != -1 && (nextTag == -1 || nextTAB < nextTag)) { nextTag = nextTAB; tagStr = "<TAB>"; }
        
        if (nextTag == -1) {
            // Draw remaining normal text
            QString sub = elidedText.mid(currentPos);
            painter->setPen(normalColor);
            painter->drawText(x, y, sub);
            break;
        } else {
            // Draw normal text before tag
            if (nextTag > currentPos) {
                QString sub = elidedText.mid(currentPos, nextTag - currentPos);
                painter->setPen(normalColor);
                painter->drawText(x, y, sub);
                x += fm.horizontalAdvance(sub);
            }
            
            // Draw tag in muted color
            painter->setPen(tagColor);
            painter->drawText(x, y, tagStr);
            x += fm.horizontalAdvance(tagStr);
            
            currentPos = nextTag + tagStr.length();
        }
    }
    
    painter->restore();
}
