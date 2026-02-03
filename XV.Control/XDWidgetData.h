#ifndef XDWIDGETDATA_H
#define XDWIDGETDATA_H
#include <QObject>
#include <QPoint>
#include <QMargins>
#include <QRect>
#include "XDCursorPosCalculator.h"

class XDFramelessHelperPrivate;
class XDCursorPosCalculator;
class QWidget;
class QEvent;
class QMouseEvent;
class QRubberBand;
class QPoint;

class XDWidgetData
{
public:
    XDWidgetData(XDFramelessHelperPrivate *_d, QWidget *window, QWidget *shadowContainerWidget = nullptr);
    ~XDWidgetData();

    QWidget *widget();
    // 处理鼠标事件-划过、按下、释放、移动
    void handleWidgetEvent(QEvent *event);
    // 更新橡皮筋状态
    void updateRubberBandStatus();

    void setShadowWidth(const int width);

private:
    // 处理鼠标按下
    void handleMousePressEvent(QMouseEvent *event);
    // 处理鼠标释放
    void handleMouseReleaseEvent(QMouseEvent *event);
    // 处理鼠标移动
    void handleMouseMoveEvent(QMouseEvent *event);
    // 处理鼠标离开
    void handleLeaveEvent(QMouseEvent *event);
    // 处理鼠标进入
    void handleHoverMoveEvent(QMouseEvent *event);

    // 更新鼠标样式
    void updateCursorShape(const QPoint &gMousePos);
    // 重置窗口大小
    void resizeWidget(const QPoint &gMousePos);
    // 移动窗体
    void moveWidget(const QPoint &gMousePos);

private:
    XDFramelessHelperPrivate *d;
    QRubberBand *m_pRubberBand;
    QWidget *m_pWidget;
    QPoint m_ptDragPos;
    double m_dLeftScale; // 鼠标位置距离最窗口最左边的距离占整个宽度的比例
    int m_nRightLength; // 鼠标位置距离最窗口最右边的距离
    XDCursorPosCalculator m_pressedMousePos;
    XDCursorPosCalculator m_moveMousePos;
    bool m_bLeftButtonPressed;
    bool m_bLeftButtonTitlePressed;
    bool m_bCursorShapeChanged;
    Qt::WindowFlags m_windowFlags;
    int m_nShadowWidth;
    int m_oldShadowWidth;
    QMargins m_oldContentsMargin;
    QWidget *m_window;
    QWidget *m_shadowContainerWidget;
    QRect m_oldGeometry;
};

#endif // XDWIDGETDATA_H
