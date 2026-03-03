// XDCableDiameterWidget.h
#ifndef XDCABLEDIAMETERWIDGET_H
#define XDCABLEDIAMETERWIDGET_H

#include <QtWidgets/QWidget>
#include <QMap>

class CableDiameterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CableDiameterWidget(QWidget *parent = nullptr);
    void reset();
    // 椭圆设置
    void setInnerEllipseCenter(const QPointF &center);
    void setInnerEllipseMinorRadius(double radius);
    void setInnerEllipseMajorRadius(double radius);
    void setOuterEllipseMinorRadius(double radius);
    void setOuterEllipseMajorRadius(double radius);

    // 坐标轴范围设置
    void setAxisRange(double range);
    void setOptimalRange();

    // 缩放控制
    void setZoomFactor(double factor);
    void zoomIn();
    void zoomOut();
    void resetZoom();

    // 自动测量设置
    void setAutoMeasurementAngles(double intervalDegrees);
    void setAutoMeasurementsEnabled(bool enabled);

    // 测量文本设置
    void setMeasurementDisplayText(int degrees, const QString& displayText);
    void setMeasurementDisplayText(double radians, const QString& displayText);
    void clearCustomDisplayTexts();

    void setWidgetVisible(bool visible);  // 自定义控制函数

signals:
    void zoomFactorChanged(double factor);

public:
    // 添加设置像素/毫米比例的方法
    void setPixelsPerMm(double pixelsPerMm);

private:
    // 添加毫米与像素转换的辅助函数
    QPointF mmToPixels(const QPointF &mmPoint) const;
    QPointF pixelsToMm(const QPointF &pixelPoint) const;

private:
    double m_pixelsPerMm; // 像素/毫米比例

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    // 椭圆参数
    QPointF m_innerEllipseCenter;
    double m_innerEllipseMinorRadius;
    double m_innerEllipseMajorRadius;
    double m_outerEllipseMinorRadius;
    double m_outerEllipseMajorRadius;

    // 测量相关
    double m_thicknessArrowAngle;
    QList<double> m_autoMeasurementAngles;
    QMap<double, QString> m_customDisplayTexts;
    bool m_autoMeasurementsEnabled;
    bool m_showMeasurementValues;

    // 坐标系相关
    int m_tickCount;
    bool m_showGrid;
    QRectF m_coordinateRect;
    QRectF m_drawingRect;
    double m_scaleFactor;
    QPointF m_centerPoint;

    // 缩放相关
    double m_originalXAxisRange;
    double m_originalYAxisRange;
    double m_xAxisRange;
    double m_yAxisRange;
    double m_zoomFactor;
    double m_minZoomFactor;
    double m_maxZoomFactor;
    QPointF m_zoomCenter;
    bool m_enableZoom;

    // 颜色
    QColor m_backgroundColor;
    QColor m_coordinateColor;
    QColor m_gridColor;
    QColor m_innerEllipseColor;
    QColor m_outerEllipseColor;
    QColor m_thicknessArrowColor;
    QColor m_crossMarkerColor;
    QColor m_tickLabelColor;

     bool m_isVisible;

    // 内部方法
    void updateDrawingParameters();
    void updateZoomParameters();
    void updateCoordinateRect();
    void calculateOptimalZoom(double axisRange);

    QPointF toScreenCoordinates(const QPointF &logicPoint) const;
    QPointF toLogicCoordinates(const QPointF &screenPoint) const;
    QPointF calculateSimpleTextOffset(double angle, const QRectF& textRect) const;

    // 绘制方法
    void drawCoordinateSystem(QPainter &painter);
    void drawTicks(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawEllipses(QPainter &painter);
    void drawThicknessArrow(QPainter &painter);
    void drawSingleMeasurementArrow(QPainter &painter, double angle, int index);
    void drawCrossMarkers(QPainter &painter);
};

#endif // XDCABLEDIAMETERWIDGET_H
