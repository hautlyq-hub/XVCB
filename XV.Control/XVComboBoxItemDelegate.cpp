// ComboBoxItemDelegate.cpp
#include "XVComboBoxItemDelegate.h"
#include <QPainter>
#include <QStyleOptionViewItem>

XVComboBoxItemDelegate::XVComboBoxItemDelegate(const QFont &baseFont, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_baseFont(baseFont)
{
    // 可选：在基础字体上调整，比如加大一点
    m_baseFont.setPointSize(m_baseFont.pointSize() + 2); // 或保持原样
    m_baseFont.setBold(true); // 按需
}

void XVComboBoxItemDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // 使用传入的字体
    opt.font = m_baseFont;

    QStyledItemDelegate::paint(painter, opt, index);
}

QSize XVComboBoxItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    // 使用基础字体计算原始大小，再调整高度
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.font = m_baseFont;

    QSize size = QStyledItemDelegate::sizeHint(opt, index);
    size.setHeight(80); // 自定义高度
    return size;
}
