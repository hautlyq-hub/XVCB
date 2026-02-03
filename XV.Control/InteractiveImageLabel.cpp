#include "InteractiveImageLabel.h"
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDebug>
#include <QMenu>
#include <QScrollBar>
#include <QWheelEvent>
#include <cmath>

InteractiveImageLabel::InteractiveImageLabel(QWidget *parent)
    : QLabel(parent)
    , m_zoomFactor(1.0)
    , m_minZoom(0.1)
    , m_maxZoom(10.0)
    , m_imageOffset(0, 0)
    , m_windowWidth(256)
    , m_windowCenter(128)
    , m_originalMinValue(0)
    , m_originalMaxValue(255)
    , m_interactionMode(ModeNone)
    , m_enableZoom(true)
    , m_enableDrag(true)
    , m_enableWindowLevel(true)
    , m_mousePressed(false)
    , m_hasImage(false)
    , m_needUpdateDisplay(true)
{
    setMouseTracking(true);
    setScaledContents(false);
    setAlignment(Qt::AlignCenter);

    // 设置样式
    setStyleSheet("background-color: #2b2b2b; border: 1px solid #555;");
}

// 设置图像
void InteractiveImageLabel::setImage(const QImage &image)
{
    if (image.isNull()) {
        m_hasImage = false;
        clear();
        setText("图像无效");
        return;
    }

    m_originalImage = image.convertToFormat(QImage::Format_Grayscale8);
    m_hasImage = true;

    // 分析原始图像的灰度范围
    m_originalMinValue = 255;
    m_originalMaxValue = 0;

    for (int y = 0; y < m_originalImage.height(); ++y) {
        const uchar *line = m_originalImage.constScanLine(y);
        for (int x = 0; x < m_originalImage.width(); ++x) {
            uchar value = line[x];
            if (value < m_originalMinValue)
                m_originalMinValue = value;
            if (value > m_originalMaxValue)
                m_originalMaxValue = value;
        }
    }

    // 重置视图参数
    m_zoomFactor = 1.0;
    m_imageOffset = QPointF(0, 0);

    // 计算合适的窗口宽窗位
    m_windowWidth = m_originalMaxValue - m_originalMinValue + 1;
    m_windowCenter = (m_originalMaxValue + m_originalMinValue) / 2;

    // 标记需要更新显示
    m_needUpdateDisplay = true;

    // 居中显示
    centerImage();

    // 发射信号
    emit imageChanged();
    emit windowLevelChanged(m_windowWidth, m_windowCenter);

    update();
}

void InteractiveImageLabel::setPixmap(const QPixmap &pixmap)
{
    setImage(pixmap.toImage());
}

QImage InteractiveImageLabel::originalImage() const
{
    return m_originalImage;
}

QPixmap InteractiveImageLabel::currentPixmap() const
{
    return m_currentPixmap;
}

void InteractiveImageLabel::zoomFit()
{
    if (!m_hasImage)
        return;

    if (m_originalImage.isNull())
        return;

    double widthRatio = double(width() - 10) / m_originalImage.width();
    double heightRatio = double(height() - 10) / m_originalImage.height();
    double fitRatio = qMin(widthRatio, heightRatio);

    setZoomFactor(fitRatio);
    centerImage();
}

void InteractiveImageLabel::zoomOriginal()
{
    setZoomFactor(1.0);
    centerImage();
}

void InteractiveImageLabel::zoomTo(double factor)
{
    setZoomFactor(factor);
}

// 平移控制
void InteractiveImageLabel::pan(int dx, int dy)
{
    if (!m_enableDrag || !m_hasImage)
        return;

    m_imageOffset += QPointF(dx / m_zoomFactor, dy / m_zoomFactor);
    clampImagePosition();
    update();
    emit viewChanged();
}

void InteractiveImageLabel::centerImage()
{
    if (!m_hasImage || m_originalImage.isNull())
        return;

    QSizeF imageSize = m_originalImage.size() * m_zoomFactor;
    QSize widgetSize = size();

    m_imageOffset = QPointF((widgetSize.width() - imageSize.width()) / (2 * m_zoomFactor),
                            (widgetSize.height() - imageSize.height()) / (2 * m_zoomFactor));

    update();
    emit viewChanged();
}

void InteractiveImageLabel::zoomIn(double factor)
{
    if (!m_enableZoom || !m_hasImage)
        return;

    double newZoom = m_zoomFactor * factor; // 放大：乘以
    applyZoom(newZoom);
}

void InteractiveImageLabel::zoomOut(double factor)
{
    if (!m_enableZoom || !m_hasImage)
        return;

    double newZoom = m_zoomFactor / factor; // 缩小：除以
    applyZoom(newZoom);
}

// 公共的缩放应用函数
void InteractiveImageLabel::applyZoom(double newZoom)
{
    newZoom = qBound(m_minZoom, newZoom, m_maxZoom);

    if (qFuzzyCompare(newZoom, m_zoomFactor)) {
        return;
    }

    // 保持鼠标位置
    if (underMouse()) {
        QPoint mousePos = mapFromGlobal(QCursor::pos());
        QPointF imagePos = widgetToImage(mousePos);

        m_zoomFactor = newZoom;

        QPointF newWidgetPos = imageToWidget(imagePos);
        QPointF delta = mousePos - newWidgetPos;
        m_imageOffset += delta / m_zoomFactor;

        clampImagePosition();
    } else {
        m_zoomFactor = newZoom;
        clampImagePosition();
    }

    update();
    emit zoomChanged(m_zoomFactor);
    emit viewChanged();
}

void InteractiveImageLabel::resetView()
{
    m_zoomFactor = 1.0;
    m_imageOffset = QPointF(0, 0);
    resetWindowLevel();
    centerImage();
    update();
}

// 窗宽窗位控制
void InteractiveImageLabel::setWindowLevel(int width, int level)
{
    if (!m_enableWindowLevel)
        return;

    m_windowWidth = qMax(1, width);
    m_windowCenter = qBound(0, level, 255);

    m_needUpdateDisplay = true;
    update();

    emit windowLevelChanged(m_windowWidth, m_windowCenter);
}

void InteractiveImageLabel::resetWindowLevel()
{
    if (!m_hasImage)
        return;

    m_windowWidth = m_originalMaxValue - m_originalMinValue + 1;
    m_windowCenter = (m_originalMaxValue + m_originalMinValue) / 2;

    m_needUpdateDisplay = true;
    update();

    emit windowLevelChanged(m_windowWidth, m_windowCenter);
}

// 属性设置器
void InteractiveImageLabel::setEnableZoom(bool enable)
{
    m_enableZoom = enable;
}

void InteractiveImageLabel::setEnableDrag(bool enable)
{
    m_enableDrag = enable;
    setCursor(enable ? Qt::OpenHandCursor : Qt::ArrowCursor);
}

void InteractiveImageLabel::setEnableWindowLevel(bool enable)
{
    m_enableWindowLevel = enable;
}

void InteractiveImageLabel::setZoomFactor(double factor, const QPointF &centerPoint)
{
    if (!m_enableZoom)
        return;

    double oldZoom = m_zoomFactor;
    m_zoomFactor = qBound(m_minZoom, factor, m_maxZoom);

    // 如果缩放因子没有变化，直接返回
    if (qFuzzyCompare(oldZoom, m_zoomFactor)) {
        return;
    }

    // 保持指定位置不变
    if (centerPoint.isNull() && underMouse()) {
        QPoint mousePos = mapFromGlobal(QCursor::pos());
        QPointF imagePos = widgetToImage(mousePos);
        QPointF newWidgetPos = imageToWidget(imagePos);
        QPointF delta = mousePos - newWidgetPos;
        m_imageOffset += delta / m_zoomFactor;
    }

    clampImagePosition();
    update();
    emit zoomChanged(m_zoomFactor);
    emit viewChanged();
}

void InteractiveImageLabel::setMinZoom(double min)
{
    m_minZoom = qMax(0.01, min);
    if (m_zoomFactor < m_minZoom) {
        setZoomFactor(m_minZoom);
    }
}

void InteractiveImageLabel::setMaxZoom(double max)
{
    m_maxZoom = qMax(m_minZoom, max);
    if (m_zoomFactor > m_maxZoom) {
        setZoomFactor(m_maxZoom);
    }
}

void InteractiveImageLabel::setWindowWidth(int width)
{
    setWindowLevel(width, m_windowCenter);
}

void InteractiveImageLabel::setWindowLevel(int level)
{
    setWindowLevel(m_windowWidth, level);
}

// 事件处理
void InteractiveImageLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor("#2b2b2b"));

    if (!m_hasImage || m_originalImage.isNull()) {
        QLabel::paintEvent(event);
        return;
    }

    // 更新显示图像（如果需要）
    if (m_needUpdateDisplay) {
        updateDisplayImage();
        m_needUpdateDisplay = false;
    }

    // 计算绘制位置
    QSizeF scaledSize = m_originalImage.size() * m_zoomFactor;
    QPointF drawPos = m_imageOffset * m_zoomFactor;

    // 绘制图像
    if (!m_displayImage.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(m_displayImage)
                             .scaled(scaledSize.toSize(),
                                     Qt::IgnoreAspectRatio,
                                     Qt::SmoothTransformation);

        painter.drawPixmap(drawPos.toPoint(), pixmap);
        m_currentPixmap = pixmap;
    }

    // 绘制边框
    painter.setPen(QPen(QColor("#555"), 1));
    painter.drawRect(QRectF(drawPos, scaledSize).adjusted(0, 0, -1, -1));

    // 显示信息
    if (underMouse()) {
        QPoint mousePos = mapFromGlobal(QCursor::pos());
        if (QRectF(drawPos, scaledSize).contains(mousePos)) {
            // 绘制十字线
            painter.setPen(QPen(Qt::yellow, 1, Qt::DotLine));
            painter.drawLine(QPoint(mousePos.x(), 0), QPoint(mousePos.x(), height()));
            painter.drawLine(QPoint(0, mousePos.y()), QPoint(width(), mousePos.y()));

            // 显示坐标信息
            QPointF imagePos = widgetToImage(mousePos);
            int pixelValue = getGrayValue(
                getPixelColor(m_originalImage, int(imagePos.x()), int(imagePos.y())));

            QString info = QString("(%1, %2): %3")
                               .arg(int(imagePos.x()))
                               .arg(int(imagePos.y()))
                               .arg(pixelValue);
            painter.setPen(Qt::white);
            painter.drawText(10, 20, info);
        }
    }
}

void InteractiveImageLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    if (m_hasImage) {
        clampImagePosition();
        update();
    }
}

void InteractiveImageLabel::mousePressEvent(QMouseEvent *event)
{
    if (!m_hasImage) {
        QLabel::mousePressEvent(event);
        return;
    }

    m_lastMousePos = event->pos();
    m_mousePressed = true;

    if (event->button() == Qt::LeftButton && m_enableDrag) {
        m_interactionMode = ModeDrag;
        setCursor(Qt::ClosedHandCursor);
        emit mouseClicked(widgetToImage(event->pos()).toPoint(),
                          getGrayValue(getPixelColor(m_originalImage,
                                                     widgetToImage(event->pos()).toPoint().x(),
                                                     widgetToImage(event->pos()).toPoint().y())));
    } else if (event->button() == Qt::RightButton && m_enableWindowLevel) {
        m_interactionMode = ModeWindowLevel;
        setCursor(Qt::SizeVerCursor);
    }

    event->accept();
}

void InteractiveImageLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_hasImage) {
        QLabel::mouseMoveEvent(event);
        return;
    }

    QPointF currentPos = event->pos();
    QPointF delta = currentPos - m_lastMousePos;

    if (m_mousePressed) {
        if (m_interactionMode == ModeDrag && m_enableDrag) {
            // 拖动图像
            m_imageOffset += delta / m_zoomFactor;
            clampImagePosition();
            update();
            emit viewChanged();
        } else if (m_interactionMode == ModeWindowLevel && m_enableWindowLevel) {
            // 调整窗宽窗位
            // 水平移动调整窗宽，垂直移动调整窗位
            int widthChange = int(delta.x());
            int levelChange = int(delta.y());

            if (event->modifiers() & Qt::ShiftModifier) {
                // 按住Shift只调整窗宽
                setWindowWidth(m_windowWidth + widthChange);
            } else if (event->modifiers() & Qt::ControlModifier) {
                // 按住Ctrl只调整窗位
                setWindowLevel(m_windowCenter + levelChange);
            } else {
                // 同时调整
                setWindowLevel(m_windowWidth + widthChange, m_windowCenter + levelChange);
            }
        }
    }

    // 发射鼠标位置信息
    emitMousePositionInfo(event->pos());

    m_lastMousePos = currentPos;
    event->accept();
}

void InteractiveImageLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_hasImage) {
        QLabel::mouseReleaseEvent(event);
        return;
    }

    m_mousePressed = false;
    m_interactionMode = ModeNone;

    if (m_enableDrag) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }

    event->accept();
}

void InteractiveImageLabel::wheelEvent(QWheelEvent *event)
{
    const double ZOOM_FACTOR = 1.25;                      // 固定缩放系数
    const double ZOOM_FACTOR_INVERSE = 1.0 / ZOOM_FACTOR; // 0.8

    int delta = event->angleDelta().y();

    if (delta > 0) {
        // 放大
        m_zoomFactor *= ZOOM_FACTOR;
    } else {
        // 缩小
        m_zoomFactor *= ZOOM_FACTOR_INVERSE;
    }

    m_zoomFactor = qBound(m_minZoom, m_zoomFactor, m_maxZoom);

    update();
}

void InteractiveImageLabel::contextMenuEvent(QContextMenuEvent *event)
{
    qDebug() << "111111";
    if (!m_hasImage) {
        QLabel::contextMenuEvent(event);
        return;
    }

    QMenu menu(this);

    // 缩放菜单
    QMenu *zoomMenu = menu.addMenu("缩放");
    zoomMenu->addAction("放大", this, &InteractiveImageLabel::zoomIn, QKeySequence(Qt::Key_Plus));
    zoomMenu->addAction("缩小", this, &InteractiveImageLabel::zoomOut, QKeySequence(Qt::Key_Minus));
    zoomMenu->addAction("适应窗口", this, &InteractiveImageLabel::zoomFit, QKeySequence("F"));
    zoomMenu->addAction("实际大小", this, &InteractiveImageLabel::zoomOriginal, QKeySequence("R"));
    zoomMenu->addSeparator();
    zoomMenu->addAction("重置视图", this, &InteractiveImageLabel::resetView, QKeySequence("Home"));

    // 窗宽窗位菜单
    QMenu *wlMenu = menu.addMenu("窗宽窗位");
    wlMenu->addAction("重置窗宽窗位",
                      this,
                      &InteractiveImageLabel::resetWindowLevel,
                      QKeySequence("W"));
    wlMenu->addSeparator();
    wlMenu->addAction(QString("窗宽: %1").arg(m_windowWidth));
    wlMenu->addAction(QString("窗位: %1").arg(m_windowCenter));

    // 显示像素值
    QPoint imagePos = widgetToImage(event->pos()).toPoint();
    if (m_originalImage.rect().contains(imagePos)) {
        int pixelValue = getGrayValue(getPixelColor(m_originalImage, imagePos.x(), imagePos.y()));
        menu.addSeparator();
        menu.addAction(
            QString("像素值 (%1, %2): %3").arg(imagePos.x()).arg(imagePos.y()).arg(pixelValue));
    }

    menu.exec(event->globalPos());
}

// 私有方法
void InteractiveImageLabel::updateDisplayImage()
{
    if (m_originalImage.isNull())
        return;

    if (m_enableWindowLevel) {
        m_displayImage = applyWindowLevel(m_originalImage);
    } else {
        m_displayImage = m_originalImage;
    }
}

QImage InteractiveImageLabel::applyWindowLevel(const QImage &image)
{
    if (image.isNull())
        return QImage();

    QImage result = image.convertToFormat(QImage::Format_Grayscale8);

    int lower = m_windowCenter - m_windowWidth / 2;
    int upper = m_windowCenter + m_windowWidth / 2;

    // 确保范围有效
    lower = qMax(0, lower);
    upper = qMin(255, upper);

    if (lower >= upper) {
        // 如果窗宽为0或负数，显示二值图像
        for (int y = 0; y < result.height(); ++y) {
            uchar *line = result.scanLine(y);
            for (int x = 0; x < result.width(); ++x) {
                line[x] = (line[x] >= m_windowCenter) ? 255 : 0;
            }
        }
    } else {
        // 应用线性窗宽窗位变换
        double scale = 255.0 / (upper - lower);

        for (int y = 0; y < result.height(); ++y) {
            uchar *line = result.scanLine(y);
            for (int x = 0; x < result.width(); ++x) {
                int value = line[x];
                if (value < lower) {
                    line[x] = 0;
                } else if (value > upper) {
                    line[x] = 255;
                } else {
                    line[x] = uchar((value - lower) * scale);
                }
            }
        }
    }

    return result;
}

QPointF InteractiveImageLabel::widgetToImage(const QPoint &widgetPos) const
{
    if (!m_hasImage)
        return QPointF();

    QPointF imagePos = (widgetPos - m_imageOffset * m_zoomFactor) / m_zoomFactor;

    // 确保在图像范围内
    imagePos.setX(qBound(0.0, imagePos.x(), double(m_originalImage.width() - 1)));
    imagePos.setY(qBound(0.0, imagePos.y(), double(m_originalImage.height() - 1)));

    return imagePos;
}

QPoint InteractiveImageLabel::imageToWidget(const QPointF &imagePos) const
{
    if (!m_hasImage)
        return QPoint();

    return (imagePos * m_zoomFactor + m_imageOffset * m_zoomFactor).toPoint();
}

QRectF InteractiveImageLabel::visibleImageRect() const
{
    if (!m_hasImage)
        return QRectF();

    QPointF topLeft = widgetToImage(QPoint(0, 0));
    QPointF bottomRight = widgetToImage(QPoint(width(), height()));

    return QRectF(topLeft, bottomRight).normalized();
}

void InteractiveImageLabel::clampImagePosition()
{
    if (!m_hasImage || m_originalImage.isNull())
        return;

    QSize widgetSize = size();
    QSizeF imageSize = m_originalImage.size() * m_zoomFactor;

    // 计算最大偏移量
    double maxOffsetX = qMax(0.0, (imageSize.width() - widgetSize.width()) / (2 * m_zoomFactor));
    double maxOffsetY = qMax(0.0, (imageSize.height() - widgetSize.height()) / (2 * m_zoomFactor));

    // 如果图像比控件小，居中显示
    if (imageSize.width() <= widgetSize.width()) {
        m_imageOffset.setX((widgetSize.width() - imageSize.width()) / (2 * m_zoomFactor));
    } else {
        // 限制在合理范围内
        m_imageOffset.setX(qBound(-maxOffsetX, m_imageOffset.x(), maxOffsetX));
    }

    if (imageSize.height() <= widgetSize.height()) {
        m_imageOffset.setY((widgetSize.height() - imageSize.height()) / (2 * m_zoomFactor));
    } else {
        m_imageOffset.setY(qBound(-maxOffsetY, m_imageOffset.y(), maxOffsetY));
    }
}

void InteractiveImageLabel::emitMousePositionInfo(const QPoint &widgetPos)
{
    if (!m_hasImage)
        return;

    QPointF imagePos = widgetToImage(widgetPos);
    QRect imageRect = m_originalImage.rect();

    if (imageRect.contains(imagePos.toPoint())) {
        int pixelValue = getGrayValue(
            getPixelColor(m_originalImage, int(imagePos.x()), int(imagePos.y())));
        emit mouseMoved(imagePos.toPoint(), pixelValue);
    }
}

QColor InteractiveImageLabel::getPixelColor(const QImage &image, int x, int y)
{
    if (image.isNull() || x < 0 || x >= image.width() || y < 0 || y >= image.height()) {
        return QColor();
    }

    return QColor(image.pixel(x, y));
}

int InteractiveImageLabel::getGrayValue(const QColor &color)
{
    if (!color.isValid())
        return -1;
    return qGray(color.rgb());
}
