#pragma once

#include <QMap>
#include <QInternal>
#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QMutex>
#include <QtWidgets/QDialog>
#include <QtCore/QCoreApplication>
#include <QMouseEvent>
#include <QtCore/qmath.h>
#include <QPainter>

#include "ui_mvexitappdialog.h"

class mvExitAppDialog : public QDialog
{
	Q_OBJECT
public:
	static mvExitAppDialog* getInstance()
	{
		if (m_pInstance == NULL)
		{
			QMutexLocker mlocker(&m_Mutex);  
			if (m_pInstance == NULL)
			{
				m_pInstance = new mvExitAppDialog();
			}
		}
		return m_pInstance;
	};
	class CGarbo
	{
	public:
		~CGarbo()
		{
			if (m_pInstance != NULL)
			{
				delete m_pInstance;
				m_pInstance = NULL;
			}
		}
	};
protected:
	virtual void paintEvent(QPaintEvent *event);

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);

private:
	static CGarbo m_Garbo;
	static mvExitAppDialog* m_pInstance;
	static QMutex m_Mutex;

public:
	mvExitAppDialog(QWidget *parent = Q_NULLPTR);
	~mvExitAppDialog();
	void initWidget();
public slots:
	void onCancellationClicked();
	void onExitClicked();
	void onCancleClicked();

public:
	int flag = 0;
	bool m_isPressed;
	QPoint m_startMovePos;

private:
	Ui::mvExitAppDialog ui;

};
