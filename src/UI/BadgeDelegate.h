#ifndef BADGEDELEGATE_H
#define BADGEDELEGATE_H

#include <QStyledItemDelegate>

class BadgeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit BadgeDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // BADGEDELEGATE_H
