#include <QEvent>
#include <QtCore/QDebug>
#include "XDFramelessHelper.h"
#include "XDFramelessHelperPrivate.h"
#include "XDCursorPosCalculator.h"

XDFramelessHelper::XDFramelessHelper(QObject *parent)
    : QObject(parent)
    , d(new XDFramelessHelperPrivate())
{
    d->m_bWidgetMovable = true;
    d->m_bWidgetResizable = true;
    d->m_bRubberBandOnMove = false;
    d->m_bRubberBandOnResize = false;
}

XDFramelessHelper::~XDFramelessHelper()
{
    QList<QWidget*> keys = d->m_widgetDataHash.keys();
    int size = keys.size();
    for (int i = 0; i < size; ++i) {
        delete d->m_widgetDataHash.take(keys[i]);
    }

    delete d;
}

void XDFramelessHelper::activateOn(QWidget *topLevelWidget, QWidget *shadowContainerWidget)
{
    if (!d->m_widgetDataHash.contains(topLevelWidget)) {
       XDWidgetData *data = new XDWidgetData(d, topLevelWidget, shadowContainerWidget);
       data->setShadowWidth(d->m_nShadowWidth);
       d->m_widgetDataHash.insert(topLevelWidget, data);
       topLevelWidget->installEventFilter(this);
    }
}

void XDFramelessHelper::removeFrom(QWidget *topLevelWidget)
{
    XDWidgetData *data = d->m_widgetDataHash.take(topLevelWidget);
    if (data) {
        topLevelWidget->removeEventFilter(this);
        delete data;
    }
}

void XDFramelessHelper::setWidgetMovable(bool movable)
{
    d->m_bWidgetMovable = movable;
}

void XDFramelessHelper::setWidgetResizable(bool resizable)
{
    d->m_bWidgetResizable = resizable;
}

void XDFramelessHelper::setRubberBandOnMove(bool movable)
{
    d->m_bRubberBandOnMove = movable;
    QList<XDWidgetData*> list = d->m_widgetDataHash.values();
    foreach (XDWidgetData *data, list) {
        data->updateRubberBandStatus();
    }
}

void XDFramelessHelper::setRubberBandOnResize(bool resizable)
{
    d->m_bRubberBandOnResize = resizable;
    QList<XDWidgetData*> list = d->m_widgetDataHash.values();
    foreach (XDWidgetData *data, list) {
        data->updateRubberBandStatus();
    }
}

void XDFramelessHelper::setBorderWidth(uint width)
{
    if (width > 0) {
        XDCursorPosCalculator::m_nBorderWidth = width;
    }
}

void XDFramelessHelper::setTitleHeight(uint height)
{
    if (height > 0) {
        XDCursorPosCalculator::m_nTitleHeight = height;
    }
}

void XDFramelessHelper::setShadowWidth(int width)
{
    if (width >= 0) {
        d->m_nShadowWidth = width;
        for (auto key : d->m_widgetDataHash.keys()) {
            d->m_widgetDataHash.value(key)->setShadowWidth(d->m_nShadowWidth);
        }
    }
}

bool XDFramelessHelper::widgetResizable() const
{
    return d->m_bWidgetResizable;
}

bool XDFramelessHelper::widgetMoable() const
{
    return d->m_bWidgetMovable;
}

bool XDFramelessHelper::rubberBandOnMove() const
{
    return d->m_bRubberBandOnMove;
}

bool XDFramelessHelper::rubberBandOnResize() const
{
    return d->m_bRubberBandOnResize;
}

uint XDFramelessHelper::borderWidth() const
{
    return XDCursorPosCalculator::m_nBorderWidth;
}

uint XDFramelessHelper::titleHeight() const
{
    return XDCursorPosCalculator::m_nTitleHeight;
}

bool XDFramelessHelper::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseMove:
    case QEvent::HoverMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::Leave: {
        XDWidgetData *data = d->m_widgetDataHash.value(static_cast<QWidget *>(watched));
        if (data) {
            data->handleWidgetEvent(event);
            return true;
        }
    }
    default:
        return QObject::eventFilter(watched, event);
    }
}
