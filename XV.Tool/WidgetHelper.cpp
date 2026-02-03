#include "WidgetHelper.h"

WidgetHelper::WidgetHelper() {}

WidgetHelper::~WidgetHelper() {}

void WidgetHelper::IncreaseAllChildWidgetsFontSize(QWidget *parent, int increment)
{
    qDebug() << QString("name:%1").arg(parent->objectName());
    // 增大父控件自身字体
    IncreaseWidgetFontSize(parent, increment);
    // 递归处理所有子控件
    QList<QWidget *> childWidgets = parent->findChildren<QWidget *>();
    for (QWidget *child : childWidgets) {
        IncreaseWidgetFontSize(child, increment);
    }
}

void WidgetHelper::IncreaseWidgetFontSize(QWidget *widget, int increment)
{
    qDebug() << QString("name:%1").arg(widget->objectName());
    QFont font = widget->font();
    font.setPointSize(font.pointSize() + increment);
    widget->setFont(font);
}
