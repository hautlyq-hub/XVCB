#ifndef SLIDERSTYLE_H
#define SLIDERSTYLE_H

#include <QRect>
#include <QtWidgets/QCommonStyle> // 改为 QCommonStyle

class SliderStyle : public QCommonStyle // 继承 QCommonStyle
{
    Q_OBJECT

public:
    SliderStyle(QStyle *baseStyle = nullptr);

    // 只需要重写你需要的方法
    void drawComplexControl(ComplexControl which,
                            const QStyleOptionComplex *option,
                            QPainter *painter,
                            const QWidget *widget = nullptr) const override;

    int styleHint(StyleHint hint,
                  const QStyleOption *option,
                  const QWidget *widget,
                  QStyleHintReturn *returnData) const override;

    int pixelMetric(PixelMetric metric,
                    const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;

    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;

    // 自定义方法
    void setValue(int value);
    int value() const;

private:
    int mValue = 0;
};

#endif // SLIDERSTYLE_H
