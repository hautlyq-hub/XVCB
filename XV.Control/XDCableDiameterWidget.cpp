// CableDiameterWidget.cpp 修改后的版本
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
inline double degreesToRadians(double degrees)
{
    return degrees * M_PI / 180.0;
}

inline double radiansToDegrees(double radians)
{
    return radians * 180.0 / M_PI;
}
} // namespace

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
    , m_pixelsPerMm(100) // 默认1mm = 100像素，可根据实际情况调整
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

// 添加设置像素/毫米比例的方法
void CableDiameterWidget::setPixelsPerMm(double pixelsPerMm)
{
    if (pixelsPerMm > 0 && !qFuzzyCompare(m_pixelsPerMm, pixelsPerMm)) {
        m_pixelsPerMm = pixelsPerMm;
        updateDrawingParameters();
        update();
    }
}

// 修改坐标转换函数
QPointF CableDiameterWidget::mmToPixels(const QPointF &mmPoint) const
{
    // 将毫米坐标转换为像素坐标
    // 注意：Y轴需要反转，因为屏幕Y轴向下为正，而数学坐标系Y轴向上为正
    double x = mmPoint.x() * m_pixelsPerMm;
    double y = -mmPoint.y() * m_pixelsPerMm; // Y轴反转

    return QPointF(x, y);
}

QPointF CableDiameterWidget::pixelsToMm(const QPointF &pixelPoint) const
{
    // 将像素坐标转换为毫米坐标
    double x = pixelPoint.x() / m_pixelsPerMm;
    double y = -pixelPoint.y() / m_pixelsPerMm; // Y轴反转

    return QPointF(x, y);
}

// 修改屏幕坐标转换函数（考虑视图变换）
QPointF CableDiameterWidget::toScreenCoordinates(const QPointF &logicPoint) const
{
    if (qFuzzyIsNull(m_scaleFactor)) {
        return m_centerPoint;
    }

    // logicPoint 是毫米单位
    // 1. 先转换为相对于视图中心的偏移（毫米）
    double offsetX = logicPoint.x() - m_zoomCenter.x();
    double offsetY = logicPoint.y() - m_zoomCenter.y();

    // 2. 将毫米偏移转换为像素偏移
    double pixelOffsetX = offsetX * m_pixelsPerMm;
    double pixelOffsetY = -offsetY * m_pixelsPerMm; // Y轴反转

    // 3. 应用缩放并转换为屏幕坐标
    double screenX = m_centerPoint.x() + pixelOffsetX * m_scaleFactor;
    double screenY = m_centerPoint.y() + pixelOffsetY * m_scaleFactor;

    return QPointF(screenX, screenY);
}

QPointF CableDiameterWidget::toLogicCoordinates(const QPointF &screenPoint) const
{
    if (qFuzzyIsNull(m_scaleFactor)) {
        return QPointF(0, 0);
    }

    // 1. 从屏幕坐标转换为相对于视图中心的像素偏移
    double pixelOffsetX = (screenPoint.x() - m_centerPoint.x()) / m_scaleFactor;
    double pixelOffsetY = (screenPoint.y() - m_centerPoint.y()) / m_scaleFactor;

    // 2. 将像素偏移转换为毫米偏移（注意Y轴反转）
    double mmOffsetX = pixelOffsetX / m_pixelsPerMm;
    double mmOffsetY = -pixelOffsetY / m_pixelsPerMm;

    // 3. 加上视图中心得到逻辑坐标
    return QPointF(m_zoomCenter.x() + mmOffsetX, m_zoomCenter.y() + mmOffsetY);
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
        update(); // 触发重绘
    }
}

void CableDiameterWidget::setInnerEllipseCenter(const QPointF &center)
{
    if (m_innerEllipseCenter != center) {
        m_innerEllipseCenter = center;
        m_isVisible = true; // 设置后显示
        update();
    }
}

void CableDiameterWidget::setInnerEllipseMinorRadius(double radius)
{
    if (!qFuzzyCompare(m_innerEllipseMinorRadius, radius) && radius > 0) {
        m_innerEllipseMinorRadius = radius;
        m_isVisible = true; // 设置后显示
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
    double maxEllipseSize = qMax(qMax(m_outerEllipseMajorRadius, m_outerEllipseMinorRadius),
                                 qMax(m_innerEllipseMajorRadius, m_innerEllipseMinorRadius));

    double maxCenterOffset = qMax(fabs(m_innerEllipseCenter.x()), fabs(m_innerEllipseCenter.y()));

    double totalMaxSize = maxEllipseSize + maxCenterOffset;

    if (totalMaxSize > 0) {
        double targetRange = totalMaxSize * 1.1;
        double optimalZoom = axisRange / targetRange;
        optimalZoom = qBound(1.6, optimalZoom, 5.0);
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
    m_isVisible = true; // 设置后显示
    update();
}

void CableDiameterWidget::setMeasurementDisplayText(int degrees, const QString &displayText)
{
    degrees = degrees % 360;
    if (degrees < 0)
        degrees += 360;

    double radians = degreesToRadians(static_cast<double>(degrees));
    setMeasurementDisplayText(radians, displayText);
    m_isVisible = true; // 设置后显示
}

void CableDiameterWidget::setMeasurementDisplayText(double radians, const QString &displayText)
{
    while (radians < 0)
        radians += 2 * M_PI;
    while (radians >= 2 * M_PI)
        radians -= 2 * M_PI;

    if (displayText.isEmpty()) {
        m_customDisplayTexts.remove(radians);
    } else {
        m_customDisplayTexts[radians] = displayText;
    }
    m_isVisible = true; // 设置后显示
    update();
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
    m_drawingRect = QRectF(margin, margin, width() - 2 * margin, height() - 2 * margin);

    if (m_drawingRect.width() > m_drawingRect.height()) {
        double diff = m_drawingRect.width() - m_drawingRect.height();
        m_drawingRect.setWidth(m_drawingRect.height());
        m_drawingRect.moveLeft(m_drawingRect.left() + diff / 2);
    } else if (m_drawingRect.height() > m_drawingRect.width()) {
        double diff = m_drawingRect.height() - m_drawingRect.width();
        m_drawingRect.setHeight(m_drawingRect.width());
        m_drawingRect.moveTop(m_drawingRect.top() + diff / 2);
    }

    // 修正缩放因子计算
    // m_coordinateRect.width() 是毫米单位（例如20mm）
    // 我们需要将整个毫米范围映射到绘图区域
    // 所以缩放因子 = 绘图区域像素 / (毫米范围 * 像素/毫米)
    double mmRange = m_coordinateRect.width();   // 毫米范围
    double pixelRange = mmRange * m_pixelsPerMm; // 对应的像素范围

    if (pixelRange > 0) {
        m_scaleFactor = m_drawingRect.width() / pixelRange;
    } else {
        m_scaleFactor = 1.0;
    }

    // 计算中心点（屏幕坐标）
    m_centerPoint = m_drawingRect.center();
}

void CableDiameterWidget::drawCoordinateSystem(QPainter &painter)
{
    painter.save();

    QPen coordinatePen(m_coordinateColor);
    coordinatePen.setWidthF(1.5);
    painter.setPen(coordinatePen);

    QPointF center = toScreenCoordinates(QPointF(0, 0));

    // 计算坐标轴端点（在毫米单位下）
    double axisLengthMm = qMax(m_coordinateRect.width(), m_coordinateRect.height()) / 2;

    QPointF xStart = toScreenCoordinates(QPointF(-axisLengthMm, 0));
    QPointF xEnd = toScreenCoordinates(QPointF(axisLengthMm, 0));
    painter.drawLine(xStart, xEnd);

    QPointF yStart = toScreenCoordinates(QPointF(0, -axisLengthMm));
    QPointF yEnd = toScreenCoordinates(QPointF(0, axisLengthMm));
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

    double xTickIntervalMm = (2 * m_xAxisRange) / m_tickCount;
    double yTickIntervalMm = (2 * m_yAxisRange) / m_tickCount;

    // 绘制X轴刻度
    for (int i = 1; i <= m_tickCount / 2; i++) {
        double posMm = i * xTickIntervalMm;
        QPointF tickPos = toScreenCoordinates(QPointF(posMm, 0));
        painter.drawLine(tickPos - QPointF(0, 5), tickPos + QPointF(0, 5));

        // 添加刻度标签（正方向）- 显示毫米单位
        QString label = QString("%1").arg(posMm, 0, 'f', 1);
        QRectF labelRect = QFontMetrics(font).boundingRect(label);
        painter.drawText(tickPos.x() - labelRect.width() / 2,
                         tickPos.y() + labelRect.height() + 5,
                         label);

        // 负方向的刻度
        double negPosMm = -posMm;
        QPointF negTickPos = toScreenCoordinates(QPointF(negPosMm, 0));
        painter.drawLine(negTickPos - QPointF(0, 5), negTickPos + QPointF(0, 5));

        // 负方向刻度标签
        painter.drawText(negTickPos.x() - labelRect.width() / 2,
                         negTickPos.y() + labelRect.height() + 5,
                         QString("%1").arg(negPosMm, 0, 'f', 1));
    }

    // 绘制Y轴刻度
    for (int i = 1; i <= m_tickCount / 2; i++) {
        double posMm = i * yTickIntervalMm;
        QPointF tickPos = toScreenCoordinates(QPointF(0, posMm));
        painter.drawLine(tickPos - QPointF(5, 0), tickPos + QPointF(5, 0));

        // 添加刻度标签（正方向）
        QString label = QString("%1").arg(posMm, 0, 'f', 1);
        QRectF labelRect = QFontMetrics(font).boundingRect(label);
        painter.drawText(tickPos.x() - labelRect.width() - 8,
                         tickPos.y() + labelRect.height() / 3,
                         label);

        // 负方向的刻度
        double negPosMm = -posMm;
        QPointF negTickPos = toScreenCoordinates(QPointF(0, negPosMm));
        painter.drawLine(negTickPos - QPointF(5, 0), negTickPos + QPointF(5, 0));

        // 负方向刻度标签
        painter.drawText(negTickPos.x() - labelRect.width() - 8,
                         negTickPos.y() + labelRect.height() / 3,
                         QString("%1").arg(negPosMm, 0, 'f', 1));
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

    double visibleWidthMm = m_coordinateRect.width();
    double visibleHeightMm = m_coordinateRect.height();
    double xGridIntervalMm = visibleWidthMm / m_tickCount;
    double yGridIntervalMm = visibleHeightMm / m_tickCount;

    for (int i = 1; i < m_tickCount; i++) {
        double xPosMm = m_coordinateRect.left() + i * xGridIntervalMm;
        QPointF top = toScreenCoordinates(QPointF(xPosMm, m_coordinateRect.top()));
        QPointF bottom = toScreenCoordinates(QPointF(xPosMm, m_coordinateRect.bottom()));
        painter.drawLine(top, bottom);
    }

    for (int i = 1; i < m_tickCount; i++) {
        double yPosMm = m_coordinateRect.top() + i * yGridIntervalMm;
        QPointF left = toScreenCoordinates(QPointF(m_coordinateRect.left(), yPosMm));
        QPointF right = toScreenCoordinates(QPointF(m_coordinateRect.right(), yPosMm));
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
    double outerMajorRadiusPixels = m_outerEllipseMajorRadius * m_pixelsPerMm * m_scaleFactor;
    double outerMinorRadiusPixels = m_outerEllipseMinorRadius * m_pixelsPerMm * m_scaleFactor;
    painter.drawEllipse(outerCenter, outerMajorRadiusPixels, outerMinorRadiusPixels);

    // 绘制内椭圆
    QPen innerPen(m_innerEllipseColor);
    innerPen.setWidthF(2.0);
    painter.setPen(innerPen);
    painter.setBrush(QBrush(m_innerEllipseColor, Qt::SolidPattern));

    QPointF innerCenter = toScreenCoordinates(m_innerEllipseCenter);
    double innerMajorRadiusPixels = m_innerEllipseMajorRadius * m_pixelsPerMm * m_scaleFactor;
    double innerMinorRadiusPixels = m_innerEllipseMinorRadius * m_pixelsPerMm * m_scaleFactor;
    painter.drawEllipse(innerCenter, innerMajorRadiusPixels, innerMinorRadiusPixels);

    // ==================== 文本显示区域 ====================
    int margin = 20;  // 统一的边距
    int padding = 15; // 背景内边距

    // 1. 左下角信息区 - 内圆中心 + 偏离信息
    painter.save();
    QFont leftBottomFont = painter.font();
    leftBottomFont.setPointSize(9);
    leftBottomFont.setBold(true);
    painter.setFont(leftBottomFont);

    // 计算偏离值
    double deviation = sqrt(pow(m_innerEllipseCenter.x(), 2) + pow(m_innerEllipseCenter.y(), 2));

    QStringList leftBottomTexts;
    leftBottomTexts << QString("内圆中心: (%1, %2) mm")
                           .arg(m_innerEllipseCenter.x(), 0, 'f', 3)
                           .arg(m_innerEllipseCenter.y(), 0, 'f', 3);
    leftBottomTexts << QString("偏 离 值: %1 mm").arg(deviation, 0, 'f', 3);

    // 计算文本尺寸
    QFontMetrics leftBottomFm(leftBottomFont);
    int leftBottomMaxWidth = 0;
    int leftBottomTotalHeight = 0;
    QList<int> leftBottomLineHeights;

    for (const QString &text : leftBottomTexts) {
        QRectF rect = leftBottomFm.boundingRect(text);
        leftBottomMaxWidth = qMax(leftBottomMaxWidth, static_cast<int>(rect.width()));
        leftBottomLineHeights.append(rect.height());
        leftBottomTotalHeight += rect.height();
    }
    leftBottomTotalHeight += (leftBottomTexts.size() - 1) * 5; // 行间距

    // 左下角位置
    int leftBottomX = margin;
    int leftBottomY = height() - margin - 10;

    // 背景框位置和大小
    int leftBottomBgX = leftBottomX - padding;
    int leftBottomBgY = leftBottomY - leftBottomTotalHeight - padding;
    int leftBottomBgWidth = leftBottomMaxWidth + padding * 2;
    int leftBottomBgHeight = leftBottomTotalHeight + padding * 2;

    // 添加半透明背景
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter
        .drawRoundedRect(leftBottomBgX, leftBottomBgY, leftBottomBgWidth, leftBottomBgHeight, 8, 8);

    // 绘制文本
    painter.setPen(Qt::green);
    int currentY = leftBottomY - leftBottomTotalHeight + leftBottomLineHeights[0];
    for (int i = 0; i < leftBottomTexts.size(); ++i) {
        painter.drawText(leftBottomX, currentY, leftBottomTexts[i]);
        if (i < leftBottomTexts.size() - 1) {
            currentY += leftBottomLineHeights[i] + 5;
        }
    }
    painter.restore();

    // 2. 左上角信息区 - 外椭圆信息
    painter.save();
    QFont topLeftFont = painter.font();
    topLeftFont.setPointSize(9);
    topLeftFont.setBold(true);
    painter.setFont(topLeftFont);

    QStringList topLeftTexts;
    topLeftTexts << QString("外椭圆: 长径=%1mm, 短径=%2mm")
                        .arg(m_outerEllipseMajorRadius * 2, 0, 'f', 3)
                        .arg(m_outerEllipseMinorRadius * 2, 0, 'f', 3);
    topLeftTexts << QString("外圆中心: (0, 0) mm");

    // 计算文本尺寸
    QFontMetrics topLeftFm(topLeftFont);
    int topLeftMaxWidth = 0;
    int topLeftTotalHeight = 0;
    QList<int> topLeftLineHeights;

    for (const QString &text : topLeftTexts) {
        QRectF rect = topLeftFm.boundingRect(text);
        topLeftMaxWidth = qMax(topLeftMaxWidth, static_cast<int>(rect.width()));
        topLeftLineHeights.append(rect.height());
        topLeftTotalHeight += rect.height();
    }
    topLeftTotalHeight += (topLeftTexts.size() - 1) * 5; // 行间距

    // 左上角位置
    int topLeftX = margin;
    int topLeftY = margin + topLeftTotalHeight + 20;

    // 背景框位置和大小
    int topLeftBgX = topLeftX - padding;
    int topLeftBgY = topLeftY - topLeftTotalHeight - padding;
    int topLeftBgWidth = topLeftMaxWidth + padding * 2;
    int topLeftBgHeight = topLeftTotalHeight + padding * 2;

    // 添加半透明背景
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(topLeftBgX, topLeftBgY, topLeftBgWidth, topLeftBgHeight, 8, 8);

    // 绘制文本
    painter.setPen(QColor(255, 150, 50)); // 橙色
    int currentTopLeftY = topLeftY - topLeftTotalHeight + topLeftLineHeights[0];
    for (int i = 0; i < topLeftTexts.size(); ++i) {
        painter.drawText(topLeftX, currentTopLeftY, topLeftTexts[i]);
        if (i < topLeftTexts.size() - 1) {
            currentTopLeftY += topLeftLineHeights[i] + 5;
        }
    }
    painter.restore();

    // 3. 右上角信息区 - 内椭圆信息
    painter.save();
    QFont topRightFont = painter.font();
    topRightFont.setPointSize(9);
    topRightFont.setBold(true);
    painter.setFont(topRightFont);

    QString topRightText = QString("内椭圆: 长径=%1mm, 短径=%2mm")
                               .arg(m_innerEllipseMajorRadius * 2, 0, 'f', 3)
                               .arg(m_innerEllipseMinorRadius * 2, 0, 'f', 3);

    // 计算文本尺寸
    QFontMetrics topRightFm(topRightFont);
    QRectF topRightRect = topRightFm.boundingRect(topRightText);

    // 右上角位置
    int topRightX = width() - topRightRect.width() - margin;
    int topRightY = margin + topRightRect.height() + 20;

    // 背景框位置和大小
    int topRightBgX = topRightX - padding;
    int topRightBgY = topRightY - topRightRect.height() - padding;
    int topRightBgWidth = topRightRect.width() + padding * 2;
    int topRightBgHeight = topRightRect.height() + padding * 2;

    // 添加半透明背景
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(topRightBgX, topRightBgY, topRightBgWidth, topRightBgHeight, 8, 8);

    // 绘制文本
    painter.setPen(QColor(100, 200, 255)); // 浅蓝色
    painter.drawText(topRightX, topRightY, topRightText);
    painter.restore();

    // 4. 右下角图例说明
    painter.save();
    QFont legendFont = painter.font();
    legendFont.setPointSize(8);
    painter.setFont(legendFont);

    // 定义图例项
    struct LegendItem
    {
        QString text;
        QColor color;
    };

    QList<LegendItem> legendItems;
    legendItems.append({"外椭圆 (橙色)", QColor(255, 150, 50)});
    legendItems.append({"内椭圆 (蓝色)", QColor(100, 200, 255)});
    legendItems.append({"内圆中心 (绿色)", Qt::green});
    legendItems.append({"偏离值 (绿色)", Qt::green});
    legendItems.append({"长短轴 (白色)", Qt::white});
    legendItems.append({"厚度测量 (黄色)", Qt::yellow});

    // 计算标题尺寸
    QFontMetrics titleFm(legendFont);
    QString titleText = "图例说明:";
    QRectF titleRect = titleFm.boundingRect(titleText);

    // 计算所有图例项的尺寸
    QFontMetrics legendFm(legendFont);
    int legendMaxWidth = 0;
    int legendTotalHeight = titleRect.height() + 10; // 标题加间距
    QList<int> legendLineHeights;

    for (const LegendItem &item : legendItems) {
        QRectF rect = legendFm.boundingRect(item.text);
        legendMaxWidth = qMax(legendMaxWidth, static_cast<int>(rect.width()));
        legendLineHeights.append(rect.height());
        legendTotalHeight += rect.height() + 3; // 行间距
    }

    // 右下角位置
    int legendX = width() - legendMaxWidth - margin - 20;
    int legendY = height() - margin - 20;

    // 背景框位置和大小
    int legendBgX = legendX - padding;
    int legendBgY = legendY - legendTotalHeight - padding;
    int legendBgWidth = legendMaxWidth + padding * 2 + 10;
    int legendBgHeight = legendTotalHeight + padding * 2;

    // 添加半透明背景
    painter.setBrush(QColor(0, 0, 0, 180));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(legendBgX, legendBgY, legendBgWidth, legendBgHeight, 8, 8);

    // 绘制标题
    painter.setPen(Qt::white);
    QFont titleFont = legendFont;
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(legendX, legendY - legendTotalHeight + titleRect.height() + 5, titleText);

    // 绘制图例项
    painter.setFont(legendFont);
    int currentLegendY = legendY - legendTotalHeight + titleRect.height() + 20;
    for (int i = 0; i < legendItems.size(); ++i) {
        painter.setPen(legendItems[i].color);
        painter.drawText(legendX, currentLegendY, legendItems[i].text);
        currentLegendY += legendLineHeights[i] + 5;
    }
    painter.restore();

    // ==================== 长短轴标记 ====================
    painter.save();

    QColor axisColor = Qt::white;
    axisColor.setAlpha(150);

    QPen axisPen(axisColor);
    axisPen.setWidthF(1.0);
    axisPen.setStyle(Qt::DashLine);
    painter.setPen(axisPen);

    QPointF outerLongStart = toScreenCoordinates(QPointF(-m_outerEllipseMajorRadius, 0));
    QPointF outerLongEnd = toScreenCoordinates(QPointF(m_outerEllipseMajorRadius, 0));
    painter.drawLine(outerLongStart, outerLongEnd);

    QPointF outerShortStart = toScreenCoordinates(QPointF(0, -m_outerEllipseMinorRadius));
    QPointF outerShortEnd = toScreenCoordinates(QPointF(0, m_outerEllipseMinorRadius));
    painter.drawLine(outerShortStart, outerShortEnd);

    QPointF innerLongStart = toScreenCoordinates(
        QPointF(-m_innerEllipseMajorRadius + m_innerEllipseCenter.x(), m_innerEllipseCenter.y()));
    QPointF innerLongEnd = toScreenCoordinates(
        QPointF(m_innerEllipseMajorRadius + m_innerEllipseCenter.x(), m_innerEllipseCenter.y()));
    painter.drawLine(innerLongStart, innerLongEnd);

    QPointF innerShortStart = toScreenCoordinates(
        QPointF(m_innerEllipseCenter.x(), -m_innerEllipseMinorRadius + m_innerEllipseCenter.y()));
    QPointF innerShortEnd = toScreenCoordinates(
        QPointF(m_innerEllipseCenter.x(), m_innerEllipseMinorRadius + m_innerEllipseCenter.y()));
    painter.drawLine(innerShortStart, innerShortEnd);
    painter.restore();

    // 在长短轴端点添加小圆点标记
    painter.save();
    painter.setBrush(Qt::white);
    painter.setPen(Qt::NoPen);

    painter.drawEllipse(outerLongStart, 3, 3);
    painter.drawEllipse(outerLongEnd, 3, 3);
    painter.drawEllipse(outerShortStart, 3, 3);
    painter.drawEllipse(outerShortEnd, 3, 3);
    painter.drawEllipse(innerLongStart, 3, 3);
    painter.drawEllipse(innerLongEnd, 3, 3);
    painter.drawEllipse(innerShortStart, 3, 3);
    painter.drawEllipse(innerShortEnd, 3, 3);
    painter.restore();

    painter.restore();
}

void CableDiameterWidget::setOptimalRange()
{
    // 计算所有需要显示的最大坐标值
    double maxX = 0.0;
    double maxY = 0.0;

    // 外椭圆上的点
    maxX = qMax(maxX, fabs(m_outerEllipseMajorRadius));  // 右端点
    maxX = qMax(maxX, fabs(-m_outerEllipseMajorRadius)); // 左端点
    maxY = qMax(maxY, fabs(m_outerEllipseMinorRadius));  // 上端点
    maxY = qMax(maxY, fabs(-m_outerEllipseMinorRadius)); // 下端点

    // 内椭圆上的点（考虑中心偏移）
    maxX = qMax(maxX, fabs(m_innerEllipseMajorRadius + m_innerEllipseCenter.x()));
    maxX = qMax(maxX, fabs(-m_innerEllipseMajorRadius + m_innerEllipseCenter.x()));
    maxY = qMax(maxY, fabs(m_innerEllipseMinorRadius + m_innerEllipseCenter.y()));
    maxY = qMax(maxY, fabs(-m_innerEllipseMinorRadius + m_innerEllipseCenter.y()));

    // 加上10%的边距
    double margin = 1.1;
    double requiredX = maxX * margin;
    double requiredY = maxY * margin;

    // 取较大的值，保持纵横比一致
    double requiredRange = qMax(requiredX, requiredY);

    // 设置轴全范围
    setAxisRange(requiredRange * 2);
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

// 修改后的绘制单个测量箭头函数
void CableDiameterWidget::drawSingleMeasurementArrow(QPainter &painter, double angle, int index)
{
    painter.save();

    QColor arrowColor = Qt::yellow;
    QPen arrowPen(arrowColor);
    arrowPen.setWidthF(1.5);
    painter.setPen(arrowPen);

    // 1. 计算外椭圆上的点（毫米单位）
    double cosA = cos(angle);
    double sinA = sin(angle);

    double outerX = m_outerEllipseMajorRadius * cosA;
    double outerY = m_outerEllipseMinorRadius * sinA;

    QPointF outerEdgePoint(outerX, outerY);
    QPointF arrowStart = toScreenCoordinates(outerEdgePoint);

    // 2. 计算内椭圆上的对应点（毫米单位）
    double innerX = m_innerEllipseMajorRadius * cosA + m_innerEllipseCenter.x();
    double innerY = m_innerEllipseMinorRadius * sinA + m_innerEllipseCenter.y();
    QPointF innerEdgePoint(innerX, innerY);
    QPointF innerScreenPoint = toScreenCoordinates(innerEdgePoint);

    // 3. 绘制两个椭圆的偏离距离（连线）
    QPen deviationPen(Qt::cyan);
    deviationPen.setWidthF(1.0);
    deviationPen.setStyle(Qt::DashLine);
    painter.setPen(deviationPen);

    // 绘制从内椭圆中心到外椭圆中心的连线
    QPointF innerCenterScreen = toScreenCoordinates(m_innerEllipseCenter);
    QPointF outerCenterScreen = toScreenCoordinates(QPointF(0, 0));
    painter.drawLine(innerCenterScreen, outerCenterScreen);

    // 在连线中点显示偏离距离
    QPointF midPoint = (innerCenterScreen + outerCenterScreen) / 2;
    double deviation = sqrt(pow(m_innerEllipseCenter.x(), 2) + pow(m_innerEllipseCenter.y(), 2));
    QString deviationText = QString("偏离: %1mm").arg(deviation, 0, 'f', 3);

    QFont deviationFont = painter.font();
    deviationFont.setPointSize(8);
    painter.setFont(deviationFont);
    QPen deviationTextPen(Qt::cyan);
    painter.setPen(deviationTextPen);

    QFontMetrics deviationMetrics(deviationFont);
    QRectF deviationTextRect = deviationMetrics.boundingRect(deviationText);
    QPointF deviationTextPos = midPoint - QPointF(deviationTextRect.width() / 2, -15);
    painter.drawText(deviationTextPos, deviationText);

    // 4. 切换回箭头绘制
    painter.setPen(arrowPen);

    // 计算单位径向向量
    double distance = sqrt(outerX * outerX + outerY * outerY);
    if (distance > 0) {
        double dirX = outerX / distance;
        double dirY = outerY / distance;

        // 绘制箭头（从外椭圆向外延伸）- 箭头长度使用像素单位
        double arrowLengthPixels = 30.0;
        QPointF arrowEnd = arrowStart
                           + QPointF(dirX * arrowLengthPixels, -dirY * arrowLengthPixels);

        // 绘制箭头线
        painter.drawLine(arrowStart, arrowEnd);

        // 箭头头部
        double arrowHeadLengthPixels = 10.0;
        double arrowAngle = M_PI / 6.0;

        // 箭头方向是从终点指向起点（指向外椭圆）
        QPointF arrowDirection = arrowStart - arrowEnd;
        double length = sqrt(arrowDirection.x() * arrowDirection.x()
                             + arrowDirection.y() * arrowDirection.y());

        if (length > 0) {
            arrowDirection = arrowDirection / length;

            // 在起点（外椭圆）处绘制箭头头部
            QPointF arrowLeft(arrowStart.x()
                                  - arrowHeadLengthPixels
                                        * (arrowDirection.x() * cos(arrowAngle)
                                           - arrowDirection.y() * sin(arrowAngle)),
                              arrowStart.y()
                                  - arrowHeadLengthPixels
                                        * (arrowDirection.x() * sin(arrowAngle)
                                           + arrowDirection.y() * cos(arrowAngle)));

            QPointF arrowRight(arrowStart.x()
                                   - arrowHeadLengthPixels
                                         * (arrowDirection.x() * cos(-arrowAngle)
                                            - arrowDirection.y() * sin(-arrowAngle)),
                               arrowStart.y()
                                   - arrowHeadLengthPixels
                                         * (arrowDirection.x() * sin(-arrowAngle)
                                            + arrowDirection.y() * cos(-arrowAngle)));

            painter.drawLine(arrowStart, arrowLeft);
            painter.drawLine(arrowStart, arrowRight);
        }

        // 5. 获取显示文本
        QString displayText;
        if (m_customDisplayTexts.contains(angle)) {
            displayText = m_customDisplayTexts[angle];
        } else {
            // 计算厚度（毫米）
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

        double degrees = radiansToDegrees(angle);

        // 根据角度确定文本在椭圆外侧的位置
        if (degrees >= 315 || degrees < 45) {
            // 右侧区域：文本放在箭头终点的右侧
            textPos = arrowEnd + QPointF(10, -textRect.height() / 2);
        } else if (degrees >= 45 && degrees < 135) {
            // 上方区域：文本放在箭头终点的上方
            textPos = arrowEnd + QPointF(-textRect.width() / 2, -20);
        } else if (degrees >= 135 && degrees < 225) {
            // 左侧区域：文本放在箭头终点的左侧
            textPos = arrowEnd + QPointF(-textRect.width() - 10, -textRect.height() / 2);
        } else {
            // 下方区域：文本放在箭头终点的下方
            textPos = arrowEnd + QPointF(-textRect.width() / 2, 20);
        }

        // 可选：为自定义文本添加背景框以提高可读性
        bool isCustomText = m_customDisplayTexts.contains(angle);
        if (isCustomText) {
            painter.save();
            QRectF backgroundRect(textPos.x() - 2,
                                  textPos.y() - textRect.height() + 2,
                                  textRect.width() + 4,
                                  textRect.height());
            painter.setBrush(QColor(0, 0, 0, 120));
            painter.setPen(Qt::NoPen);
            painter.drawRect(backgroundRect);
            painter.restore();
        }

        painter.drawText(textPos, displayText);

        // 7. 绘制角度标签
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

        QPointF arrowMidPoint = (arrowStart + arrowEnd) / 2;

        if (degrees >= 315 || degrees < 45) {
            angleTextPos = arrowMidPoint + QPointF(0, angleTextRect.height() + 10);
        } else if (degrees >= 45 && degrees < 135) {
            angleTextPos = arrowMidPoint + QPointF(angleTextRect.width() + 5, 0);
        } else if (degrees >= 135 && degrees < 225) {
            angleTextPos = arrowMidPoint + QPointF(0, -10);
        } else {
            angleTextPos = arrowMidPoint + QPointF(-angleTextRect.width() - 5, 0);
        }

        painter.drawText(angleTextPos, angleText);
    }

    painter.restore();
}

void CableDiameterWidget::drawCrossMarkers(QPainter &painter)
{
    painter.save();

    QPen crossPen(m_crossMarkerColor);
    crossPen.setWidthF(1.5);
    painter.setPen(crossPen);

    // 坐标系中心十字
    double crossSizePixels = 15.0;
    QPointF centerScreen = toScreenCoordinates(QPointF(0, 0));
    painter.drawLine(centerScreen - QPointF(crossSizePixels, 0),
                     centerScreen + QPointF(crossSizePixels, 0));
    painter.drawLine(centerScreen - QPointF(0, crossSizePixels),
                     centerScreen + QPointF(0, crossSizePixels));

    // 内椭圆中心十字
    QPointF innerCenterScreen = toScreenCoordinates(m_innerEllipseCenter);
    double smallCrossSizePixels = 10.0;
    painter.drawLine(innerCenterScreen - QPointF(smallCrossSizePixels, 0),
                     innerCenterScreen + QPointF(smallCrossSizePixels, 0));
    painter.drawLine(innerCenterScreen - QPointF(0, smallCrossSizePixels),
                     innerCenterScreen + QPointF(0, smallCrossSizePixels));

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

    // 计算缩放因子
    double zoomFactor = 1.1;
    double delta;

    if (event->angleDelta().y() > 0) {
        // 向上滚动：应该放大
        delta = zoomFactor; // 原来是 1/zoomFactor，改回来
    } else {
        // 向下滚动：应该缩小
        delta = 1.0 / zoomFactor; // 原来是 zoomFactor，改回来
    }

    double newZoomFactor = m_zoomFactor * delta;
    newZoomFactor = qBound(m_minZoomFactor, newZoomFactor, m_maxZoomFactor);

    if (qFuzzyCompare(newZoomFactor, m_zoomFactor)) {
        event->accept();
        return;
    }

    // 保持缩放中心不变（鼠标位置）
    // 我们只需要改变缩放因子，不需要改变缩放中心
    m_zoomFactor = newZoomFactor;

    // 更新坐标矩形和相关参数
    updateZoomParameters();

    // 触发重绘
    update();

    event->accept();
}
