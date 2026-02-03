#include "mvexitappdialog.h"

#include <QPainterPath>

mvExitAppDialog::mvExitAppDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	Qt::WindowFlags flags = Qt::Dialog;

	flags |= Qt::FramelessWindowHint;

	setWindowFlags(flags);

	initWidget();

}

mvExitAppDialog::~mvExitAppDialog()
{

}
void mvExitAppDialog::paintEvent(QPaintEvent *event)
{

    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.addRect(3, 3, width() - 6, height() - 6);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, QBrush(QColor(255, 255, 255, 255)));
    QColor color(0, 0, 0);
    for (int i = 0; i<3; i++)
    {
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.addRect(3 - i, 3 - i, this->width() - (3 - i) * 2, this->height() - (3 - i) * 2);
        color.setAlpha(150 - qSqrt(i) * 50);
        painter.setPen(color);
        painter.drawPath(path);
    }
}

void mvExitAppDialog::initWidget()
{
	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(onCancellationClicked()));
	connect(ui.pushButton_2, SIGNAL(clicked()), this, SLOT(onExitClicked()));
	connect(ui.pushButton_3, SIGNAL(clicked()), this, SLOT(onCancleClicked()));
}

void  mvExitAppDialog::onCancellationClicked()
{
	flag = 1;
	done(Rejected);
}
void mvExitAppDialog::onExitClicked()
{
	flag = 2;
	done(Accepted);
}
void mvExitAppDialog::onCancleClicked()
{
	flag = 0;
	done(Rejected);
}

void mvExitAppDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
	//Q_UNUSED(event);
	//this->isMaximized() ? this->showNormal() : this->showMaximized();
	//event->accept();
}
void mvExitAppDialog::mousePressEvent(QMouseEvent *event)
{
	Q_UNUSED(event);

	m_isPressed = true;
	m_startMovePos = event->globalPos();

	event->accept();
}

void mvExitAppDialog::mouseMoveEvent(QMouseEvent *event)
{
	if (m_isPressed)
	{
		QPoint movePoint = event->globalPos() - m_startMovePos;
		QPoint widgetPos = this->pos();
		m_startMovePos = event->globalPos();

		this->move(widgetPos.x() + movePoint.x(), widgetPos.y() + movePoint.y());

		//this->parentWidget()->pos();
		//this->parentWidget()->move(widgetPos.x() + movePoint.x(), widgetPos.y() + movePoint.y());
	}

	event->accept();
}

void mvExitAppDialog::mouseReleaseEvent(QMouseEvent *event)
{
	m_isPressed = false;

	event->accept();
}


