#pragma once

#include <QBitmap>
#include <QtWidgets/QDialog>
#include <QPainter>
#include "ui_mvaboutwidget.h"

namespace Ui {
    class mvAboutWidget;
}

namespace mv
{
	class mvAboutWidget : public QDialog
	{
		Q_OBJECT

	public:
		mvAboutWidget(QWidget *parent = Q_NULLPTR);
		~mvAboutWidget();
		void paintEvent(QPaintEvent *event);
		public slots:
		void onSpecificationClicked();
	private:
		Ui::mvAboutWidget ui;

		bool isChecked = false;
	};
}
