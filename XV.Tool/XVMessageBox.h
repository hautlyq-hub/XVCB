#ifndef XVMESSAGEBOX_H
#define XVMESSAGEBOX_H

#include <QDialog>

#include <QLabel>
#include <QPushButton>

#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QStyle>

#include <QEvent>
#include <QWindowStateChangeEvent>

class XVMessageBox : public QDialog
{
    Q_OBJECT
public:
    enum Icon { NoIcon = 0, Information = 1, Warning = 2, Critical = 3, Question = 4 };

    enum Button { Ok = 0x00000400, Cancel = 0x00400000, Yes = 0x00004000, No = 0x00010000 };

    static int Show(QWidget* parent,
                    const QString& title,
                    const QString& text,
                    Icon icon = NoIcon,
                    int buttons = Ok | Cancel);

public:
    explicit XVMessageBox(QWidget* parent = nullptr);
    void SetupUI(const QString& title, const QString& text, Icon icon, int buttons);

protected:
    // 重写事件处理函数防止最小化
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    QLabel* iconLabel;
    QLabel* textLabel;
    QPushButton* okButton;
    QPushButton* cancelButton;
    QPushButton* yesButton;
    QPushButton* noButton;
    int clickedButton = 0;
};

#endif // XVMESSAGEBOX_H
