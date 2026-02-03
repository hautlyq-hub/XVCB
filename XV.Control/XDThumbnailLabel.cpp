
#include <QtWidgets/QApplication>
#include <QColor>
#include <QPainter>

#include <QLayout>

#include "XDThumbnailLabel.h"
#include "ui_XDThumbnailLabel.h"

#include <iostream>


class XDThumbnailLabelPrivate: public Ui_XDThumbnailLabel
{
  Q_DECLARE_PUBLIC(XDThumbnailLabel);
protected:
  XDThumbnailLabel* const q_ptr;
public:
  typedef Ui_XDThumbnailLabel Superclass;


  XDThumbnailLabelPrivate(XDThumbnailLabel* parent);
  virtual ~XDThumbnailLabelPrivate();

  virtual void setupUi(QWidget* widget);

  Qt::Alignment TextPosition;
  bool SelectedFlag;
  QColor SelectedColor;
  QModelIndex SourceIndex;
  QPixmap OriginalThumbnail;
  Qt::TransformationMode TransformationMode;


  void updateThumbnail();
};


XDThumbnailLabelPrivate::XDThumbnailLabelPrivate(XDThumbnailLabel* parent)
  : q_ptr(parent)
{
  Q_Q(XDThumbnailLabel);

  this->SelectedFlag = false;
  this->SelectedColor = q->palette().color(QPalette::Highlight);
  this->TextPosition = Qt::AlignBottom | Qt::AlignHCenter;
  this->TransformationMode = Qt::FastTransformation;
}


XDThumbnailLabelPrivate::~XDThumbnailLabelPrivate()
{
}


void XDThumbnailLabelPrivate::setupUi(QWidget* widget)
{
  Q_Q(XDThumbnailLabel);

  if (q->layout()) {
      q->layout()->setSizeConstraint(QLayout::SetNoConstraint);
  }
  q->setText(QString());
}

void XDThumbnailLabelPrivate::updateThumbnail()
{
    this->PixmapLabel->setPixmap(this->OriginalThumbnail.isNull()
                                     ? QPixmap()
                                     : this->OriginalThumbnail.scaled(this->PixmapLabel->size(),
                                                                      Qt::KeepAspectRatio,
                                                                      this->TransformationMode));
}

XDThumbnailLabel::XDThumbnailLabel(QWidget* parentWidget)
    : Superclass(parentWidget)
    , d_ptr(new XDThumbnailLabelPrivate(this))
{
    Q_D(XDThumbnailLabel);

    d->setupUi(this);

    d->TextLabel->setStyleSheet("font:15pt");
}

XDThumbnailLabel::~XDThumbnailLabel()
{
}

void XDThumbnailLabel::setText(const QString& text)
{
    Q_D(XDThumbnailLabel);

    d->TextLabel->setText(text); // 这个应该可以工作
    d->TextLabel->setVisible(false);
}

QString XDThumbnailLabel::text()const
{
  Q_D(const XDThumbnailLabel);
  return d->TextLabel->text();
}

void XDThumbnailLabel::setPath(const QString &text)
{
	mPath = text;
}


QString XDThumbnailLabel::path()const
{
	return mPath;
}

void XDThumbnailLabel::setTextPosition(const Qt::Alignment& position)
{
  Q_D(XDThumbnailLabel);
  d->TextPosition = position;
  int textIndex = -1;
  for (textIndex = 0; textIndex < this->layout()->count(); ++textIndex)
    {
    if (this->layout()->itemAt(textIndex)->widget() == d->TextLabel)
      {
      break;
      }
    }
  if (textIndex > -1 && textIndex < this->layout()->count())
    {
    this->layout()->takeAt(textIndex);
    }
  int row = 1;
  int col = 1;
  QGridLayout* gridLayout = qobject_cast<QGridLayout*>(this->layout());
  if (position & Qt::AlignTop)
    {
    row = 0;
    }
  else if (position &Qt::AlignBottom)
    {
    row = 2;
    }
  else
    {
    row = 1;
    }
  if (position & Qt::AlignLeft)
    {
    col = 0;
    }
  else if (position & Qt::AlignRight)
    {
    col = 2;
    }
  else
    {
    col = 1;
    }
  if (row == 1 && col == 1)
    {
    d->TextLabel->setVisible(false);
    }
  else
    {
    gridLayout->addWidget(d->TextLabel,row, col);
    }
}


Qt::Alignment XDThumbnailLabel::textPosition()const
{
  Q_D(const XDThumbnailLabel);
  return d->TextPosition;
}


void XDThumbnailLabel::setPixmap(const QPixmap &pixmap)
{
  Q_D(XDThumbnailLabel);

  d->OriginalThumbnail = pixmap;
  d->updateThumbnail();
}


const QPixmap* XDThumbnailLabel::pixmap()const
{
  Q_D(const XDThumbnailLabel);

  return d->OriginalThumbnail.isNull() ? 0 : &(d->OriginalThumbnail);
}


Qt::TransformationMode XDThumbnailLabel::transformationMode()const
{
  Q_D(const XDThumbnailLabel);
  return d->TransformationMode;
}


void XDThumbnailLabel::setTransformationMode(Qt::TransformationMode mode)
{
  Q_D(XDThumbnailLabel);
  d->TransformationMode = mode;
  d->updateThumbnail();
}


void XDThumbnailLabel::paintEvent(QPaintEvent* event)
{
  Q_D(XDThumbnailLabel);
  this->Superclass::paintEvent(event);
  if (d->SelectedFlag && d->SelectedColor.isValid())
    {
    QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);
	QLinearGradient linear(0, 0, 135, 115);
	linear.setColorAt(0, QColor(249, 104, 174));
	linear.setColorAt(1, QColor(30, 81, 241));
	linear.setSpread(QGradient::PadSpread);
	p.setPen(QPen(QBrush(linear), 3));
    p.drawRoundedRect(QRect(0, 0, this->width() - 1, this->height() - 1), 0, 0);
    }
}


void XDThumbnailLabel::setSelected(bool flag)
{
  Q_D(XDThumbnailLabel);
  d->SelectedFlag = flag;
  this->update();
}


bool XDThumbnailLabel::isSelected()const
{
  Q_D(const XDThumbnailLabel);
  return d->SelectedFlag;
}


void XDThumbnailLabel::setSelectedColor(const QColor& color)
{
  Q_D(XDThumbnailLabel);
  d->SelectedColor = color;
  this->update();
}


QColor XDThumbnailLabel::selectedColor()const
{
  Q_D(const XDThumbnailLabel);
  return d->SelectedColor;
}


QSize XDThumbnailLabel::minimumSizeHint()const
{
  Q_D(const XDThumbnailLabel);
  if (d->TextLabel->isVisibleTo(const_cast<XDThumbnailLabel*>(this)) &&
      !d->TextLabel->text().isEmpty())
    {
    return d->TextLabel->minimumSizeHint();
    }
  return QSize();
}


QSize XDThumbnailLabel::sizeHint()const
{
  Q_D(const XDThumbnailLabel);
  return d->OriginalThumbnail.isNull() ? this->Superclass::sizeHint()
                                       : d->OriginalThumbnail.size().expandedTo(QSize(1, 1));
}


int XDThumbnailLabel::heightForWidth(int width)const
{
  Q_D(const XDThumbnailLabel);
  if (d->OriginalThumbnail.isNull() ||
      d->OriginalThumbnail.width() == 0)
    {
    return this->Superclass::heightForWidth(width);
    }
  double ratio = static_cast<double>(d->OriginalThumbnail.height()) /
    static_cast<double>(d->OriginalThumbnail.width());
  return static_cast<int>(ratio * width + 0.5);
}


void XDThumbnailLabel::mousePressEvent(QMouseEvent* event)
{
  this->Superclass::mousePressEvent(event);
  this->setSelected(true);
  emit selected(*this);
}


void XDThumbnailLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
  this->Superclass::mouseDoubleClickEvent(event);
  emit doubleClicked(*this);
}


void XDThumbnailLabel::resizeEvent(QResizeEvent *event)
{
  Q_D(XDThumbnailLabel);
  this->Superclass::resizeEvent(event);
  d->updateThumbnail();
}
