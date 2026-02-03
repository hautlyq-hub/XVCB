#ifndef INTERACTIVEIMAGELABEL_H
#define INTERACTIVEIMAGELABEL_H

#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QTransform>
#include <QWheelEvent>

class InteractiveImageLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(bool enableZoom READ enableZoom WRITE setEnableZoom)
    Q_PROPERTY(bool enableDrag READ enableDrag WRITE setEnableDrag)
    Q_PROPERTY(bool enableWindowLevel READ enableWindowLevel WRITE setEnableWindowLevel)
    Q_PROPERTY(double zoomFactor READ zoomFactor WRITE setZoomFactor)
    Q_PROPERTY(double minZoom READ minZoom WRITE setMinZoom)
    Q_PROPERTY(double maxZoom READ maxZoom WRITE setMaxZoom)
    Q_PROPERTY(int windowWidth READ windowWidth WRITE setWindowWidth NOTIFY windowLevelChanged)
    Q_PROPERTY(int windowLevel READ windowLevel WRITE setWindowLevel NOTIFY windowLevelChanged)

public:
    enum InteractionMode { ModeNone = 0, ModeDrag, ModeWindowLevel };

    explicit InteractiveImageLabel(QWidget *parent = nullptr);

    // 设置和获取图像
    void setImage(const QImage &image);
    void setPixmap(const QPixmap &pixmap);
    QImage originalImage() const;
    QPixmap currentPixmap() const;

    // 缩放控制
    void zoomIn(double factor = 1.25);
    void zoomOut(double factor = 0.8);
    void zoomFit();
    void zoomOriginal();
    void zoomTo(double factor);

    // 平移控制
    void pan(int dx, int dy);
    void centerImage();
    void resetView();

    // 窗宽窗位控制
    void setWindowLevel(int width, int level);
    void resetWindowLevel();

    // 属性访问器
    bool enableZoom() const { return m_enableZoom; }
    bool enableDrag() const { return m_enableDrag; }
    bool enableWindowLevel() const { return m_enableWindowLevel; }
    double zoomFactor() const { return m_zoomFactor; }
    double minZoom() const { return m_minZoom; }
    double maxZoom() const { return m_maxZoom; }
    int windowWidth() const { return m_windowWidth; }
    int windowLevel() const { return m_windowCenter; }

    // 属性设置器
    void setEnableZoom(bool enable);
    void setEnableDrag(bool enable);
    void setEnableWindowLevel(bool enable);
    void setZoomFactor(double factor, const QPointF &centerPoint = QPointF());
    void setMinZoom(double min);
    void setMaxZoom(double max);
    void setWindowWidth(int width);
    void setWindowLevel(int level);
    void applyZoom(double newZoom);

signals:
    void imageChanged();
    void zoomChanged(double factor);
    void viewChanged();
    void windowLevelChanged(int width, int level);
    void mouseMoved(QPoint imagePos, int pixelValue);
    void mouseClicked(QPoint imagePos, int pixelValue);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    // 图像数据
    QImage m_originalImage;
    QImage m_displayImage;
    QPixmap m_currentPixmap;

    // 变换参数
    double m_zoomFactor;
    double m_minZoom;
    double m_maxZoom;
    QPointF m_imageOffset;
    QPointF m_lastMousePos;

    // 窗宽窗位参数
    int m_windowWidth;
    int m_windowCenter;
    int m_originalMinValue;
    int m_originalMaxValue;

    // 交互状态
    InteractionMode m_interactionMode;
    bool m_enableZoom;
    bool m_enableDrag;
    bool m_enableWindowLevel;
    bool m_mousePressed;

    // 标记
    bool m_hasImage;
    bool m_needUpdateDisplay;

    // 私有方法
    void updateDisplayImage();
    QImage applyWindowLevel(const QImage &image);
    QPointF widgetToImage(const QPoint &widgetPos) const;
    QPoint imageToWidget(const QPointF &imagePos) const;
    QRectF visibleImageRect() const;
    void clampImagePosition();
    void emitMousePositionInfo(const QPoint &widgetPos);
    void createContextMenu();

    // 辅助方法
    static QColor getPixelColor(const QImage &image, int x, int y);
    static int getGrayValue(const QColor &color);
};

#endif // INTERACTIVEIMAGELABEL_H
