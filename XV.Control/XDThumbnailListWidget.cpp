
#include "XDThumbnailListWidget.h"
#include "XDFlowLayout.h"
#include "XDThumbnailLabel.h"
#include "ui_XDThumbnailListWidget.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QEvent>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QPixmap>
#include <QResizeEvent>
#include <QtCore/QTimer>
#include <QMimeData>
#include <QDrag>

#include <QtWidgets/QPushButton>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QScroller>

#include <iostream>

class XDThumbnailListWidgetPrivate
	: public Ui_XDThumbnailListWidget
{
	Q_DECLARE_PUBLIC(XDThumbnailListWidget);
public:
	XDThumbnailListWidgetPrivate(XDThumbnailListWidget* parent);

	/// Initialize
	void init();

	void clearAllThumbnails();
	void deleteThumbnail(int currentIndex);
	void addThumbnail(XDThumbnailLabel* thumbnail);
	void InsertThumbnail(XDThumbnailLabel* thumbnail, int index);
	void updateScrollAreaContentWidgetSize(QSize size);
	QString currentThumbnailLabel();

protected:
	XDThumbnailListWidget* const q_ptr;

	int CurrentThumbnail;
	bool isFiltrationMouseEvent;
	QSize ThumbnailSize;
	bool RequestRelayout;

private:
	Q_DISABLE_COPY(XDThumbnailListWidgetPrivate);
};



XDThumbnailListWidgetPrivate
::XDThumbnailListWidgetPrivate(XDThumbnailListWidget* parent)
  : q_ptr(parent)
  , CurrentThumbnail(-1)
  ,isFiltrationMouseEvent(false)
  , ThumbnailSize(-1, -1)
  , RequestRelayout(false)
{
}

void XDThumbnailListWidgetPrivate::init()
{
  Q_Q(XDThumbnailListWidget);

  this->setupUi(q);
  XDFlowLayout* flowLayout = new XDFlowLayout;

  //flowLayout->setOrientation(Qt::Vertical);
  //flowLayout->setPreferredExpandingDirections(Qt::Horizontal);
  flowLayout->setHorizontalSpacing(0);
  flowLayout->setVerticalSpacing(0);
  flowLayout->setContentsMargins(0, 0, 0, 0);

  this->ScrollAreaContentWidget->setLayout(flowLayout);

  this->ScrollArea->installEventFilter(q);
  QScroller::grabGesture(this->ScrollArea, QScroller::LeftMouseButtonGesture);

}

//----------------------------------------------------------------------------
void XDThumbnailListWidgetPrivate::clearAllThumbnails()
{
  Q_Q(XDThumbnailListWidget);

  QLayoutItem* item;
  while((item = this->ScrollAreaContentWidget->layout()->takeAt(0)))
    {
    XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(item->widget());
    if(thumbnailWidget)
      {
      q->disconnect(thumbnailWidget, SIGNAL(selected(XDThumbnailLabel)), q, SLOT(onThumbnailSelected(XDThumbnailLabel)));
      q->disconnect(thumbnailWidget, SIGNAL(doubleClicked(XDThumbnailLabel)), q, SLOT(doubleThumbnailClicked(XDThumbnailLabel)));
      }
    item->widget()->deleteLater();
    }
}

//------------------------------------------------------------------------------
void XDThumbnailListWidgetPrivate::deleteThumbnail(int currentIndex)
{
	Q_Q(XDThumbnailListWidget);

	QLayoutItem* item = this->ScrollAreaContentWidget->layout()->takeAt(currentIndex);
	if (item == NULL)
		return;
	XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(item->widget());
	if (thumbnailWidget)
	{
		q->disconnect(thumbnailWidget, SIGNAL(selected(XDThumbnailLabel)), q, SLOT(onThumbnailSelected(XDThumbnailLabel)));
		//q->disconnect(thumbnailWidget, SIGNAL(selected(XDThumbnailLabel)), q, SIGNAL(selected(XDThumbnailLabel)));
		q->disconnect(thumbnailWidget, SIGNAL(doubleClicked(XDThumbnailLabel)), q, SLOT(doubleThumbnailClicked(XDThumbnailLabel)));
	}
	item->widget()->deleteLater();
}

//----------------------------------------------------------------------------
void XDThumbnailListWidgetPrivate::addThumbnail(XDThumbnailLabel* thumbnail)
{
	Q_Q(XDThumbnailListWidget);

	this->ScrollAreaContentWidget->layout()->addWidget(thumbnail);
	thumbnail->installEventFilter(q);
	this->RequestRelayout = true;

	q->connect(thumbnail, SIGNAL(selected(XDThumbnailLabel)),q, SLOT(onThumbnailSelected(XDThumbnailLabel)));
	//q->connect(thumbnail, SIGNAL(selected(XDThumbnailLabel)),q, SIGNAL(selected(XDThumbnailLabel)));
	q->connect(thumbnail, SIGNAL(doubleClicked(XDThumbnailLabel)),q, SLOT(doubleThumbnailClicked(XDThumbnailLabel)));
}
void XDThumbnailListWidgetPrivate::InsertThumbnail(XDThumbnailLabel* thumbnail,int index)
{
	Q_Q(XDThumbnailListWidget);

	int num = this->ScrollAreaContentWidget->layout()->count();
	if ( num >= index)
	{
		QLayoutItem* item;
		QList<XDThumbnailLabel*> list;
		while ((item = this->ScrollAreaContentWidget->layout()->takeAt(index)))
		{
			XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(item->widget());
			//if (thumbnailWidget)
			//{
			//	q->disconnect(thumbnailWidget, SIGNAL(selected(XDThumbnailLabel)), q, SLOT(onThumbnailSelected(XDThumbnailLabel)));
			//	//q->disconnect(thumbnailWidget, SIGNAL(selected(XDThumbnailLabel)), q, SIGNAL(selected(XDThumbnailLabel)));
			//	q->disconnect(thumbnailWidget, SIGNAL(doubleClicked(XDThumbnailLabel)), q, SLOT(doubleThumbnailClicked(XDThumbnailLabel)));
			//}
			//item->widget()->deleteLater();

			list << thumbnailWidget;
		}
		//for (int i = index; i < num; i++)
		//{
		//	this->ScrollAreaContentWidget->layout()->itemAt(i);
		//}
		////QLayoutItem* item;
		////while ((item = this->ScrollAreaContentWidget->layout()->takeAt(index)))
		////{
		////	XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(item->widget());
		////	if (thumbnailWidget)
		////	{
		////		q->disconnect(thumbnailWidget, SIGNAL(selected(XDThumbnailLabel)), q, SLOT(onThumbnailSelected(XDThumbnailLabel)));
		////		//q->disconnect(thumbnailWidget, SIGNAL(selected(XDThumbnailLabel)), q, SIGNAL(selected(XDThumbnailLabel)));
		////		q->disconnect(thumbnailWidget, SIGNAL(doubleClicked(XDThumbnailLabel)), q, SLOT(doubleThumbnailClicked(XDThumbnailLabel)));
		////	}
		////	item->widget()->deleteLater();
		////}

		this->ScrollAreaContentWidget->layout()->addWidget(thumbnail);
		thumbnail->installEventFilter(q);
		this->RequestRelayout = true;
		q->connect(thumbnail, SIGNAL(selected(XDThumbnailLabel)), q, SLOT(onThumbnailSelected(XDThumbnailLabel)));
		//q->connect(thumbnail, SIGNAL(selected(XDThumbnailLabel)),q, SIGNAL(selected(XDThumbnailLabel)));
		q->connect(thumbnail, SIGNAL(doubleClicked(XDThumbnailLabel)), q, SLOT(doubleThumbnailClicked(XDThumbnailLabel)));
		
		//for(int k = 0;k<tempScrollAreaContentWidget->layout()->count();k++)
		//{
		//	QLayoutItem* item = tempScrollAreaContentWidget->layout()->takeAt(k);
		//	XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(item->widget());
		//	if (thumbnailWidget)
		//	{
		//		this->ScrollAreaContentWidget->layout()->addItem(item);
		//	}
		//}

        for (XDThumbnailLabel* var : list)
        {
            this->ScrollAreaContentWidget->layout()->addWidget(var);
            var->installEventFilter(q);
            this->RequestRelayout = true;

            q->connect(var, SIGNAL(selected(XDThumbnailLabel)), q, SLOT(onThumbnailSelected(XDThumbnailLabel)));
            // q->connect(thumbnail, SIGNAL(selected(XDThumbnailLabel)), q, SIGNAL(selected(XDThumbnailLabel)));
            q->connect(var, SIGNAL(doubleClicked(XDThumbnailLabel)), q, SLOT(doubleThumbnailClicked(XDThumbnailLabel)));
        }

		list.clear();
	}
	else
	{
		this->ScrollAreaContentWidget->layout()->addWidget(thumbnail);
		thumbnail->installEventFilter(q);
		this->RequestRelayout = true;

		q->connect(thumbnail, SIGNAL(selected(XDThumbnailLabel)), q, SLOT(onThumbnailSelected(XDThumbnailLabel)));
		//q->connect(thumbnail, SIGNAL(selected(XDThumbnailLabel)),q, SIGNAL(selected(XDThumbnailLabel)));
		q->connect(thumbnail, SIGNAL(doubleClicked(XDThumbnailLabel)), q, SLOT(doubleThumbnailClicked(XDThumbnailLabel)));
	}
}
//----------------------------------------------------------------------------
void XDThumbnailListWidgetPrivate::updateScrollAreaContentWidgetSize(QSize size)
{
  QSize mvNewViewportSize = size - QSize(2 * this->ScrollArea->lineWidth(),
                                       2 * this->ScrollArea->lineWidth());

  XDFlowLayout* flowLayout = qobject_cast<XDFlowLayout*>(
    this->ScrollAreaContentWidget->layout());
  mvNewViewportSize = mvNewViewportSize.expandedTo(flowLayout->minimumSize());
  if (flowLayout->hasHeightForWidth())
    {
    int mvNewViewPortHeight = mvNewViewportSize.height();
    mvNewViewportSize.rheight() = flowLayout->heightForWidth( mvNewViewportSize.width() );
    if (mvNewViewportSize.height() > mvNewViewPortHeight)
      {
      mvNewViewportSize.rwidth() -= this->ScrollArea->verticalScrollBar()->sizeHint().width();
      mvNewViewportSize.rheight() = flowLayout->heightForWidth( mvNewViewportSize.width() );
      }
    }
  else if (flowLayout->hasWidthForHeight())
    {
    int mvNewViewPortWidth = mvNewViewportSize.width();
    mvNewViewportSize.rwidth() = flowLayout->widthForHeight( mvNewViewportSize.height() );
    if (mvNewViewportSize.width() > mvNewViewPortWidth)
      {
      mvNewViewportSize.rheight() -= this->ScrollArea->horizontalScrollBar()->sizeHint().height();
      mvNewViewportSize.rwidth() = flowLayout->widthForHeight( mvNewViewportSize.height() );
      }
    }
  this->ScrollAreaContentWidget->resize(mvNewViewportSize);
}

QString XDThumbnailListWidgetPrivate::currentThumbnailLabel()
{
	QString label = "";
	int index = this->CurrentThumbnail;
	if (CurrentThumbnail == -1)
	{
		return label;
	}
	QLayoutItem* item = this->ScrollAreaContentWidget->layout()->itemAt(index);
	if (item != nullptr)
	{
		XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(item->widget());
		label = thumbnailWidget->text();
	}
	return label;
}


XDThumbnailListWidget::XDThumbnailListWidget(QWidget* _parent):
  Superclass(_parent),
  d_ptr(new XDThumbnailListWidgetPrivate(this))
{
  Q_D(XDThumbnailListWidget);

  d->init();
}

XDThumbnailListWidget
::XDThumbnailListWidget(XDThumbnailListWidgetPrivate *_ptr, QWidget *_parent)
  : Superclass(_parent)
  , d_ptr(_ptr)
{
  Q_D(XDThumbnailListWidget);

  d->init();
}

XDThumbnailListWidget::~XDThumbnailListWidget()
{
  Q_D(XDThumbnailListWidget);

  d->clearAllThumbnails();
}

void XDThumbnailListWidget::addThumbnails(const QList<QPixmap>& thumbnails)
{
  for(int i=0; i<thumbnails.count(); i++)
    {
    this->addThumbnail(thumbnails[i]);
    }
}

void XDThumbnailListWidget::addThumbnail(const QPixmap& pixmap, const QString& label)
{
	Q_D(XDThumbnailListWidget);

	XDThumbnailLabel* widget = new XDThumbnailLabel(d->ScrollAreaContentWidget);
	QStringList lists;
	if (label.contains("|"))
	{
		lists = label.split("|");
		widget->setText(lists[0]);
		widget->setPath(lists[1]);
	}
	else
		widget->setText(label);

	widget->setTextPosition(Qt::AlignBottom);
	if (d->ThumbnailSize.isValid())
	{
		widget->setFixedSize(d->ThumbnailSize);
	}
	widget->setPixmap(pixmap);

	d->addThumbnail(widget);
}

void XDThumbnailListWidget::insertThumbnail(const QPixmap& pixmap, const QString& label, int index)
{
	Q_D(XDThumbnailListWidget);
	XDThumbnailLabel* widget = new XDThumbnailLabel(d->ScrollAreaContentWidget);
	QStringList lists;
	if (label.contains("|"))
	{
		lists = label.split("|");
		widget->setText(lists[0]);
		widget->setPath(lists[1]);
	}
	else
		widget->setText(label);
	widget->setTextPosition(Qt::AlignBottom);
	if (d->ThumbnailSize.isValid())
	{
		widget->setFixedSize(d->ThumbnailSize);
	}
	widget->setPixmap(pixmap);
	d->InsertThumbnail(widget, index);
}

void XDThumbnailListWidget::updateThumbnail(const QPixmap& pixmap, QString label,int index)
{
	Q_D(XDThumbnailListWidget);
	int len = d->ScrollAreaContentWidget->layout()->count();
	if (index < len && index >= 0)
	{
		XDThumbnailLabel* widget = qobject_cast<XDThumbnailLabel*>(d->ScrollAreaContentWidget->layout()->itemAt(index)->widget());
		QStringList lists;
		if (label.contains("|"))
		{
			lists = label.split("|");
			widget->setText(lists[0]);
			widget->setPath(lists[1]);
		}
		else
			widget->setText(label);

		widget->setPixmap(pixmap);

	}

	//if (d->ThumbnailSize.isValid())
	//{
	//	widget->setFixedSize(d->ThumbnailSize);
	//}
	//d->addThumbnail(widget);

}

void XDThumbnailListWidget::setCurrentThumbnail(int index)
{
  Q_D(XDThumbnailListWidget);

  int count = d->ScrollAreaContentWidget->layout()->count();

  if(index >= count)return;

  for(int i=0; i<count; i++)
    {
    XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(d->ScrollAreaContentWidget->layout()->itemAt(i)->widget());
    if(i == index)
      {
      thumbnailWidget->setSelected(true);
      }
    else
      {
      thumbnailWidget->setSelected(false);
      }
    }

  d->CurrentThumbnail = index;
}



int XDThumbnailListWidget::currentThumbnail()
{
  Q_D(XDThumbnailListWidget);

  return d->CurrentThumbnail;
}

QString XDThumbnailListWidget::currentThumbnailLabel()
{
	Q_D(XDThumbnailListWidget);
	return d->currentThumbnailLabel();
}

void XDThumbnailListWidget::deleteThumbnail(int currentIndex)
{
	Q_D(XDThumbnailListWidget);
	d->deleteThumbnail(currentIndex);
}


void XDThumbnailListWidget::onThumbnailSelected(const XDThumbnailLabel &widget)
{
	Q_D(XDThumbnailListWidget);
	int len = d->ScrollAreaContentWidget->layout()->count();
	for (int i = 0; i<len; i++)
	{
		XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(d->ScrollAreaContentWidget->layout()->itemAt(i)->widget());
		if (thumbnailWidget && (&widget != thumbnailWidget))
		{
			thumbnailWidget->setSelected(false);
		}
		else
		{
			setCurrentThumbnail(i);
			emit emitCurrentThunbnailChange(i);
		}
	}
}
void XDThumbnailListWidget::doubleThumbnailClicked(const XDThumbnailLabel& widget)
{
	Q_D(XDThumbnailListWidget);
	int len = d->ScrollAreaContentWidget->layout()->count();
	for (int i = 0; i<len; i++)
	{
		XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(d->ScrollAreaContentWidget->layout()->itemAt(i)->widget());
		if (thumbnailWidget && (&widget != thumbnailWidget))
		{
			thumbnailWidget->setSelected(false);
		}
		else
		{
			setCurrentThumbnail(i);
			emit emitDoubleThunbnailChange(i);
		}
	}
}
void XDThumbnailListWidget::onThumbnailNotSelected()
{
	Q_D(XDThumbnailListWidget);

	for (int i = 0; i < d->ScrollAreaContentWidget->layout()->count(); i++)
	{
		XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(d->ScrollAreaContentWidget->layout()->itemAt(i)->widget());
		thumbnailWidget->setSelected(false);
	}
	d->CurrentThumbnail = -1;

}
QStringList XDThumbnailListWidget::GetAllThumbnail()
{
	Q_D(XDThumbnailListWidget);

	QStringList list;
	for (int i = 0; i < d->ScrollAreaContentWidget->layout()->count(); i++)
	{
		XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(d->ScrollAreaContentWidget->layout()->itemAt(i)->widget());
		list.append(thumbnailWidget->text());
	}
	return list;
}
QString XDThumbnailListWidget::GetLastThumbnailLable()
{
	Q_D(XDThumbnailListWidget);

    QString ret = "0";
	if(d->ScrollAreaContentWidget->layout()->count()> 0)
	{
		XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(d->ScrollAreaContentWidget->layout()->itemAt(d->ScrollAreaContentWidget->layout()->count()-1)->widget());
		ret = thumbnailWidget->text();
	}
	return ret;
}

void XDThumbnailListWidget::setFlow(Qt::Orientation flow)
{
  Q_D(XDThumbnailListWidget);
  XDFlowLayout* flowLayout = qobject_cast<XDFlowLayout*>(
    d->ScrollAreaContentWidget->layout());
  flowLayout->setOrientation(flow);
}


Qt::Orientation XDThumbnailListWidget::flow()const
{
  Q_D(const XDThumbnailListWidget);
  XDFlowLayout* flowLayout = qobject_cast<XDFlowLayout*>(
    d->ScrollAreaContentWidget->layout());
  return flowLayout->orientation();
}


void XDThumbnailListWidget::setThumbnailSize(QSize size)
{
  Q_D(XDThumbnailListWidget);
  if (size.isValid())
    {
    foreach( XDThumbnailLabel* thumbnail,
             this->findChildren<XDThumbnailLabel*>())
      {
      thumbnail->setFixedSize(size);
      }
    }
  d->ThumbnailSize = size;
}


QSize XDThumbnailListWidget::thumbnailSize()const
{
  Q_D(const XDThumbnailListWidget);
  return d->ThumbnailSize;
}


void XDThumbnailListWidget::clearThumbnails()
{
  Q_D(XDThumbnailListWidget);
  d->clearAllThumbnails();
}
void XDThumbnailListWidget::setFiltrationMouseEvent(bool va)
{
	Q_D(XDThumbnailListWidget);
	d->isFiltrationMouseEvent = va;
}

void XDThumbnailListWidget::resizeEvent(QResizeEvent* event)
{
  Q_D(XDThumbnailListWidget);
  d->updateScrollAreaContentWidgetSize(event->size());
}


bool XDThumbnailListWidget::eventFilter(QObject* watched, QEvent* event)
{
  Q_D(XDThumbnailListWidget);
  if (d->RequestRelayout &&  qobject_cast<XDThumbnailLabel*>(watched) &&  event->type() == QEvent::Show)
  {
	  d->RequestRelayout = false;
	  watched->removeEventFilter(this);
	  QTimer::singleShot(0, this, SLOT(updateLayout()));
  }
  if (d->isFiltrationMouseEvent)
  {
	  if (event->type() == QEvent::MouseButtonPress)
	  {
		  QMouseEvent *mouseEvent = (QMouseEvent *)event;
		  if (mouseEvent->buttons()&Qt::LeftButton)
		  {
			  return true;
		  }
	  }
	  if (event->type() == QEvent::MouseButtonDblClick)
	  {
		  return true;
	  }
  }
  return this->Superclass::eventFilter(watched, event);
}


void XDThumbnailListWidget::updateLayout()
{
  Q_D(XDThumbnailListWidget);
  d->updateScrollAreaContentWidgetSize(this->size());
}

void XDThumbnailListWidget::mousePressEvent(QMouseEvent *e)
{
	if (e->button() == Qt::LeftButton)
	{
		QWidget::mousePressEvent(e);
	}
}

void XDThumbnailListWidget::mouseMoveEvent(QMouseEvent *e)
{
	Q_D(XDThumbnailListWidget);
	QMimeData* mimdata = new QMimeData();
	int index = d->CurrentThumbnail;

	QLayoutItem* item = d->ScrollAreaContentWidget->layout()->itemAt(index);
	QDrag* drag = new QDrag(this);

	if (item != nullptr)
	{
		XDThumbnailLabel* thumbnailWidget = qobject_cast<XDThumbnailLabel*>(item->widget());
		const QPixmap* label = thumbnailWidget->pixmap();
		QByteArray qba;
		QByteArray encoded;
		QDataStream stream(&encoded, QIODevice::WriteOnly);
		stream << thumbnailWidget->path();

		mimdata->setData("ImagePath", encoded);
		mimdata->setImageData(*label);
		drag->setPixmap(*label);
		drag->setHotSpot(QPoint(drag->pixmap().width() / 2, drag->pixmap().height() / 2));
		drag->setMimeData(mimdata);
		drag->exec(Qt::CopyAction);
		QWidget::mouseMoveEvent(e);
	}

}
