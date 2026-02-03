#ifndef XDTITLEBAR_H
#define XDTITLEBAR_H

#include <QtWidgets/QWidget>

class QLabel;
class QPushButton;
class XDTitleBar : public QWidget
{
    Q_OBJECT
public:
    XDTitleBar(QWidget *parent, QWidget *window, QWidget *shadowContainerWidget, bool canResize);
    ~XDTitleBar();

    /**
     * @brief setMinimumVisible 设置最小化按钮是否可见
     * @param minimum
     */
    void setMinimumVisible(bool minimum);
    /**
     * @brief setMaximumVisible 设计最大化还原按钮是否可见
     * @param maximum
     */
    void setMaximumVisible(bool maximum);

    /**
     * @brief setTitleHeight
     *  改变标题栏的高度
     *  \warning
     *  如果通过其它函数改变标题栏高度，标题栏的某些区域可能不能拖动窗口
     *  或者标题栏以外的区域可能可以拖动窗口
     * @param height
     */
    void setTitleHeight(int height);

    /**
     * @brief customWidget
     *  自定义添加内容。除按钮图标标题之外的widget
     * @return
     */
    QWidget *customWidget() const;

    QPushButton *minimizeButton() const;
    QPushButton *maximizeButton() const;
    QPushButton *closeButton() const;
    QLabel *titleLabel() const;

    /**
     * @brief oldSize
     * @return
     */
    QSize oldSize() const;

protected:
    virtual void paintEvent(QPaintEvent *e);

    /**
     * @brief mouseDoubleClickEvent 双击标题栏进行界面的最大化/还原
     * @param event QMouseEvent *
     */
    virtual void mouseDoubleClickEvent(QMouseEvent *event);   

    /**
     * @brief eventFilter 设置界面标题与图标
     * @param obj
     * @param event
     * @return
     *  bool
     */
    virtual bool eventFilter(QObject *obj, QEvent *event);

private slots:
    /**
     * @brief onClicked 进行最小化、最大化/还原、关闭操作
     */
    void onClicked();

signals:
    void ShowMaximized();
    void ShowNormal();
    void HeightChanged(int height);

private:
    /**
     * @brief updateMaximize update the button status
     */
    void updateMaximize();

private:
    QLabel *m_pIconLabel;
    QLabel *m_pTitleLabel;
    QPushButton *m_pMinimizeButton;
    QPushButton *m_pMaximizeButton;
    QPushButton *m_pCloseButton;
    QWidget *m_pCustomWidget; // 图标，标题，最大最小关闭按钮之外，自定义添加的内容
    QWidget* m_window;
    QWidget* m_shadowContainerWidget;
    QMargins m_oldContentsMargin;
    bool m_canResize;
    QSize m_oldSize; // the size before Maximized
};

#endif // XDTITLEBAR_H
