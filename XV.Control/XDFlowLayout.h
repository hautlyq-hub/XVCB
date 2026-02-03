

#ifndef __XDFLOWLAYOUT_H
#define __XDFLOWLAYOUT_H


#include <QtWidgets/QLayout>


class XDFlowLayoutPrivate;

class XDFlowLayout : public QLayout
{
  Q_OBJECT
  Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)

  Q_PROPERTY(Qt::Orientations preferredExpandingDirections READ preferredExpandingDirections WRITE setPreferredExpandingDirections)

  Q_PROPERTY(bool alignItems READ alignItems WRITE setAlignItems)

  Q_PROPERTY(int horizontalSpacing READ horizontalSpacing WRITE setHorizontalSpacing)

  Q_PROPERTY(int verticalSpacing READ verticalSpacing WRITE setVerticalSpacing)

public:
  typedef QLayout Superclass;
  explicit XDFlowLayout(Qt::Orientation orientation, QWidget* parent = 0);
  explicit XDFlowLayout(QWidget* parent);
  explicit XDFlowLayout();
  virtual ~XDFlowLayout();

  void setOrientation(Qt::Orientation orientation);
  Qt::Orientation orientation()const;

  void setPreferredExpandingDirections(Qt::Orientations directions);
  Qt::Orientations preferredExpandingDirections()const;

  int horizontalSpacing() const;
  void setHorizontalSpacing(int);

  int verticalSpacing() const;
  void setVerticalSpacing(int);

  bool alignItems()const;
  void setAlignItems(bool);

  static XDFlowLayout* replaceLayout(QWidget* widget);

  virtual bool hasWidthForHeight() const;
  virtual int widthForHeight(int) const;

  virtual void addItem(QLayoutItem *item);
  virtual Qt::Orientations expandingDirections() const;
  virtual bool hasHeightForWidth() const;
  virtual int heightForWidth(int) const;
  virtual int count() const;
  virtual QLayoutItem *itemAt(int index) const;
  virtual QSize minimumSize() const;
  virtual void setGeometry(const QRect &rect);
  virtual QSize sizeHint() const;
  virtual QLayoutItem *takeAt(int index);
  
protected:
  QScopedPointer<XDFlowLayoutPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(XDFlowLayout);
  Q_DISABLE_COPY(XDFlowLayout);
};

#endif
