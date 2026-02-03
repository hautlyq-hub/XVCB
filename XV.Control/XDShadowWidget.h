#ifndef XDSHADOWWIDGET_H
#define XDSHADOWWIDGET_H

#include <QtWidgets/QWidget>

class XDSkin9GridImage
{
protected:
    QImage m_img;
    QRect m_arrayImageGrid[9];
    //
    bool clear();
public:
    static bool splitRect(const QRect& rcSrc, QPoint ptTopLeft, QRect* parrayRect, int nArrayCount);
    bool setImage(const QImage& image, QPoint ptTopLeft);
    void drawBorder(QPainter* p, QRect rc) const;
};


class XDShadowWidget : public QWidget
{
    Q_OBJECT
public:
    XDShadowWidget(int shadowSize, bool canResize, QWidget *parent = nullptr);

    virtual void paintEvent(QPaintEvent *e);

private:
    XDSkin9GridImage* m_shadow;
    int m_shadowSize;
};

#endif // XDSHADOWWIDGET_H
