// CableDiameterWidget.cpp
#include "XDCableDiameterWidget.h"
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QtCore/QDebug>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

// 自定义角度转换函数
namespace {
    inline double degreesToRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }

    inline double radiansToDegrees(double radians) {
        return radians * 180.0 / M_PI;
    }
}

CableDiameterWidget::CableDiameterWidget(QWidget *parent)
    : QWidget(parent)
    , m_innerEllipseCenter(0.0, 0.0)
    , m_innerEllipseMinorRadius(2.0)
    , m_innerEllipseMajorRadius(3.0)
    , m_outerEllipseMinorRadius(4.0)
    , m_outerEllipseMajorRadius(5.0)
    , m_thicknessArrowAngle(M_PI / 4.0)
    , m_tickCount(10)
    , m_showGrid(true)
    , m_coordinateRect(-10.0, -10.0, 20.0, 20.0)
    , m_scaleFactor(1.0)
    , m_originalXAxisRange(10.0)
    , m_originalYAxisRange(10.0)
    , m_xAxisRange(10.0)
    , m_yAxisRange(10.0)
    , m_zoomFactor(1.0)
    , m_minZoomFactor(0.1)
    , m_maxZoomFactor(5.0)
    , m_zoomCenter(0.0, 0.0)
    , m_enableZoom(true)
    , m_showMeasurementValues(true)
    , m_autoMeasurementsEnabled(true)
{
    setAutoFillBackground(true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);
    setPalette(palette);

    // 设置颜色
    m_backgroundColor = Qt::black;
    m_coordinateColor = Qt::white;
    m_gridColor = QColor(60, 60, 60);
    m_innerEllipseColor = QColor(0, 200, 255, 200);
    m_outerEllipseColor = QColor(255, 100, 0, 150);
    m_thicknessArrowColor = Qt::yellow;
    m_crossMarkerColor = QColor(255, 50, 50);
    m_tickLabelColor = Qt::white;

    setMouseTracking(true);

}

void CableDiameterWidget::reset()
{
    m_isVisible = false;
    m_autoMeasurementAngles.clear();
    m_customDisplayTexts.clear();
    update();
}

void CableDiameterWidget::setWidgetVisible(bool visible)
{
    if (m_isVisible != visible) {
        m_isVisible = visible;
        update();  // 触发重绘
    }
}

void CableDiameterWidget::setInnerEllipseCenter(const QPointF &center)
{
    if (m_innerEllipseCenter != center) {
        m_innerEllipseCenter = center;
        m_isVisible = true;  // 设置后显示
        update();
    }
}

void CableDiameterWidget::setInnerEllipseMinorRadius(double radius)
{
    if (!qFuzzyCompare(m_innerEllipseMinorRadius, radius) && radius > 0) {
        m_innerEllipseMinorRadius = radius;
        m_isVisible = true;  // 设置后显示
        update();
    }
}

void CableDiameterWidget::setInnerEllipseMajorRadius(double radius)
{
    if (!qFuzzyCompare(m_innerEllipseMajorRadius, radius) && radius > 0) {
        m_innerEllipseMajorRadius = radius;
        update();
    }
}

void CableDiameterWidget::setOuterEllipseMinorRadius(double radius)
{
    if (!qFuzzyCompare(m_outerEllipseMinorRadius, radius) && radius > 0) {
        m_outerEllipseMinorRadius = radius;
        update();
    }
}

void CableDiameterWidget::setOuterEllipseMajorRadius(double radius)
{
    if (!qFuzzyCompare(m_outerEllipseMajorRadius, radius) && radius > 0) {
        m_outerEllipseMajorRadius = radius;
        update();
    }
}

void CableDiameterWidget::setAxisRange(double range)
{
    if (range > 0) {
        m_originalXAxisRange = range;
        m_originalYAxisRange = range;

        // 计算合适的缩放倍数
        calculateOptimalZoom(range);
        updateCoordinateRect();

        // 设置可见
        m_isVisible = true;
        update();
    }
}

void CableDiameterWidget::calculateOptimalZoom(double axisRange)
{
    double maxEllipseSize = qMax(
        qMax(m_outerEllipseMajorRadius, m_outerEllipseMinorRadius),
        qMax(m_innerEllipseMajorRadius, m_innerEllipseMinorRadius)
    );

    double maxCenterOffset = qMax(
        fabs(m_innerEllipseCenter.x()),
        fabs(m_innerEllipseCenter.y())
    );

    double totalMaxSize = maxEllipseSize + maxCenterOffset;

    if (totalMaxSize > 0) {
        double baseZoom = axisRange / totalMaxSize;
        double safetyFactor = 0.8;
        double optimalZoom = baseZoom * safetyFactor;
        optimalZoom = qBound(0.5, optimalZoom, 2.0);
        m_zoomFactor = optimalZoom;
    }
}

void CableDiameterWidget::setZoomFactor(double factor)
{
    factor = qBound(m_minZoomFactor, factor, m_maxZoomFactor);

    if (!qFuzzyCompare(m_zoomFactor, factor)) {
        m_zoomFactor = factor;
        updateZoomParameters();
        update();
    }
}

void CableDiameterWidget::zoomIn()
{
    setZoomFactor(m_zoomFactor * 1.1);
}

void CableDiameterWidget::zoomOut()
{
    setZoomFactor(m_zoomFactor * 0.9);
}

void CableDiameterWidget::resetZoom()
{
    setZoomFactor(1.0);
}

void CableDiameterWidget::updateZoomParameters()
{
    m_xAxisRange = m_originalXAxisRange / m_zoomFactor;
    m_yAxisRange = m_originalYAxisRange / m_zoomFactor;

    m_coordinateRect = QRectF(m_zoomCenter.x() - m_xAxisRange,
                              m_zoomCenter.y() - m_yAxisRange,
                              2 * m_xAxisRange,
                              2 * m_yAxisRange);
}

void CableDiameterWidget::updateCoordinateRect()
{
    updateZoomParameters();
}

void CableDiameterWidget::setAutoMeasurementAngles(double intervalDegrees)
{
    m_autoMeasurementAngles.clear();
    m_customDisplayTexts.clear();

    if (intervalDegrees > 0) {
        for (double angle = 0; angle < 360; angle += intervalDegrees) {
            m_autoMeasurementAngles.append(degreesToRadians(angle));
        }
    }
    m_isVisible = true;  // 设置后显示
    update();
}

void CableDiameterWidget::setMeasurementDisplayText(int degrees, const QString& displayText)
{
    degrees = degrees % 360;
    if (degrees < 0) degrees += 360;

    double radians = degreesToRadians(static_cast<double>(degrees));
    setMeasurementDisplayText(radians, displayText);
    m_isVisible = true;  // 设置后显示
}

void CableDiameterWidget::setMeasurementDisplayText(double radians, const QString& displayText)
{
    while (radians < 0) radians += 2 * M_PI;
    while (radians >= 2 * M_PI) radians -= 2 * M_PI;

    if (displayText.isEmpty()) {
        m_customDisplayTexts.remove(radians);
    } else {
        m_customDisplayTexts[radians] = displayText;
    }
    m_isVisible = true;  // 设置后显示
    update();
}

QPointF CableDiameterWidget::toScreenCoordinates(const QPointF &logicPoint) const
{
    if (qFuzzyIsNull(m_scaleFactor)) {
        return m_centerPoint;
    }

    double x = m_centerPoint.x() + logicPoint.x() * m_scaleFactor;
    double y = m_centerPoint.y() - logicPoint.y() * m_scaleFactor;  // Y轴反向

    return QPointF(x, y);
}

QPointF CableDiameterWidget::toLogicCoordinates(const QPointF &screenPoint) const
{
    if (qFuzzyIsNull(m_scaleFactor)) {
        return QPointF(0, 0);
    }

    double x = (screenPoint.x() - m_centerPoint.x()) / m_scaleFactor;
    double y = (m_centerPoint.y() - screenPoint.y()) / m_scaleFactor;  // Y轴反向

    return QPointF(x, y);
}

void CableDiameterWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

   // 如果不显示内容，只绘制黑色背景
   if (!m_isVisible) {
       QPainter painter(this);
       painter.fillRect(rect(), Qt::black);
       return;
   }

   QPainter painter(this);
   painter.setRenderHint(QPainter::Antialiasing, true);

   updateDrawingParameters();

   drawCoordinateSystem(painter);

   if (m_showGrid) {
       drawGrid(painter);
   }

   drawEllipses(painter);
   drawThicknessArrow(painter);
   drawCrossMarkers(painter);
}

void CableDiameterWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateDrawingParameters();
}

void CableDiameterWidget::updateDrawingParameters()
{
    if (width() <= 0 || height() <= 0) {
        return;
    }

    int margin = 40;
    m_drawingRect = QRectF(margin, margin,
                          width() - 2 * margin,
                          height() - 2 * margin);

    if (m_drawingRect.width() > m_drawingRect.height()) {
        double diff = m_drawingRect.width() - m_drawingRect.height();
        m_drawingRect.setWidth(m_drawingRect.height());
        m_drawingRect.moveLeft(m_drawingRect.left() + diff / 2);
    } else if (m_drawingRect.height() > m_drawingRect.width()) {
        double diff = m_drawingRect.height() - m_drawingRect.width();
        m_drawingRect.setHeight(m_drawingRect.width());
        m_drawingRect.moveTop(m_drawingRect.top() + diff / 2);
    }

    m_scaleFactor = m_drawingRect.width() / m_coordinateRect.width();
    m_centerPoint = m_drawingRect.center();
}

void CableDiameterWidget::drawCoordinateSystem(QPainter &painter)
{
    painter.save();

    QPen coordinatePen(m_coordinateColor);
    coordinatePen.setWidthF(1.5);
    painter.setPen(coordinatePen);

    QPointF center = toScreenCoordinates(QPointF(0, 0));
    double axisLength = qMax(m_coordinateRect.width(), m_coordinateRect.height()) / 2;

    QPointF xStart = toScreenCoordinates(QPointF(-axisLength, 0));
    QPointF xEnd = toScreenCoordinates(QPointF(axisLength, 0));
    painter.drawLine(xStart, xEnd);

    QPointF yStart = toScreenCoordinates(QPointF(0, -axisLength));
    QPointF yEnd = toScreenCoordinates(QPointF(0, axisLength));
    painter.drawLine(yStart, yEnd);

    drawTicks(painter);
    painter.restore();
}

void CableDiameterWidget::drawTicks(QPainter &painter)
{
    painter.save();

    QPen tickPen(m_coordinateColor);
    tickPen.setWidthF(1.0);
    painter.setPen(tickPen);

    QFont font = painter.font();
    font.setPointSize(7);
    painter.setFont(font);
    painter.setPen(m_tickLabelColor);

    double xTickInterval = (2 * m_xAxisRange) / m_tickCount;
    double yTickInterval = (2 * m_yAxisRange) / m_tickCount;

    // 绘制X轴刻度
    for (int i = 1; i <= m_tickCount / 2; i++) {
        double pos = i * xTickInterval;
        QPointF tickPos = toScreenCoordinates(QPointF(pos, 0));
        painter.drawLine(tickPos - QPointF(0, 5), tickPos + QPointF(0, 5));

        // 添加刻度标签（正方向）
        QString label = QString("%1").arg(pos, 0, 'f', 1);
        QRectF labelRect = QFontMetrics(font).boundingRect(label);
        painter.drawText(tickPos.x() - labelRect.width()/2,
                        tickPos.y() + labelRect.height() + 5,
                        label);

        // 负方向的刻度
        double negPos = -pos;
        QPointF negTickPos = toScreenCoordinates(QPointF(negPos, 0));
        painter.drawLine(negTickPos - QPointF(0, 5), negTickPos + QPointF(0, 5));

        // 负方向刻度标签
        painter.drawText(negTickPos.x() - labelRect.width()/2,
                        negTickPos.y() + labelRect.height() + 5,
                        QString("%1").arg(negPos, 0, 'f', 1));
    }

    // 绘制Y轴刻度
    for (int i = 1; i <= m_tickCount / 2; i++) {
        double pos = i * yTickInterval;
        QPointF tickPos = toScreenCoordinates(QPointF(0, pos));
        painter.drawLine(tickPos - QPointF(5, 0), tickPos + QPointF(5, 0));

        // 添加刻度标签（正方向）
        QString label = QString("%1").arg(pos, 0, 'f', 1);
        QRectF labelRect = QFontMetrics(font).boundingRect(label);
        painter.drawText(tickPos.x() - labelRect.width() - 8,
                        tickPos.y() + labelRect.height()/3,
                        label);

        // 负方向的刻度
        double negPos = -pos;
        QPointF negTickPos = toScreenCoordinates(QPointF(0, negPos));
        painter.drawLine(negTickPos - QPointF(5, 0), negTickPos + QPointF(5, 0));

        // 负方向刻度标签
        painter.drawText(negTickPos.x() - labelRect.width() - 8,
                        negTickPos.y() + labelRect.height()/3,
                        QString("%1").arg(negPos, 0, 'f', 1));
    }

    // 添加坐标轴标签
    QFont axisFont = painter.font();
    axisFont.setPointSize(8);
    axisFont.setBold(true);
    painter.setFont(axisFont);

    // X轴标签
    QPointF xEnd = toScreenCoordinates(QPointF(m_xAxisRange * 0.9, 0));
    painter.drawText(xEnd.x() + 10, xEnd.y() - 10, "X (mm)");

    // Y轴标签
    QPointF yEnd = toScreenCoordinates(QPointF(0, m_yAxisRange * 0.9));
    painter.drawText(yEnd.x() + 10, yEnd.y() - 10, "Y (mm)");

    painter.restore();
}

void CableDiameterWidget::drawGrid(QPainter &painter)
{
    painter.save();

    QPen gridPen(m_gridColor);
    gridPen.setStyle(Qt::DashLine);
    gridPen.setWidthF(0.5);
    painter.setPen(gridPen);

    double visibleWidth = m_coordinateRect.width();
    double visibleHeight = m_coordinateRect.height();
    double xGridInterval = visibleWidth / m_tickCount;
    double yGridInterval = visibleHeight / m_tickCount;

    for (int i = 1; i < m_tickCount; i++) {
        double xPos = m_coordinateRect.left() + i * xGridInterval;
        QPointF top = toScreenCoordinates(QPointF(xPos, m_coordinateRect.top()));
        QPointF bottom = toScreenCoordinates(QPointF(xPos, m_coordinateRect.bottom()));
        painter.drawLine(top, bottom);
    }

    for (int i = 1; i < m_tickCount; i++) {
        double yPos = m_coordinateRect.top() + i * yGridInterval;
        QPointF left = toScreenCoordinates(QPointF(m_coordinateRect.left(), yPos));
        QPointF right = toScreenCoordinates(QPointF(m_coordinateRect.right(), yPos));
        painter.drawLine(left, right);
    }

    painter.restore();
}

void CableDiameterWidget::drawEllipses(QPainter &painter)
{
    painter.save();

    // 绘制外椭圆
    QPen outerPen(m_outerEllipseColor);
    outerPen.setWidthF(2.0);
    painter.setPen(outerPen);
    painter.setBrush(QBrush(m_outerEllipseColor, Qt::Dense4Pattern));

    QPointF outerCenter = toScreenCoordinates(QPointF(0, 0));
    QSizeF outerSize(m_outerEllipseMajorRadius * 2 * m_scaleFactor,
                    m_outerEllipseMinorRadius * 2 * m_scaleFactor);
    painter.drawEllipse(outerCenter, outerSize.width()/2, outerSize.height()/2);

    // 绘制内椭圆
    QPen innerPen(m_innerEllipseColor);
    innerPen.setWidthF(2.0);
    painter.setPen(innerPen);
    painter.setBrush(QBrush(m_innerEllipseColor, Qt::SolidPattern));

    QPointF innerCenter = toScreenCoordinates(m_innerEllipseCenter);
    QSizeF innerSize(m_innerEllipseMajorRadius * 2 * m_scaleFactor,
                    m_innerEllipseMinorRadius * 2 * m_scaleFactor);
    painter.drawEllipse(innerCenter, innerSize.width()/2, innerSize.height()/2);

    // 1. 显示内椭圆中心点坐标（在右下区域，无背景）
    painter.save();
    QFont centerFont = painter.font();
    centerFont.setPointSize(8);
    centerFont.setBold(true);
    painter.setFont(centerFont);
    QPen centerTextPen(Qt::green);
    painter.setPen(centerTextPen);

    QString centerText = QString("中心: (%1, %2)")
                        .arg(m_innerEllipseCenter.x(), 0, 'f', 2)
                        .arg(m_innerEllipseCenter.y(), 0, 'f', 2);

    QFontMetrics centerMetrics(centerFont);
    QRectF centerTextRect = centerMetrics.boundingRect(centerText);

    // 计算文本位置：内椭圆中心右下区域
    // 偏移量：向右下方偏移一定距离
    double offsetX = 10;  // 向右偏移
    double offsetY = 5;   // 向下偏移

    QPointF centerTextPos = innerCenter + QPointF(offsetX, offsetY);

    // 绘制文本（无背景）
    painter.drawText(centerTextPos, centerText);
    painter.restore();

    // 2. 显示椭圆长短径信息
    painter.save();
    QFont ellipseInfoFont = painter.font();
    ellipseInfoFont.setPointSize(8);
    painter.setFont(ellipseInfoFont);

    // 设置不同颜色的文本
    QPen outerInfoPen(QColor(255, 150, 50));  // 橙色
    QPen innerInfoPen(QColor(100, 200, 255)); // 浅蓝色

    // 计算文本显示位置（在widget的右上角）
    int textX = width() - 200;
    int textY = 20;
    int lineHeight = 15;

    // 外椭圆信息
    painter.setPen(outerInfoPen);
    QString outerInfo = QString("外椭圆: 长径=%1mm, 短径=%2mm")
                       .arg(m_outerEllipseMajorRadius, 0, 'f', 2)
                       .arg(m_outerEllipseMinorRadius, 0, 'f', 2);
    painter.drawText(textX, textY, outerInfo);

    // 内椭圆信息
    painter.setPen(innerInfoPen);
    QString innerInfo = QString("内椭圆: 长径=%1mm, 短径=%2mm")
                       .arg(m_innerEllipseMajorRadius, 0, 'f', 2)
                       .arg(m_innerEllipseMinorRadius, 0, 'f', 2);
    painter.drawText(textX, textY + lineHeight, innerInfo);

    // 3. 可选：在椭圆上标记长短径方向
    // 标记外椭圆长径方向（X轴方向）
    painter.setPen(outerInfoPen);
    QPointF outerLongStart = toScreenCoordinates(QPointF(-m_outerEllipseMajorRadius, 0));
    QPointF outerLongEnd = toScreenCoordinates(QPointF(m_outerEllipseMajorRadius, 0));
    painter.drawLine(outerLongStart, outerLongEnd);

    // 在外椭圆长径中点添加标签
    QPointF outerLongMid = (outerLongStart + outerLongEnd) / 2;
    painter.drawText(outerLongMid.x() + 5, outerLongMid.y() - 5,
                    QString("长轴=%1mm").arg(m_outerEllipseMajorRadius * 2, 0, 'f', 2));

    // 标记外椭圆短径方向（Y轴方向）
    QPointF outerShortStart = toScreenCoordinates(QPointF(0, -m_outerEllipseMinorRadius));
    QPointF outerShortEnd = toScreenCoordinates(QPointF(0, m_outerEllipseMinorRadius));
    painter.drawLine(outerShortStart, outerShortEnd);

    // 在外椭圆短径中点添加标签
    QPointF outerShortMid = (outerShortStart + outerShortEnd) / 2;
    painter.drawText(outerShortMid.x() + 5, outerShortMid.y() + 15,
                    QString("短轴=%1mm").arg(m_outerEllipseMinorRadius * 2, 0, 'f', 2));

    // 标记内椭圆长径方向
    painter.setPen(innerInfoPen);
    QPointF innerLongStart = toScreenCoordinates(
        QPointF(-m_innerEllipseMajorRadius + m_innerEllipseCenter.x(), m_innerEllipseCenter.y()));
    QPointF innerLongEnd = toScreenCoordinates(
        QPointF(m_innerEllipseMajorRadius + m_innerEllipseCenter.x(), m_innerEllipseCenter.y()));
    painter.drawLine(innerLongStart, innerLongEnd);

    // 在内椭圆长径中点添加标签
    QPointF innerLongMid = (innerLongStart + innerLongEnd) / 2;
    painter.drawText(innerLongMid.x() + 5, innerLongMid.y() - 5,
                    QString("长轴=%1mm").arg(m_innerEllipseMajorRadius * 2, 0, 'f', 2));

    // 标记内椭圆短径方向
    QPointF innerShortStart = toScreenCoordinates(
        QPointF(m_innerEllipseCenter.x(), -m_innerEllipseMinorRadius + m_innerEllipseCenter.y()));
    QPointF innerShortEnd = toScreenCoordinates(
        QPointF(m_innerEllipseCenter.x(), m_innerEllipseMinorRadius + m_innerEllipseCenter.y()));
    painter.drawLine(innerShortStart, innerShortEnd);

    // 在内椭圆短径中点添加标签
    QPointF innerShortMid = (innerShortStart + innerShortEnd) / 2;
    painter.drawText(innerShortMid.x() + 5, innerShortMid.y() + 15,
                    QString("短轴=%1mm").arg(m_innerEllipseMinorRadius * 2, 0, 'f', 2));

    painter.restore();

    painter.restore();
}

void CableDiameterWidget::drawThicknessArrow(QPainter &painter)
{
    painter.save();

    if (m_autoMeasurementsEnabled && !m_autoMeasurementAngles.isEmpty()) {
        for (int i = 0; i < m_autoMeasurementAngles.size(); ++i) {
            drawSingleMeasurementArrow(painter, m_autoMeasurementAngles[i], i);
        }
    } else {
        drawSingleMeasurementArrow(painter, m_thicknessArrowAngle, 0);
    }

    painter.restore();
}

// 简化的绘制单个测量箭头函数
void CableDiameterWidget::drawSingleMeasurementArrow(QPainter &painter, double angle, int index)
{
    painter.save();

    QColor arrowColor = Qt::yellow;
    QPen arrowPen(arrowColor);
    arrowPen.setWidthF(1.5);
    painter.setPen(arrowPen);

    // 1. 计算外椭圆上的点
    double cosA = cos(angle);
    double sinA = sin(angle);

    // 外椭圆参数方程得到的点
    double outerX = m_outerEllipseMajorRadius * cosA;
    double outerY = m_outerEllipseMinorRadius * sinA;

    QPointF outerEdgePoint(outerX, outerY);
    QPointF arrowStart = toScreenCoordinates(outerEdgePoint);

    // 2. 计算内椭圆上的对应点
    double innerX = m_innerEllipseMajorRadius * cosA + m_innerEllipseCenter.x();
    double innerY = m_innerEllipseMinorRadius * sinA + m_innerEllipseCenter.y();
    QPointF innerEdgePoint(innerX, innerY);
    QPointF innerScreenPoint = toScreenCoordinates(innerEdgePoint);

    // 3. 绘制两个圆的偏离距离（连线）
    QPen deviationPen(Qt::cyan);
    deviationPen.setWidthF(1.0);
    deviationPen.setStyle(Qt::DashLine);
    painter.setPen(deviationPen);

    // 绘制从内椭圆中心到外椭圆中心的连线
    QPointF innerCenterScreen = toScreenCoordinates(m_innerEllipseCenter);
    QPointF outerCenterScreen = toScreenCoordinates(QPointF(0, 0)); // 外椭圆中心在原点
    painter.drawLine(innerCenterScreen, outerCenterScreen);

    // 在连线中点显示偏离距离
    QPointF midPoint = (innerCenterScreen + outerCenterScreen) / 2;
    double deviation = sqrt(pow(m_innerEllipseCenter.x(), 2) +
                          pow(m_innerEllipseCenter.y(), 2));
    QString deviationText = QString("偏离: %1mm").arg(deviation, 0, 'f', 2);

    QFont deviationFont = painter.font();
    deviationFont.setPointSize(8);
    painter.setFont(deviationFont);
    QPen deviationTextPen(Qt::cyan);
    painter.setPen(deviationTextPen);

    QFontMetrics deviationMetrics(deviationFont);
    QRectF deviationTextRect = deviationMetrics.boundingRect(deviationText);
    QPointF deviationTextPos = midPoint - QPointF(deviationTextRect.width()/2, -15);
    painter.drawText(deviationTextPos, deviationText);

    // 4. 切换回箭头绘制
    painter.setPen(arrowPen);

    // 计算单位径向向量
    double distance = sqrt(outerX*outerX + outerY*outerY);
    if (distance > 0) {
        double dirX = outerX / distance;
        double dirY = outerY / distance;

        // 绘制箭头（从外椭圆向外延伸）
        double arrowLength = 30.0;
        QPointF arrowEnd = arrowStart + QPointF(dirX * arrowLength, -dirY * arrowLength);

        // 绘制箭头线
        painter.drawLine(arrowStart, arrowEnd);

        // 修改箭头头部方向：指向外椭圆（起点）
        double arrowHeadLength = 10.0;
        double arrowAngle = M_PI / 6.0;

        // 箭头方向是从终点指向起点（指向外椭圆）
        QPointF arrowDirection = arrowStart - arrowEnd;
        double length = sqrt(arrowDirection.x() * arrowDirection.x() +
                            arrowDirection.y() * arrowDirection.y());

        if (length > 0) {
            arrowDirection = arrowDirection / length;

            // 在起点（外椭圆）处绘制箭头头部
            QPointF arrowLeft(
                arrowStart.x() - arrowHeadLength * (arrowDirection.x() * cos(arrowAngle) - arrowDirection.y() * sin(arrowAngle)),
                arrowStart.y() - arrowHeadLength * (arrowDirection.x() * sin(arrowAngle) + arrowDirection.y() * cos(arrowAngle))
            );

            QPointF arrowRight(
                arrowStart.x() - arrowHeadLength * (arrowDirection.x() * cos(-arrowAngle) - arrowDirection.y() * sin(-arrowAngle)),
                arrowStart.y() - arrowHeadLength * (arrowDirection.x() * sin(-arrowAngle) + arrowDirection.y() * cos(-arrowAngle))
            );

            // 绘制箭头头部（在起点处）
            painter.drawLine(arrowStart, arrowLeft);
            painter.drawLine(arrowStart, arrowRight);
        }

        // 5. 获取显示文本：优先使用自定义文本
        QString displayText;
        if (m_customDisplayTexts.contains(angle)) {
            // 直接使用设置的自定义文本（例如"1.25mm"）
            displayText = m_customDisplayTexts[angle];
        } else {
            // 如果没有自定义文本，计算默认厚度
            double innerDistance = sqrt(innerX * innerX + innerY * innerY);
            double thickness = distance - innerDistance;
            displayText = QString("%1mm").arg(thickness, 0, 'f', 2);
        }

        // 设置厚度文本样式
        QPen textPen(Qt::white);
        textPen.setWidthF(1.5);
        painter.setPen(textPen);

        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(9);
        painter.setFont(font);

        QFontMetrics metrics(painter.font());
        QRectF textRect = metrics.boundingRect(displayText);

        // 6. 计算文本位置（在箭头外面，椭圆外侧）
        QPointF textPos;

        // 标准化角度
        double normalizedAngle = angle;
        while (normalizedAngle < 0) normalizedAngle += 2 * M_PI;
        while (normalizedAngle >= 2 * M_PI) normalizedAngle -= 2 * M_PI;

        // 将角度转换为度数用于判断
        double degrees = radiansToDegrees(normalizedAngle);

        // 文本偏移距离（根据角度调整，确保在椭圆外侧）
        double textOffsetX = 0;
        double textOffsetY = 0;

        // 根据角度确定文本在椭圆外侧的位置
        if (degrees >= 315 || degrees < 45) {
            // 右侧区域：文本放在箭头终点的右侧
            textPos = arrowEnd + QPointF(10, -textRect.height()/2);
        }
        else if (degrees >= 45 && degrees < 135) {
            // 上方区域：文本放在箭头终点的上方
            textPos = arrowEnd + QPointF(-textRect.width()/2, -20);
        }
        else if (degrees >= 135 && degrees < 225) {
            // 左侧区域：文本放在箭头终点的左侧
            textPos = arrowEnd + QPointF(-textRect.width() - 10, -textRect.height()/2);
        }
        else { // 225-315度
            // 下方区域：文本放在箭头终点的下方
            textPos = arrowEnd + QPointF(-textRect.width()/2, 20);
        }

        // 可选：为自定义文本添加背景框以提高可读性
        bool isCustomText = m_customDisplayTexts.contains(angle);
        if (isCustomText) {
            // 自定义文本添加背景框
            painter.save();
            QRectF backgroundRect(textPos.x() - 2, textPos.y() - textRect.height() + 2,
                                 textRect.width() + 4, textRect.height());
            painter.setBrush(QColor(0, 0, 0, 120)); // 半透明黑色背景
            painter.setPen(Qt::NoPen);
            painter.drawRect(backgroundRect);
            painter.restore();
        }

        // 绘制厚度文本
        painter.drawText(textPos, displayText);

        // 7. 绘制角度标签（显示角度值）
        QString angleText = QString("%1°").arg(static_cast<int>(degrees));
        QPen anglePen(QColor(180, 180, 180));
        painter.setPen(anglePen);
        QFont angleFont = painter.font();
        angleFont.setPointSize(7);
        angleFont.setBold(false);
        painter.setFont(angleFont);

        QFontMetrics angleMetrics(angleFont);
        QRectF angleTextRect = angleMetrics.boundingRect(angleText);
        QPointF angleTextPos;

        // 角度文本放在箭头线中点附近，与测量文本分开
        QPointF arrowMidPoint = (arrowStart + arrowEnd) / 2;

        if (degrees >= 315 || degrees < 45) {
            // 右侧：角度文本放在箭头中点下方
            angleTextPos = arrowMidPoint + QPointF(0, angleTextRect.height() + 10);
        }
        else if (degrees >= 45 && degrees < 135) {
            // 上方：角度文本放在箭头中点右侧
            angleTextPos = arrowMidPoint + QPointF(angleTextRect.width() + 5, 0);
        }
        else if (degrees >= 135 && degrees < 225) {
            // 左侧：角度文本放在箭头中点上方
            angleTextPos = arrowMidPoint + QPointF(0, -10);
        }
        else { // 225-315度
            // 下方：角度文本放在箭头中点左侧
            angleTextPos = arrowMidPoint + QPointF(-angleTextRect.width() - 5, 0);
        }

        painter.drawText(angleTextPos, angleText);
    }

    painter.restore();
}

// 简化的文本偏移计算
QPointF CableDiameterWidget::calculateSimpleTextOffset(double angle, const QRectF& textRect) const
{
    // 标准化角度
    while (angle < 0) angle += 2 * M_PI;
    while (angle >= 2 * M_PI) angle -= 2 * M_PI;

    // 简单的四个方向
    if (angle < M_PI/4 || angle >= 7*M_PI/4) {
        // 右侧
        return QPointF(-textRect.width() - 10, -textRect.height()/2);
    }
    else if (angle >= M_PI/4 && angle < 3*M_PI/4) {
        // 上方
        return QPointF(-textRect.width()/2, 15);
    }
    else if (angle >= 3*M_PI/4 && angle < 5*M_PI/4) {
        // 左侧
        return QPointF(10, -textRect.height()/2);
    }
    else { // 225°-315°
        // 下方
        return QPointF(-textRect.width()/2, -15);
    }
}

void CableDiameterWidget::drawCrossMarkers(QPainter &painter)
{
    painter.save();

    QPen crossPen(m_crossMarkerColor);
    crossPen.setWidthF(1.5);
    painter.setPen(crossPen);

    // 坐标系中心十字
    double crossSize = 15.0;
    QPointF centerScreen = toScreenCoordinates(QPointF(0, 0));
    painter.drawLine(centerScreen - QPointF(crossSize, 0), centerScreen + QPointF(crossSize, 0));
    painter.drawLine(centerScreen - QPointF(0, crossSize), centerScreen + QPointF(0, crossSize));

    // 小椭圆中心十字
    QPointF innerCenterScreen = toScreenCoordinates(m_innerEllipseCenter);
    double smallCrossSize = 10.0;
    painter.drawLine(innerCenterScreen - QPointF(smallCrossSize, 0), innerCenterScreen + QPointF(smallCrossSize, 0));
    painter.drawLine(innerCenterScreen - QPointF(0, smallCrossSize), innerCenterScreen + QPointF(0, smallCrossSize));

    painter.restore();
}

void CableDiameterWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_enableZoom) {
        event->ignore();
        return;
    }

    // 获取鼠标位置（屏幕坐标）
    QPointF mouseScreenPos = event->position();

    // 将鼠标位置转换为逻辑坐标（作为缩放中心）
    QPointF mouseLogicPos = toLogicCoordinates(mouseScreenPos);

    // 计算缩放因子
    double zoomFactor = 1.1; // 每次滚轮缩放10%
    double delta;

    // 判断滚轮方向
    if (event->angleDelta().y() > 0) {
        // 向上滚动：放大
        delta = 1.0 / zoomFactor;
    } else {
        // 向下滚动：缩小
        delta = zoomFactor;
    }

    // 应用缩放限制
    double newZoomFactor = m_zoomFactor * delta;
    newZoomFactor = qBound(m_minZoomFactor, newZoomFactor, m_maxZoomFactor);

    // 如果缩放因子没有变化，直接返回
    if (qFuzzyCompare(newZoomFactor, m_zoomFactor)) {
        event->accept();
        return;
    }

    // 计算新的缩放中心
    // 公式: new_center = mousePos - (mousePos - old_center) * (old_zoom / new_zoom)
    QPointF oldCenter = m_zoomCenter;
    double zoomRatio = m_zoomFactor / newZoomFactor;
    QPointF newCenter(mouseLogicPos.x() - (mouseLogicPos.x() - oldCenter.x()) * zoomRatio,
                      mouseLogicPos.y() - (mouseLogicPos.y() - oldCenter.y()) * zoomRatio);

    // 应用新的缩放中心和缩放因子
    m_zoomFactor = newZoomFactor;
    m_zoomCenter = newCenter;

    // 更新坐标矩形和相关参数
    updateZoomParameters();

    // 触发重绘
    update();

    event->accept();
}
