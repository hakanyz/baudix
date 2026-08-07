#ifndef DATACOLUMNDELEGATE_H
#define DATACOLUMNDELEGATE_H

#include <QStyledItemDelegate>

class DataColumnDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DataColumnDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // DATACOLUMNDELEGATE_H
