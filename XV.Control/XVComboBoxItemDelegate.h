// XVComboBoxItemDelegate.h
#ifndef XVCOMBOBOXITEMDELEGATE_H
#define XVCOMBOBOXITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QFont>

class XVComboBoxItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit XVComboBoxItemDelegate(const QFont &baseFont, QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

private:
    QFont m_baseFont;
};

#endif // XVCOMBOBOXITEMDELEGATE_H