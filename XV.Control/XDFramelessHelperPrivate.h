#ifndef XDFRAMELESSHELPERPRIVATE_H
#define XDFRAMELESSHELPERPRIVATE_H

#include <QHash>
#include <QtWidgets/QWidget>
#include "XDWidgetData.h"

/**
 * @brief The FramelessHelperPrivate class
 *  存储界面对应的数据集合，以及是否可移动、可缩放属性
 */
class XDFramelessHelperPrivate
{
public:
    QHash<QWidget*, XDWidgetData*> m_widgetDataHash;
    int m_nShadowWidth;
    bool m_bWidgetMovable        : true;
    bool m_bWidgetResizable      : true;
    bool m_bRubberBandOnResize   : true;
    bool m_bRubberBandOnMove     : true;
};

#endif // XDFRAMELESSHELPERPRIVATE_H
