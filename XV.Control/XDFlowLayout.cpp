

#include <QtCore/QDebug>
#include <QtWidgets/QStyle>
#include <QtWidgets/QWidget>

#include "XDFlowLayout.h"

#include <cmath>



class XDFlowLayoutPrivate
{
  Q_DECLARE_PUBLIC(XDFlowLayout);
protected:
  XDFlowLayout* const q_ptr;
public:
  XDFlowLayoutPrivate(XDFlowLayout& object);
  void init();
  void deleteAll();

  int doLayout(const QRect &rect, bool testOnly) const;
  int smartSpacing(QStyle::PixelMetric pm) const;
  QSize maxSizeHint(int* visibleItemsCount = 0)const;

  QList<QLayoutItem *> ItemList;
  Qt::Orientation Orientation;
  int HorizontalSpacing;
  int VerticalSpacing;
  bool AlignItems;
  Qt::Orientations PreferredDirections;
};


XDFlowLayoutPrivate::XDFlowLayoutPrivate(XDFlowLayout& object)
  :q_ptr(&object)
{
  this->HorizontalSpacing = -1;
  this->VerticalSpacing = -1;
  this->Orientation = Qt::Horizontal;
  this->PreferredDirections = Qt::Horizontal | Qt::Vertical;
  this->AlignItems = true;
}


void XDFlowLayoutPrivate::init()
{
}


void XDFlowLayoutPrivate::deleteAll()
{
  Q_Q(XDFlowLayout);
  foreach(QLayoutItem* item, this->ItemList)
    {
    delete item;
    }
  this->ItemList.clear();
  q->invalidate();
}


QSize XDFlowLayoutPrivate::maxSizeHint(int *visibleItemsCount)const
{
  if (visibleItemsCount)
    {
    *visibleItemsCount = 0;
    }
  QSize maxItemSize;
  foreach (QLayoutItem* item, this->ItemList)
    {
    QWidget *wid = item->widget();
    if (wid && !wid->isVisibleTo(wid->parentWidget()))
      {
      continue;
      }
    maxItemSize.rwidth() = qMax(item->sizeHint().width(), maxItemSize.width());
    maxItemSize.rheight() = qMax(item->sizeHint().height(), maxItemSize.height());
    if (visibleItemsCount)
      {
      ++*visibleItemsCount;
      }
    }
  return maxItemSize;
}


int XDFlowLayoutPrivate::doLayout(const QRect& rect, bool testOnly)const
{
  Q_Q(const XDFlowLayout);
  int left, top, right, bottom;
  q->getContentsMargins(&left, &top, &right, &bottom);
  QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
  QPoint pos = QPoint(effectiveRect.x(), effectiveRect.y());
  int length = 0;
  int max = this->Orientation == Qt::Horizontal ?
    effectiveRect.right() + 1 : effectiveRect.bottom() + 1;
  int maxX = left + right;
  int maxY = top + bottom;
  QSize maxItemSize = this->AlignItems ? this->maxSizeHint() : QSize();

  int spaceX = q->horizontalSpacing();
  int spaceY = q->verticalSpacing();
  int space = this->Orientation == Qt::Horizontal ? spaceX : spaceY;
  QLayoutItem* previousItem = NULL;
  foreach (QLayoutItem* item, this->ItemList)
    {
    QWidget *wid = item->widget();
    if (wid && wid->isHidden())
      {
      continue;
      }
    QPoint next = pos;
    QSize itemSize = this->AlignItems ? maxItemSize : item->sizeHint();
    if (this->Orientation == Qt::Horizontal)
      {
      next += QPoint(itemSize.width() + spaceX, 0);
      }
    else
      {
      next += QPoint(0, itemSize.height() + spaceY);
      }
    if (this->Orientation == Qt::Horizontal &&
        (next.x() - space > max) && length > 0)
      {

		if (!testOnly && q->alignment() == Qt::AlignJustify && previousItem)
        {
        QRect geometry = previousItem->geometry();
        geometry.adjust(0, 0, max + space - pos.x(), 0);
        previousItem->setGeometry(geometry);
        maxX = qMax(maxX, geometry.right() + right);
        }
      pos = QPoint(effectiveRect.x(), pos.y() + length + space);
      next = pos + QPoint(itemSize.width() + space, 0);
      length = 0;
      }
    else if (this->Orientation == Qt::Vertical &&
          (next.y() - space > max) && length > 0)
      {
      pos = QPoint( pos.x() + length + space, effectiveRect.y());
      next = pos + QPoint(0, itemSize.height() + space);
      length = 0;
      }

    if (!testOnly)
      {
      item->setGeometry(QRect(pos, item->sizeHint()));
      }

    maxX = qMax( maxX , pos.x() + item->sizeHint().width() + right);
    maxY = qMax( maxY , pos.y() + item->sizeHint().height() + bottom);
    pos = next;
    length = qMax(length, this->Orientation == Qt::Horizontal ?
      itemSize.height() : itemSize.width());
    previousItem = item;
    }
  return this->Orientation == Qt::Horizontal ? maxY : maxX;
}


int XDFlowLayoutPrivate::smartSpacing(QStyle::PixelMetric pm) const
{
  Q_Q(const XDFlowLayout);
  QObject* parentObject = q->parent();
  if (!parentObject)
    {
    return -1;
    }
  else if (parentObject->isWidgetType())
    {
    QWidget* parentWidget = qobject_cast<QWidget *>(parentObject);
    return parentWidget->style()->pixelMetric(pm, 0, parentWidget);
    }
  else
    {
    return static_cast<QLayout *>(parentObject)->spacing();
    }
}


XDFlowLayout::XDFlowLayout(Qt::Orientation orientation, QWidget *parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new XDFlowLayoutPrivate(*this))
{
  Q_D(XDFlowLayout);
  d->init();
  this->setOrientation(orientation);
}


XDFlowLayout::XDFlowLayout(QWidget *parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new XDFlowLayoutPrivate(*this))
{
  Q_D(XDFlowLayout);
  d->init();
}


XDFlowLayout::XDFlowLayout()
  : d_ptr(new XDFlowLayoutPrivate(*this))
{
  Q_D(XDFlowLayout);
  d->init();
}


XDFlowLayout::~XDFlowLayout()
{
  Q_D(XDFlowLayout);
  d->deleteAll();
}


void XDFlowLayout::setOrientation(Qt::Orientation orientation)
{
  Q_D(XDFlowLayout);
  d->Orientation = orientation;
  this->invalidate();
}


void XDFlowLayout::setPreferredExpandingDirections(Qt::Orientations directions)
{
  Q_D(XDFlowLayout);
  d->PreferredDirections = directions;
}


Qt::Orientations XDFlowLayout::preferredExpandingDirections()const
{
  Q_D(const XDFlowLayout);
  return d->PreferredDirections;
}
  

Qt::Orientation XDFlowLayout::orientation() const
{
  Q_D(const XDFlowLayout);
  return d->Orientation;
}


void XDFlowLayout::setHorizontalSpacing(int spacing)
{
  Q_D(XDFlowLayout);
  d->HorizontalSpacing = spacing;
  this->invalidate();
}


int XDFlowLayout::horizontalSpacing() const
{
  Q_D(const XDFlowLayout);
  if (d->HorizontalSpacing < 0)
    {
    return d->smartSpacing(QStyle::PM_MenuBarPanelWidth);
    }
  return d->HorizontalSpacing;
}


void XDFlowLayout::setVerticalSpacing(int spacing)
{
  Q_D(XDFlowLayout);
  d->VerticalSpacing = spacing;
  this->invalidate();
}


int XDFlowLayout::verticalSpacing() const
{
  Q_D(const XDFlowLayout);
  if (d->VerticalSpacing < 0)
    {
    return d->smartSpacing(QStyle::PM_MenuBarPanelWidth);
    }
  return d->VerticalSpacing;
}


void XDFlowLayout::setAlignItems(bool align)
{
  Q_D(XDFlowLayout);
  d->AlignItems = align;
  this->invalidate();
}


bool XDFlowLayout::alignItems() const
{
  Q_D(const XDFlowLayout);
  return d->AlignItems;
}


void XDFlowLayout::addItem(QLayoutItem *item)
{
  Q_D(XDFlowLayout);
  d->ItemList << item;
  this->invalidate();
}


Qt::Orientations XDFlowLayout::expandingDirections() const
{
  return Qt::Vertical | Qt::Horizontal;
}


bool XDFlowLayout::hasWidthForHeight() const
{
  Q_D(const XDFlowLayout);
  return d->Orientation == Qt::Vertical;
}


int XDFlowLayout::widthForHeight(int height) const
{
  Q_D(const XDFlowLayout);
  QRect rect(0, 0, 0, height);
  int width = d->doLayout(rect, true);
  return width;
}


bool XDFlowLayout::hasHeightForWidth() const
{
  Q_D(const XDFlowLayout);
  return d->Orientation == Qt::Horizontal;
}


int XDFlowLayout::heightForWidth(int width) const
{
  Q_D(const XDFlowLayout);
  QRect rect(0, 0, width, 0);

  if (d->AlignItems && d->Orientation == Qt::Vertical)
    {
    int itemCount;
    QSize itemSize = d->maxSizeHint(&itemCount);
    QMargins margins = this->contentsMargins();
    int realWidth = width - margins.left() - margins.right();
    int itemCountPerRow = (realWidth + this->horizontalSpacing())
      / (itemSize.width() + this->horizontalSpacing());
    int rowCount = std::ceil( static_cast<float>(itemCount) / itemCountPerRow);
    rect.setHeight(rowCount * itemSize.height() +
                   (rowCount -1) * this->verticalSpacing() +
                   margins.top() + margins.bottom());
    }
  int height = d->doLayout(rect, true);
  return height;
}


int XDFlowLayout::count() const
{
  Q_D(const XDFlowLayout);
  return d->ItemList.count();
}


QLayoutItem *XDFlowLayout::itemAt(int index) const
{
  Q_D(const XDFlowLayout);
  if (index < 0 || index >= this->count())
    {
    return 0;
    }
  return d->ItemList[index];
}


QSize XDFlowLayout::minimumSize() const
{
  Q_D(const XDFlowLayout);
  QSize size;
  foreach(QLayoutItem* item, d->ItemList)
    {
    QWidget* widget = item->widget();
    if (widget && !widget->isVisibleTo(widget->parentWidget()))
      {
      continue;
      }
    size = size.expandedTo(item->minimumSize());
    }
  int left, top, right, bottom;
  this->getContentsMargins(&left, &top, &right, &bottom);
  size += QSize(left+right, top+bottom);
  return size;
}


void XDFlowLayout::setGeometry(const QRect &rect)
{
  Q_D(XDFlowLayout);
  this->QLayout::setGeometry(rect);
  d->doLayout(rect, false);
}

// --------------------------------------------------------------------------
QSize XDFlowLayout::sizeHint() const
{
  Q_D(const XDFlowLayout);
  QSize size = QSize(0,0);
  int countX = 0;
  int countY = 0;
  QSize maxSizeHint = d->AlignItems ? d->maxSizeHint() : QSize();

  foreach (QLayoutItem* item, d->ItemList)
    {
    QWidget* widget = item->widget();
    if (widget && !widget->isVisibleTo(widget->parentWidget()))
      {
      continue;
      }
    QSize itemSize = d->AlignItems ? maxSizeHint : item->sizeHint();
    Qt::Orientation grow;
    if (d->PreferredDirections & Qt::Horizontal &&
        !(d->PreferredDirections & Qt::Vertical))
      {
      grow = Qt::Horizontal;
      }
    else if (d->PreferredDirections & Qt::Vertical &&
             !(d->PreferredDirections & Qt::Horizontal))
      {
      grow = Qt::Vertical;
      }
    else
      {
      grow = countY >= countX ? Qt::Horizontal : Qt::Vertical;
      }
    if (grow == Qt::Horizontal)
      {
      size.rwidth() += itemSize.width();
      size.rheight() = qMax(itemSize.height(), size.height());
      ++countX;
      }
    else
      {
      size.rwidth() = qMax(itemSize.width(), size.width());
      size.rheight() += itemSize.height();
      ++countY;
      }
    }

  size += QSize((countX-1) * this->horizontalSpacing(),
                (countY-1) * this->verticalSpacing());

  int left, top, right, bottom;
  this->getContentsMargins(&left, &top, &right, &bottom);
  size += QSize(left+right, top+bottom);
  return size;
}


QLayoutItem *XDFlowLayout::takeAt(int index)
{
  Q_D(XDFlowLayout);
  if (index < 0 || index >= this->count())
    {
    return 0;
    }
  QLayoutItem* item = d->ItemList.takeAt(index);
  this->invalidate();
  return item;
}


XDFlowLayout* XDFlowLayout::replaceLayout(QWidget* widget)
{
  QLayout* oldLayout = widget->layout();

  XDFlowLayout* flowLayout = new XDFlowLayout;
  bool isVerticalLayout = qobject_cast<QVBoxLayout*>(oldLayout) != 0;
  flowLayout->setPreferredExpandingDirections(
    isVerticalLayout ? Qt::Vertical : Qt::Horizontal);
  flowLayout->setAlignItems(false);
  int margins[4];
  oldLayout->getContentsMargins(&margins[0],&margins[1],&margins[2],&margins[3]);
  QLayoutItem* item = 0;
  while((item = oldLayout->takeAt(0)))
    {
    if (item->widget())
      {
      flowLayout->addWidget(item->widget());
      }
    }

  delete oldLayout;
  flowLayout->setContentsMargins(0,0,0,0);
  widget->setLayout(flowLayout);
  return flowLayout;
}
