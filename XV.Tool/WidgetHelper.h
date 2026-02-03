#ifndef WIDGETHELPER_H
#define WIDGETHELPER_H

#include <QDebug>
#include <QWidget>

class WidgetHelper
{
public:
    WidgetHelper();
    ~WidgetHelper();

    void IncreaseAllChildWidgetsFontSize(QWidget *parent, int increment);
    void IncreaseWidgetFontSize(QWidget *widget, int increment);
};

#endif // WIDGETHELPER_H
