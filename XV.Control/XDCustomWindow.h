#ifndef XDCUSTOMWINDOW_H
#define XDCUSTOMWINDOW_H

#include <QtWidgets/QDialogButtonBox>
#include <QHash>
#include "XDShadowWindow.h"
#include "XV.Tool/mvconfig.h"


class XDMessageBox : public XDShadowWindow<QDialog>
{
    Q_OBJECT
public:
    XDMessageBox(QWidget *parent = nullptr,
                                const QString &title = tr("Tip"),
                                const QString &text = "",
                                QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                QMessageBox::StandardButton defaultButton = QMessageBox::Ok);

    QAbstractButton *clickedButton() const;
    QMessageBox::StandardButton standardButton(QAbstractButton *button) const;

    void setDefaultButton(QPushButton *button);
    void setDefaultButton(QMessageBox::StandardButton button);

    void setTitle(const QString &title);
    void setText(const QString &text);
    void setIcon(const QString &icon);
    void addWidget(QWidget *pWidget);

    QLabel *titleLabel() const;

    QDialogButtonBox *buttonBox() const;

	static QMessageBox::StandardButton showInformation(QWidget *parent,
                                                       const QString &title,
                                                       const QString &text,
                                                       QMessageBox::StandardButton buttons = QMessageBox::Ok);

	static QMessageBox::StandardButton showError(QWidget *parent,
                                                 const QString &title,
                                                 const QString &text,
                                                 QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                                 QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

	static QMessageBox::StandardButton showSuccess(QWidget *parent,
                                                   const QString &title,
                                                   const QString &text,
                                                   QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                                   QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

	static QMessageBox::StandardButton showQuestion(QWidget *parent,
                                                    const QString &title,
                                                    const QString &text,
                                                    QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No,
                                                    QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

	static QMessageBox::StandardButton showWarning(QWidget *parent,
                                                   const QString &title,
                                                   const QString &text,
                                                   QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                                   QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

	static QMessageBox::StandardButton showCritical(QWidget *parent,
                                                    const QString &title,
                                                    const QString &text,
                                                    QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                                    QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

	static QMessageBox::StandardButton showCheckBoxQuestion(QWidget *parent,
                                                            const QString &title,
                                                            const QString &text,
                                                            QMessageBox::StandardButtons buttons,
                                                            QMessageBox::StandardButton defaultButton);

public:

    static void setButtonStyleSheet(QDialogButtonBox::StandardButton button, const QString &styleSheet);

    static void setTitleStyleSheet(const QString &styleSheet);

private slots:
    void onButtonClicked(QAbstractButton *button);

private:
    int execReturnCode(QAbstractButton *button);
    void translateUI();
    void initStyle();

private:
    QLabel *m_pIconLabel;
    QLabel *m_pLabel;
    QGridLayout *m_pGridLayout;
    QDialogButtonBox *m_pButtonBox;
    QAbstractButton *m_pClickedButton;
    QAbstractButton *m_pDefaultButton;

    static QHash<QDialogButtonBox::StandardButton, QString> m_buttonsStyleSheet;
    static QString m_titleStyleSheet;
};

#endif // XDCUSTOMWINDOW_H
