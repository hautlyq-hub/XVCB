

#ifndef __XDTHUMBNAILLABEL_H
#define __XDTHUMBNAILLABEL_H


#include <QtWidgets/QWidget>
#include <QModelIndex>



class XDThumbnailLabelPrivate;

class XDThumbnailLabel : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(Qt::Alignment textPosition READ textPosition WRITE setTextPosition)
  Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap)
  Q_PROPERTY(Qt::TransformationMode transformationMode READ transformationMode WRITE setTransformationMode)
  Q_PROPERTY(bool selected READ isSelected WRITE setSelected)
  Q_PROPERTY(QColor selectedColor READ selectedColor WRITE setSelectedColor)
public:
  typedef QWidget Superclass;
  explicit XDThumbnailLabel(QWidget* parent=0);
  virtual ~XDThumbnailLabel();

  void setText(const QString& text);
  QString text()const;

  void setPath(const QString& text);
  QString path()const;

  void setTextPosition(const Qt::Alignment& alignment);
  Qt::Alignment textPosition()const;

  void setPixmap(const QPixmap& pixmap);
  const QPixmap* pixmap()const;

  Qt::TransformationMode transformationMode()const;
  void setTransformationMode(Qt::TransformationMode mode);

  void setSelected(bool selected);
  bool isSelected()const;

  void setSelectedColor(const QColor& color);
  QColor selectedColor()const;

  virtual QSize minimumSizeHint()const;
  virtual QSize sizeHint()const;
  virtual int heightForWidth(int width)const;

protected:
  QScopedPointer<XDThumbnailLabelPrivate> d_ptr;

  virtual void paintEvent(QPaintEvent* event);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseDoubleClickEvent(QMouseEvent* event);

  virtual void resizeEvent(QResizeEvent* event);

private:
  Q_DECLARE_PRIVATE(XDThumbnailLabel);
  Q_DISABLE_COPY(XDThumbnailLabel);
  QString mPath = "";
Q_SIGNALS:
  void selected(const XDThumbnailLabel& widget);
  void doubleClicked(const XDThumbnailLabel& widget);
};

#endif
