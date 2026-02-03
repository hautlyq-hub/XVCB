// XDDiameterHistoryWidget.h
#ifndef XDDIAMETERHISTORYWIDGET_H
#define XDDIAMETERHISTORYWIDGET_H

#include <QtWidgets/QWidget>
#include <QVector>
#include <QPair>

class DiameterHistoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiameterHistoryWidget(QWidget *parent = nullptr);

    // 数据管理
    void addData(double innerDiameter, double outerDiameter);
    void clearData();
    void setMaxDataPoints(int maxPoints);

    // 显示设置
    void setYAxisRange(double min, double max);
    void setAutoYAxis(bool autoScale);
    void setShowGrid(bool show);
    void setLineWidth(double width);

    // 颜色设置
    void setInnerLineColor(const QColor &color);
    void setOuterLineColor(const QColor &color);
    void setGridColor(const QColor &color);
    void setBackgroundColor(const QColor &color);

    // 获取数据信息
    int dataCount() const;
    QVector<QPair<double, double>> getData() const;

    void reset();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // 数据存储
    QVector<double> m_innerDiameters;
    QVector<double> m_outerDiameters;
    int m_maxDataPoints;

    // 显示设置
    bool m_autoYScale;
    double m_yMin;
    double m_yMax;
    bool m_showGrid;
    double m_lineWidth;

    // 颜色设置
    QColor m_innerLineColor;
    QColor m_outerLineColor;
    QColor m_gridColor;
    QColor m_backgroundColor;
    QColor m_textColor;

    // 绘图区域
    QRectF m_plotArea;
    double m_xScale;
    double m_yScale;

    // 内部方法
    void updateScaling();
    void drawCoordinateSystem(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawCurves(QPainter &painter);
    void drawLegend(QPainter &painter);
    double calculateOptimalYMax() const;
    double calculateOptimalYMin() const;
    void drawStatistics(QPainter &painter);
    void drawTitle(QPainter &painter);

};

#endif // XDDIAMETERHISTORYWIDGET_H
