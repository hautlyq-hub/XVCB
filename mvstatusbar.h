#pragma once

#include <QAction>
#include <QEvent>
#include <QHelpEvent>
#include <QMetaMethod>
#include <QMetaObject>
#include <QMutex>
#include <QPainter>
#include <QPixmap>
#include <QtCore/QString>
#include <QtCore/qmath.h>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QWidget>
#include <map>

#include "XV.DataAccess/mvdicomdatabase.h"

class QLabel;
class QPixmap;
class QAction;
class QToolButton;

namespace mv
{
	class StatusBar : public QWidget
	{
		Q_OBJECT

	public:
		StatusBar(QWidget *parent = Q_NULLPTR); 
		virtual ~StatusBar(); 


	private:


	};
}
