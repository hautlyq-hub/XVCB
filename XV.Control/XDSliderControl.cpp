#include "XDSliderControl.h"
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QTextOption>
#include <QtWidgets/QStyleOption>

SliderStyle::SliderStyle(QStyle *baseStyle)
{
    Q_UNUSED(baseStyle); // QCommonStyle 不需要基样式
}

void SliderStyle::setValue(int value)
{
    mValue = value;
}

int SliderStyle::value() const
{
    return mValue;
}

void SliderStyle::drawComplexControl(ComplexControl which,
                                     const QStyleOptionComplex *option,
                                     QPainter *painter,
                                     const QWidget *widget) const
{
    if (which == QStyle::CC_Slider) {
        if (!painter || !option)
            return;

        const QStyleOption *baseOption = static_cast<const QStyleOption *>(option);
        QRect grooveRect = baseOption->rect.adjusted(10, 10, -10, -10);

        QLinearGradient gradient(0, 0, grooveRect.width(), grooveRect.height());
        gradient.setColorAt(0.0, QColor("#414141"));
        gradient.setColorAt(1.0, QColor("#414141"));
        painter->setPen(Qt::transparent);
        painter->setBrush(gradient);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->translate(grooveRect.x(), grooveRect.y());
        QPainterPath drawtriangle;

        // 水平滑块
        drawtriangle.moveTo(0, 0);
        drawtriangle.lineTo(grooveRect.width(), 0);
        drawtriangle.lineTo(grooveRect.width(), grooveRect.height());
        drawtriangle.lineTo(grooveRect.bottomLeft());
        drawtriangle.lineTo(0, 0);

        painter->drawPath(drawtriangle);
        painter->restore();

        // 手柄
        int handleSize = grooveRect.height() - 1;
        int valueRange = 100;
        int handlePos = (mValue * (grooveRect.width() - handleSize)) / valueRange;

        QRect handleRect(handlePos, grooveRect.y(), handleSize, handleSize);

        QLinearGradient gradient2(0, 0, handleRect.width(), handleRect.height());
        gradient2.setColorAt(0.0, QColor("#616161"));
        gradient2.setColorAt(1.0, QColor("#616161"));
        painter->setPen(Qt::transparent);
        painter->setBrush(gradient2);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        painter->drawRect(handleRect);

        QFont font;
        font.setFamily("Microsoft YaHei");
        font.setPointSize(10);
        font.setCapitalization(QFont::SmallCaps);
        painter->setFont(font);

        QTextOption textOption(Qt::AlignHCenter | Qt::AlignVCenter);
        textOption.setWrapMode(QTextOption::WrapAnywhere);

        painter->setPen(QColor(255, 255, 255));
        painter->drawText(handleRect, QString::number(mValue), textOption);
        painter->restore();
    } else {
        // 调用基类实现
        QCommonStyle::drawComplexControl(which, option, painter, widget);
    }
}

int SliderStyle::styleHint(StyleHint hint,
                           const QStyleOption *option,
                           const QWidget *widget,
                           QStyleHintReturn *returnData) const
{
    if (hint == QStyle::SH_Slider_AbsoluteSetButtons) {
        return Qt::LeftButton;
    }

    return QCommonStyle::styleHint(hint, option, widget, returnData);
}

int SliderStyle::pixelMetric(PixelMetric metric,
                             const QStyleOption *option,
                             const QWidget *widget) const
{
    return QCommonStyle::pixelMetric(metric, option, widget);
}

void SliderStyle::polish(QWidget *widget)
{
    if (widget) {
        widget->setAttribute(Qt::WA_Hover, true);
    }
    QCommonStyle::polish(widget);
}

void SliderStyle::unpolish(QWidget *widget)
{
    if (widget) {
        widget->setAttribute(Qt::WA_Hover, false);
    }
    QCommonStyle::unpolish(widget);
}
