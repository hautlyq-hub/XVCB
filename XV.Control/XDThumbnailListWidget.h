

#ifndef __XDTHUMBNAILLISTWIDGET_H
#define __XDTHUMBNAILLISTWIDGET_H


#include <QtWidgets/QWidget>


class Ui_XDThumbnailListWidget;
class XDThumbnailLabel;
class XDThumbnailListWidget;
class QResizeEvent;
class XDThumbnailListWidgetPrivate;
class XDThumbnailLabel;


class  XDThumbnailListWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(int currentThumbnail READ currentThumbnail WRITE setCurrentThumbnail)
  Q_PROPERTY(Qt::Orientation flow READ flow WRITE setFlow)
  Q_PROPERTY(QSize thumbnailSize READ thumbnailSize WRITE setThumbnailSize)
public:
  typedef QWidget Superclass;
  explicit XDThumbnailListWidget(QWidget* parent=0);
  virtual ~XDThumbnailListWidget();


  void addThumbnail(const QPixmap& thumbnail, const QString& label = QString());
  void insertThumbnail(const QPixmap& pixmap, const QString& label, int index);
  void updateThumbnail(const QPixmap& thumbnail, QString label, int index);


  void addThumbnails(const QList<QPixmap>& thumbnails);


  void setCurrentThumbnail(int index);


  int currentThumbnail();
  QString currentThumbnailLabel();

  void onThumbnailNotSelected();
  QStringList GetAllThumbnail();
  QString GetLastThumbnailLable();

  void clearThumbnails();
  void setFiltrationMouseEvent(bool va);
  void deleteThumbnail(int currentIndex);


  void setFlow(Qt::Orientation orientation);
  Qt::Orientation flow()const;


  QSize thumbnailSize()const;

  virtual bool eventFilter(QObject* watched, QEvent* event);

public Q_SLOTS:

void setThumbnailSize(QSize size);

Q_SIGNALS:
  void selected(const XDThumbnailLabel& widget);
  void doubleClicked(const XDThumbnailLabel& widget);
  void emitCurrentThunbnailChange(int index);
  void emitDoubleThunbnailChange(int index);

protected Q_SLOTS:
  void onThumbnailSelected(const XDThumbnailLabel& widget);
  void doubleThumbnailClicked(const XDThumbnailLabel& widget);
  void updateLayout();

protected:
  explicit XDThumbnailListWidget(XDThumbnailListWidgetPrivate* ptr, QWidget* parent=0);
  XDThumbnailListWidgetPrivate* d_ptr;
  void mousePressEvent(QMouseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);

  virtual void resizeEvent(QResizeEvent* event);

private:
  Q_DECLARE_PRIVATE(XDThumbnailListWidget);
  Q_DISABLE_COPY(XDThumbnailListWidget);

};

#endif
