#include "XVMessageBox.h"

int XVMessageBox::Show(
    QWidget *parent, const QString &title, const QString &text, Icon icon, int buttons)
{
    XVMessageBox msgBox(parent);
    msgBox.SetupUI(title, text, icon, buttons);
    msgBox.exec();
    return msgBox.clickedButton;
}

XVMessageBox::XVMessageBox(QWidget *parent)
    : QDialog(parent)
{}

void XVMessageBox::SetupUI(const QString &title, const QString &text, Icon icon, int buttons)
{
    setWindowTitle(title);
    // 创建控件
    iconLabel = new QLabel(this);
    textLabel = new QLabel(text, this);
    textLabel->setWordWrap(true);
    QFont font = this->font();
    font.setPointSize(font.pointSize() + 2);
    textLabel->setFont(font);
    // 设置图标
    QStyle *style = this->style();
    QIcon ico;
    switch (icon) {
    case Information:
        ico = style->standardIcon(QStyle::SP_MessageBoxInformation);
        break;
    case Warning:
        ico = style->standardIcon(QStyle::SP_MessageBoxWarning);
        break;
    case Critical:
        ico = style->standardIcon(QStyle::SP_MessageBoxCritical);
        break;
    case Question:
        ico = style->standardIcon(QStyle::SP_MessageBoxQuestion);
        break;
    default:
        break;
    }
    iconLabel->setPixmap(ico.pixmap(48, 48));
    // 创建按钮
    auto createButton = [this, font](Button type, const QString &text) {
        QPushButton *btn = new QPushButton(text, this);
        btn->setFont(font);
        btn->setFixedSize(QSize(80, 60));
        connect(btn, &QPushButton::clicked, [this, type] {
            clickedButton = type;
            accept();
        });
        return btn;
    };
    if (buttons & Ok) {
        okButton = createButton(Ok, tr("OK"));
    }
    if (buttons & Cancel) {
        cancelButton = createButton(Cancel, tr("Cancel"));
    }
    if (buttons & Yes) {
        yesButton = createButton(Yes, tr("Yes"));
    }
    if (buttons & No) {
        noButton = createButton(No, tr("No"));
    }
    // 布局
    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->addWidget(iconLabel, 0, Qt::AlignTop);
    contentLayout->addWidget(textLabel, 1);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    if (buttons & Ok) {
        buttonLayout->addWidget(okButton);
    }
    if (buttons & Yes) {
        buttonLayout->addWidget(yesButton);
    }
    if (buttons & No) {
        buttonLayout->addWidget(noButton);
    }
    if (buttons & Cancel) {
        buttonLayout->addWidget(cancelButton);
    }
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(contentLayout);
    mainLayout->addLayout(buttonLayout);
}

void XVMessageBox::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent *stateEvent = static_cast<QWindowStateChangeEvent *>(event);
        if (stateEvent->oldState() != Qt::WindowMinimized && isMinimized()) {
            // 立即恢复窗口状态
            // showNormal();
            // 立即取消最小化操作
            QMetaObject::invokeMethod(
                this,
                [this] { setWindowState(windowState() & ~Qt::WindowMinimized); },
                Qt::QueuedConnection);
        }
        if (stateEvent->oldState() != Qt::WindowMaximized && isMaximized()) {
            // showNormal();
            // 立即取消最小化操作
            QMetaObject::invokeMethod(
                this,
                [this] { setWindowState(windowState() & ~Qt::WindowMaximized); },
                Qt::QueuedConnection);
        }
    } else {
        QDialog::changeEvent(event);
    }
}

void XVMessageBox::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
}
