// XDDiameterHistoryWidget.cpp
#include "XDDiameterHistoryWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QtCore/QDebug>
#include <algorithm>

DiameterHistoryWidget::DiameterHistoryWidget(QWidget *parent)
    : QWidget(parent)
    , m_maxDataPoints(100)
    , m_autoYScale(true)
    , m_yMin(0.0)
    , m_yMax(10.0)
    , m_showGrid(true)
    , m_lineWidth(2.0)
    , m_innerLineColor(QColor(0, 200, 255))    // 蓝色
    , m_outerLineColor(QColor(255, 100, 0))    // 橙色
    , m_gridColor(QColor(100, 100, 100))       // 浅灰色网格
    , m_backgroundColor(Qt::transparent)       // 透明背景
    , m_textColor(Qt::white)                   // 白色文字
{
    // 设置最小尺寸
    setMinimumSize(500, 120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置透明背景
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
}

void DiameterHistoryWidget::addData(double innerDiameter, double outerDiameter)
{
    // 添加新数据
    m_innerDiameters.append(innerDiameter);
    m_outerDiameters.append(outerDiameter);

    // 如果数据超过最大点数，移除旧数据
    while (m_innerDiameters.size() > m_maxDataPoints) {
        m_innerDiameters.removeFirst();
        m_outerDiameters.removeFirst();
    }

    // 如果自动缩放Y轴，重新计算范围
    if (m_autoYScale) {
        m_yMin = calculateOptimalYMin();
        m_yMax = calculateOptimalYMax();
    }

    update();
}

void DiameterHistoryWidget::clearData()
{
    m_innerDiameters.clear();
    m_outerDiameters.clear();
    update();
}

void DiameterHistoryWidget::setMaxDataPoints(int maxPoints)
{
    if (maxPoints > 0 && maxPoints != m_maxDataPoints) {
        m_maxDataPoints = maxPoints;

        // 如果当前数据超过新的最大点数，截断
        while (m_innerDiameters.size() > m_maxDataPoints) {
            m_innerDiameters.removeFirst();
            m_outerDiameters.removeFirst();
        }

        update();
    }
}

void DiameterHistoryWidget::setYAxisRange(double min, double max)
{
    if (min < max) {
        m_autoYScale = false;
        m_yMin = min;
        m_yMax = max;
        update();
    }
}

void DiameterHistoryWidget::setAutoYAxis(bool autoScale)
{
    if (m_autoYScale != autoScale) {
        m_autoYScale = autoScale;
        if (autoScale) {
            m_yMin = calculateOptimalYMin();
            m_yMax = calculateOptimalYMax();
        }
        update();
    }
}

void DiameterHistoryWidget::setShowGrid(bool show)
{
    if (m_showGrid != show) {
        m_showGrid = show;
        update();
    }
}

void DiameterHistoryWidget::setLineWidth(double width)
{
    if (width > 0 && !qFuzzyCompare(m_lineWidth, width)) {
        m_lineWidth = width;
        update();
    }
}

void DiameterHistoryWidget::setInnerLineColor(const QColor &color)
{
    if (m_innerLineColor != color) {
        m_innerLineColor = color;
        update();
    }
}

void DiameterHistoryWidget::setOuterLineColor(const QColor &color)
{
    if (m_outerLineColor != color) {
        m_outerLineColor = color;
        update();
    }
}

void DiameterHistoryWidget::setGridColor(const QColor &color)
{
    if (m_gridColor != color) {
        m_gridColor = color;
        update();
    }
}

void DiameterHistoryWidget::setBackgroundColor(const QColor &color)
{
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        update();
    }
}

int DiameterHistoryWidget::dataCount() const
{
    return m_innerDiameters.size();
}

QVector<QPair<double, double>> DiameterHistoryWidget::getData() const
{
    QVector<QPair<double, double>> data;
    int count = qMin(m_innerDiameters.size(), m_outerDiameters.size());
    for (int i = 0; i < count; ++i) {
        data.append(qMakePair(m_innerDiameters[i], m_outerDiameters[i]));
    }
    return data;
}

void DiameterHistoryWidget::reset()
{
    // 清除所有数据
    m_innerDiameters.clear();
    m_outerDiameters.clear();

    // 重置Y轴范围为默认值
    if (m_autoYScale) {
        m_yMin = 0.0;
        m_yMax = 10.0;
    }

    // 触发重绘
    update();
}

double DiameterHistoryWidget::calculateOptimalYMax() const
{
    if (m_outerDiameters.isEmpty()) {
        return 10.0;  // 默认最大值
    }

    double maxValue = *std::max_element(m_outerDiameters.constBegin(),
                                       m_outerDiameters.constEnd());

    // 添加10%的边距
    if (maxValue > 0) {
        return maxValue * 1.1;
    } else {
        return 10.0;
    }
}

double DiameterHistoryWidget::calculateOptimalYMin() const
{
    if (m_innerDiameters.isEmpty()) {
        return 0.0;  // 默认最小值
    }

    double minValue = *std::min_element(m_innerDiameters.constBegin(),
                                       m_innerDiameters.constEnd());

    // 确保最小值不会小于0，并添加10%边距
    minValue = qMax(0.0, minValue);
    if (minValue > 0) {
        return minValue * 0.9;
    } else {
        return 0.0;
    }
}

void DiameterHistoryWidget::updateScaling()
{
    // 减小左边距，从60改为40
    int leftMargin = 40;     // Y轴刻度需要空间
    int rightMargin = 15;    // 右侧留少量空间
    int topMargin = 15;      // 顶部标题
    int bottomMargin = 35;   // X轴标签和刻度

    // 确保绘图区域有效
    int availableWidth = width() - leftMargin - rightMargin;
    int availableHeight = height() - topMargin - bottomMargin;

    // 最小绘图区域保护
    if (availableWidth < 200) {
        availableWidth = 200;
        leftMargin = (width() - availableWidth) / 2;
    }
    if (availableHeight < 60) {  // 最小绘图高度
        availableHeight = 60;
        topMargin = (height() - availableHeight) / 2;
    }

    m_plotArea = QRectF(leftMargin, topMargin,
                       availableWidth, availableHeight);

    if (m_plotArea.width() <= 0 || m_plotArea.height() <= 0) {
        return;
    }

    // 计算X轴缩放因子
    int dataCount = m_innerDiameters.size();
    if (dataCount <= 1) {
        m_xScale = m_plotArea.width();
    } else {
        m_xScale = m_plotArea.width() / qMax(1, dataCount - 1);
    }

    // 计算Y轴缩放因子
    double yRange = m_yMax - m_yMin;
    if (yRange > 0) {
        m_yScale = m_plotArea.height() / yRange;
    } else {
        m_yScale = 1.0;
    }
}

void DiameterHistoryWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    updateScaling();

    // 使用父部件的背景色 #545454
    painter.fillRect(rect(), QColor(0x54, 0x54, 0x54));

    // 绘制标题
    drawTitle(painter);

    // 绘制坐标系统
    drawCoordinateSystem(painter);

    if (m_showGrid) {
        drawGrid(painter);
    }

    // 绘制曲线
    drawCurves(painter);

    // 绘制图例
    drawLegend(painter);

    // 绘制数据统计
    drawStatistics(painter);
}

void DiameterHistoryWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateScaling();
}

void DiameterHistoryWidget::drawTitle(QPainter &painter)
{
    painter.save();

    QFont titleFont = painter.font();
    titleFont.setPointSize(9);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(m_textColor);

    QString title = QString("直径变化趋势 (最近%1条记录)").arg(m_maxDataPoints);
    QRectF titleRect(0, 5, width(), 20);

    // 检查标题是否在边界内
    if (titleRect.bottom() < m_plotArea.top() - 5) {
        painter.drawText(titleRect, Qt::AlignCenter, title);
    }

    painter.restore();
}

void DiameterHistoryWidget::drawCoordinateSystem(QPainter &painter)
{
    painter.save();

    QPen axisPen(m_textColor);
    axisPen.setWidthF(1.5);
    painter.setPen(axisPen);

    // 绘制X轴和Y轴
    painter.drawLine(m_plotArea.bottomLeft(), m_plotArea.bottomRight());  // X轴
    painter.drawLine(m_plotArea.bottomLeft(), m_plotArea.topLeft());      // Y轴

    // 设置字体
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    painter.setPen(m_textColor);

    int dataCount = m_innerDiameters.size();

    // 绘制X轴刻度（跳着标记）
    int maxTicks = qMin(8, dataCount);
    if (dataCount > 1 && maxTicks > 1) {
        double tickInterval = (dataCount - 1.0) / (maxTicks - 1.0);

        for (int i = 0; i < maxTicks; ++i) {
            int tickIndex = i * tickInterval;
            double xPos = m_plotArea.left() + tickIndex * m_xScale;

            // 绘制刻度线
            painter.drawLine(QPointF(xPos, m_plotArea.bottom()),
                            QPointF(xPos, m_plotArea.bottom() + 5));

            // 每隔一个刻度显示标签
            if (i % 2 == 0) {
                QString label = QString::number(tickIndex + 1);
                QRectF labelRect = QFontMetrics(font).boundingRect(label);

                painter.drawText(xPos - labelRect.width()/2,
                               m_plotArea.bottom() + labelRect.height() + 8,
                               label);
            }
        }
    }

    // 绘制Y轴刻度（跳着标记）
    int yTicks = 6;
    double yTickInterval = (m_yMax - m_yMin) / yTicks;

    for (int i = 0; i <= yTicks; ++i) {
        double yValue = m_yMin + i * yTickInterval;
        double yPos = m_plotArea.bottom() - (yValue - m_yMin) * m_yScale;

        // 绘制刻度线
        painter.drawLine(QPointF(m_plotArea.left() - 5, yPos),
                        QPointF(m_plotArea.left(), yPos));

        // 每隔一个刻度显示标签
        if (i % 2 == 0) {
            QString label = QString::number(yValue, 'f', 1);
            QRectF labelRect = QFontMetrics(font).boundingRect(label);
            painter.drawText(m_plotArea.left() - labelRect.width() - 8,
                           yPos + labelRect.height()/3,
                           label);
        }
    }

    painter.restore();
}

void DiameterHistoryWidget::drawGrid(QPainter &painter)
{
    painter.save();

    QPen gridPen(m_gridColor);
    gridPen.setStyle(Qt::DashLine);
    gridPen.setWidthF(0.5);
    painter.setPen(gridPen);

    // 绘制垂直网格线
    int dataCount = m_innerDiameters.size();
    int maxTicks = qMin(8, dataCount);

    if (dataCount > 1 && maxTicks > 1) {
        double tickInterval = (dataCount - 1.0) / (maxTicks - 1.0);

        for (int i = 1; i < maxTicks - 1; ++i) {
            int tickIndex = i * tickInterval;
            double xPos = m_plotArea.left() + tickIndex * m_xScale;

            painter.drawLine(QPointF(xPos, m_plotArea.top()),
                            QPointF(xPos, m_plotArea.bottom()));
        }
    }

    // 绘制水平网格线
    int yTicks = 6;
    double yTickInterval = (m_yMax - m_yMin) / yTicks;

    for (int i = 1; i < yTicks; ++i) {
        double yValue = m_yMin + i * yTickInterval;
        double yPos = m_plotArea.bottom() - (yValue - m_yMin) * m_yScale;

        painter.drawLine(QPointF(m_plotArea.left(), yPos),
                        QPointF(m_plotArea.right(), yPos));
    }

    painter.restore();
}

void DiameterHistoryWidget::drawCurves(QPainter &painter)
{
    painter.save();

    int dataCount = m_innerDiameters.size();

    // 只有在有足够数据时才绘制曲线
    if (dataCount > 0) {
        // 绘制内径曲线
        if (dataCount > 1) {
            QPainterPath innerPath;

            for (int i = 0; i < dataCount; ++i) {
                double x = m_plotArea.left() + i * m_xScale;
                double y = m_plotArea.bottom() - (m_innerDiameters[i] - m_yMin) * m_yScale;

                if (i == 0) {
                    innerPath.moveTo(x, y);
                } else {
                    innerPath.lineTo(x, y);
                }
            }

            QPen innerPen(m_innerLineColor);
            innerPen.setWidthF(m_lineWidth);
            painter.setPen(innerPen);
            painter.drawPath(innerPath);
        }

        // 绘制外径曲线
        if (dataCount > 1) {
            QPainterPath outerPath;

            for (int i = 0; i < dataCount; ++i) {
                double x = m_plotArea.left() + i * m_xScale;
                double y = m_plotArea.bottom() - (m_outerDiameters[i] - m_yMin) * m_yScale;

                if (i == 0) {
                    outerPath.moveTo(x, y);
                } else {
                    outerPath.lineTo(x, y);
                }
            }

            QPen outerPen(m_outerLineColor);
            outerPen.setWidthF(m_lineWidth);
            painter.setPen(outerPen);
            painter.drawPath(outerPath);
        }

        // 在数据点较少时绘制数据点
        if (dataCount <= 20) {
            for (int i = 0; i < dataCount; ++i) {
                double x = m_plotArea.left() + i * m_xScale;
                double yInner = m_plotArea.bottom() - (m_innerDiameters[i] - m_yMin) * m_yScale;
                double yOuter = m_plotArea.bottom() - (m_outerDiameters[i] - m_yMin) * m_yScale;

                // 绘制内径点
                painter.setPen(m_innerLineColor);
                painter.setBrush(m_innerLineColor);
                painter.drawEllipse(QPointF(x, yInner), 2, 2);

                // 绘制外径点
                painter.setPen(m_outerLineColor);
                painter.setBrush(m_outerLineColor);
                painter.drawEllipse(QPointF(x, yOuter), 2, 2);
            }
        }
    } else {
        // 没有数据时显示提示
        painter.setPen(m_textColor);
        painter.drawText(m_plotArea, Qt::AlignCenter, "暂无数据，请点击测量");
    }

    painter.restore();
}

void DiameterHistoryWidget::drawLegend(QPainter &painter)
{
    painter.save();

    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    // 图例位置（在绘图区域顶部右侧）
    QRectF legendRect(m_plotArea.right() - 100, m_plotArea.top(), 95, 30);

    // 绘制图例背景
    painter.setBrush(QColor(0, 0, 0, 150));
    painter.setPen(Qt::NoPen);
    painter.drawRect(legendRect);

    // 绘制内径图例
    painter.setPen(m_innerLineColor);
    painter.drawLine(legendRect.left() + 8, legendRect.top() + 8,
                    legendRect.left() + 25, legendRect.top() + 8);
    painter.drawText(legendRect.left() + 30, legendRect.top() + 13, "内径");

    // 绘制外径图例
    painter.setPen(m_outerLineColor);
    painter.drawLine(legendRect.left() + 8, legendRect.top() + 20,
                    legendRect.left() + 25, legendRect.top() + 20);
    painter.drawText(legendRect.left() + 30, legendRect.top() + 25, "外径");

    painter.restore();
}

void DiameterHistoryWidget::drawStatistics(QPainter &painter)
{
    painter.save();

    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    painter.setPen(m_textColor);

    if (!m_innerDiameters.isEmpty() && !m_outerDiameters.isEmpty()) {
        // 获取最新数据
        double currentInner = m_innerDiameters.last();
        double currentOuter = m_outerDiameters.last();
        double currentThickness = currentOuter - currentInner;

        // 统计数据区域（底部）
        QRectF statsRect(10, height() - 25, width() - 20, 20);

        QString statsText = QString("当前: 内径=%1mm | 外径=%2mm | 厚度=%3mm | 记录数:%4")
                           .arg(currentInner, 0, 'f', 2)
                           .arg(currentOuter, 0, 'f', 2)
                           .arg(currentThickness, 0, 'f', 2)
                           .arg(m_innerDiameters.size());

        // 添加背景
        painter.setBrush(QColor(0, 0, 0, 150));
        painter.setPen(Qt::NoPen);
        painter.drawRect(statsRect);

        // 绘制文本
        painter.setPen(m_textColor);
        painter.drawText(statsRect, Qt::AlignCenter, statsText);
    }

    painter.restore();
}
