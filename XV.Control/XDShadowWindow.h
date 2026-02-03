#ifndef XDSHADOWWINDOW_H
#define XDSHADOWWINDOW_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QIcon>
#include <QtCore/QDebug>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMessageBox>
#include "XDShadowWidget.h"
#include "XDTitleBar.h"
#include "XDFramelessHelper.h"

class QPainter;
class QLineEdit;
class QLabel;

/*************************************
 * ----------------------------------
 * |shadowWindow                    |
 * |  ----------------------------  |
 * |  |ShadowWidget              |  |
 * |  |  ----------------------  |  |
 * |  |  |ShadowClientWidget  |  |  |
 * |  |  | ------------------ |  |  |
 * |  |  | |TitleBar        | |  |  |
 * |  |  | ------------------ |  |  |
 * |  |  | ------------------ |  |  |
 * |  |  | |ClientWidget    | |  |  |
 * |  |  | |                | |  |  |
 * |  |  | ------------------ |  |  |
 * |  |  ----------------------  |  |
 * |  |                          |  |
 * |  ----------------------------  |
 * |                                |
 * ----------------------------------
 *************************************/
template <class Base>
class XDShadowWindow : public Base
{
public:
	XDShadowWindow(bool canResize, int shadowSize, QWidget *parent = nullptr)
		: Base(parent)
		, m_shadowSize(shadowSize)
		, m_pShadowWidget(nullptr)
		, m_pClientWidget(nullptr)
	{
		Base *pT = this;
		pT->setAttribute(Qt::WA_TranslucentBackground, true);
		pT->setWindowFlags(Qt::FramelessWindowHint);
		pT->setContentsMargins(0, 0, 0, 0);

		QVBoxLayout *pWindowLayout = new QVBoxLayout(pT);
		pWindowLayout->setContentsMargins(0, 0, 0, 0);
		pWindowLayout->setSpacing(0);

		// 边框阴影widget
		m_pShadowWidget = new XDShadowWidget(m_shadowSize, canResize, this);
		m_pShadowWidget->setContentsMargins(m_shadowSize, m_shadowSize, m_shadowSize, m_shadowSize);
		m_pShadowWidget->setAutoFillBackground(true);
		pWindowLayout->addWidget(m_pShadowWidget);

		QVBoxLayout *pRootLayout = new QVBoxLayout(m_pShadowWidget);
		pRootLayout->setContentsMargins(0, 0, 0, 0);
		pRootLayout->setSpacing(0);

		// 窗口主要区域
		QWidget *pShadowClientWidget = new QWidget(m_pShadowWidget);
		pRootLayout->addWidget(pShadowClientWidget);
		pShadowClientWidget->setAutoFillBackground(true);
		m_pShadowClientLayout = new QVBoxLayout(pShadowClientWidget);
		m_pShadowClientLayout->setContentsMargins(0, 0, 0, 0);
		m_pShadowClientLayout->setSpacing(0);

		m_titleBar = new XDTitleBar(pShadowClientWidget, this, m_pShadowWidget, canResize);
		this->installEventFilter(m_titleBar);
		m_pShadowClientLayout->addWidget(m_titleBar);
		m_titleBar->setObjectName("titleBar1");

		m_pClientWidget = new QWidget(pShadowClientWidget);
		m_pShadowClientLayout->addWidget(m_pClientWidget);

		m_pClientLayout = new QVBoxLayout;
		m_pClientWidget->setLayout(m_pClientLayout);

		m_pHelper = new XDFramelessHelper(this);
		m_pHelper->setShadowWidth(m_shadowSize);
		m_pHelper->activateOn(this, m_pShadowWidget);
		m_pHelper->setTitleHeight(m_titleBar->height());
		m_pHelper->setWidgetResizable(true);
		m_pHelper->setWidgetMovable(true);
		m_pHelper->setRubberBandOnMove(false);
		m_pHelper->setRubberBandOnResize(false);

		QObject::connect(m_titleBar, &XDTitleBar::HeightChanged, [this](const int &height) { m_pHelper->setTitleHeight(height); });
	}

public:
    QWidget* rootWidget() const { return m_pShadowWidget; }
    QWidget *clientWidget() const { return m_pClientWidget; }
    QLayout* clientLayout() const { return m_pClientLayout; }
    XDTitleBar *titleBar() const { return m_titleBar; }
    QSize oldSize() const { return m_titleBar->oldSize(); }
    void setRubberBandOnMove(bool enable) { m_pHelper->setRubberBandOnMove(enable); }
    void setRubberBandOnResize(bool enable) { m_pHelper->setRubberBandOnResize(enable); }
    void setTitleBarHeight(int height)
    {
        if (height < 0)
            return;

        m_titleBar->setFixedHeight(height);
        m_pHelper->setTitleHeight(height);
    }

    void setClientWidget(QWidget *client) {
        if (client == nullptr)
            return;

        m_pShadowClientLayout->removeWidget(m_pClientWidget);
        m_pClientWidget->deleteLater();
        m_pClientLayout->deleteLater();
        m_pClientLayout = nullptr;
        m_pClientWidget = client;
        m_pShadowClientLayout->addWidget(m_pClientWidget);
    }

    void setResizable(bool resizable) { m_pHelper->setWidgetResizable(resizable); }
    void setMovable(bool movable) {m_pHelper->setWidgetMovable(movable); }

private:
    int m_shadowSize;
    XDShadowWidget *m_pShadowWidget;
    QWidget *m_pClientWidget;
    QVBoxLayout *m_pClientLayout;
    QVBoxLayout *m_pShadowClientLayout;
    XDFramelessHelper *m_pHelper;
    XDTitleBar *m_titleBar;
};


#endif // XDSHADOWWINDOW_H
