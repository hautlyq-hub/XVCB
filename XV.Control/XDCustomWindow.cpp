
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include "XDCustomWindow.h"

QHash<QDialogButtonBox::StandardButton, QString> XDMessageBox::m_buttonsStyleSheet;
QString XDMessageBox::m_titleStyleSheet;
XDMessageBox::XDMessageBox(QWidget *parent,
                                       const QString &title,
                                       const QString &text,
                                       QMessageBox::StandardButtons buttons,
                                       QMessageBox::StandardButton defaultButton)
    : XDShadowWindow<QDialog>(false, 10, parent)
{
    if (parent != nullptr) {
        setWindowIcon(parent->windowIcon());
    } else {
        this->setWindowIcon(QIcon(MV_APP_ICON));
    }
    this->setWindowTitle(title);

    setResizable(false);
    titleBar()->setMinimumVisible(false);
    titleBar()->setMaximumVisible(false);

    m_pButtonBox = new QDialogButtonBox(this);
    m_pButtonBox->setStandardButtons(QDialogButtonBox::StandardButtons(int(buttons)));
    setDefaultButton(defaultButton);

    m_pIconLabel = new QLabel(this);
    m_pLabel = new QLabel(this);
    initStyle();

    m_pIconLabel->setFixedSize(35, 35);
    m_pIconLabel->setScaledContents(true);

    m_pLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_pLabel->setObjectName("messageTextLabel");
    m_pLabel->setOpenExternalLinks(true);
    m_pLabel->setText(text);

    QWidget *pClientWidget = new QWidget(this);
    m_pGridLayout = new QGridLayout(pClientWidget);
    m_pGridLayout->addWidget(m_pIconLabel, 0, 0, 2, 1, Qt::AlignTop);
    m_pGridLayout->addWidget(m_pLabel, 0, 1, 2, 1);
    m_pGridLayout->addWidget(m_pButtonBox, m_pGridLayout->rowCount(), 0, 1, m_pGridLayout->columnCount());
    m_pGridLayout->setSizeConstraint(QLayout::SetNoConstraint);
    m_pGridLayout->setHorizontalSpacing(10);
    m_pGridLayout->setVerticalSpacing(10);
    m_pGridLayout->setContentsMargins(10, 10, 10, 10);
    clientLayout()->addWidget(pClientWidget);

    translateUI();


    connect(m_pButtonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onButtonClicked(QAbstractButton*)));
}

QAbstractButton *XDMessageBox::clickedButton() const
{
    return m_pClickedButton;
}

QMessageBox::StandardButton XDMessageBox::standardButton(QAbstractButton *button) const
{
    return (QMessageBox::StandardButton)m_pButtonBox->standardButton(button);
}

void XDMessageBox::setDefaultButton(QPushButton *button)
{
    if (!m_pButtonBox->buttons().contains(button))
        return;

    m_pDefaultButton = button;
    button->setDefault(true);
    button->setFocus();
}

void XDMessageBox::setDefaultButton(QMessageBox::StandardButton button)
{
    setDefaultButton(m_pButtonBox->button(QDialogButtonBox::StandardButton(button)));
}

void XDMessageBox::setTitle(const QString &title)
{
    setWindowTitle(title);
}

void XDMessageBox::setText(const QString &text)
{
    m_pLabel->setText(text);
}

void XDMessageBox::setIcon(const QString &icon)
{
    m_pIconLabel->setPixmap(QPixmap(icon));
}

void XDMessageBox::addWidget(QWidget *pWidget)
{
    m_pLabel->hide();
    m_pGridLayout->addWidget(pWidget, 0, 1, 2, 1);
}

QLabel *XDMessageBox::titleLabel() const
{
    return m_pLabel;
}

QDialogButtonBox *XDMessageBox::buttonBox() const
{
    return m_pButtonBox;
}

QMessageBox::StandardButton XDMessageBox::showInformation(QWidget *parent,
                                                                const QString &title,
                                                                const QString &text,
                                                                QMessageBox::StandardButton buttons)
{
    XDMessageBox msgBox(parent, tr("XPhaseContrast"), text, buttons);
    msgBox.setIcon(XV_APP_MSG_INFO);
    if (msgBox.exec() == -1)
        return QMessageBox::Cancel;
    return msgBox.standardButton(msgBox.clickedButton());
}

QMessageBox::StandardButton XDMessageBox::showError(QWidget *parent,
                                                          const QString &title,
                                                          const QString &text,
                                                          QMessageBox::StandardButtons buttons,
                                                          QMessageBox::StandardButton defaultButton)
{
    XDMessageBox msgBox(parent, tr("XPhaseContrast"), text, buttons, defaultButton);
    msgBox.setIcon(XV_APP_MSG_ERROR);
    if (msgBox.exec() == -1)
        return QMessageBox::Cancel;
    return msgBox.standardButton(msgBox.clickedButton());
}

QMessageBox::StandardButton XDMessageBox::showSuccess(QWidget *parent,
                                                            const QString &title,
                                                            const QString &text,
                                                            QMessageBox::StandardButtons buttons,
                                                            QMessageBox::StandardButton defaultButton)
{
    XDMessageBox msgBox(parent, tr("XPhaseContrast"), text, buttons, defaultButton);
    msgBox.setIcon(XV_APP_MSG_SUCCESS);
    if (msgBox.exec() == -1)
        return QMessageBox::Cancel;
    return msgBox.standardButton(msgBox.clickedButton());
}

QMessageBox::StandardButton XDMessageBox::showQuestion(QWidget *parent,
                                                             const QString &title,
                                                             const QString &text,
                                                             QMessageBox::StandardButtons buttons,
                                                             QMessageBox::StandardButton defaultButton)
{
    XDMessageBox msgBox(parent, tr("XPhaseContrast"), text, buttons, defaultButton);
    msgBox.setIcon(XV_APP_MSG_QUESTION);
		msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint);
    if (msgBox.exec() == -1)
        return QMessageBox::Cancel;
    return msgBox.standardButton(msgBox.clickedButton());
}

QMessageBox::StandardButton XDMessageBox::showWarning(QWidget *parent,
                                                            const QString &title,
                                                            const QString &text,
                                                            QMessageBox::StandardButtons buttons,
                                                            QMessageBox::StandardButton defaultButton)
{
    XDMessageBox msgBox(parent, tr("XPhaseContrast"), text, buttons, defaultButton);
    msgBox.setIcon(XV_APP_MSG_WARNING);
    if (msgBox.exec() == -1)
        return QMessageBox::Cancel;
    return msgBox.standardButton(msgBox.clickedButton());
}

QMessageBox::StandardButton XDMessageBox::showCritical(QWidget *parent,
                                                             const QString &title,
                                                             const QString &text,
                                                             QMessageBox::StandardButtons buttons,
                                                             QMessageBox::StandardButton defaultButton)
{
    XDMessageBox msgBox(parent, tr("XPhaseContrast"), text, buttons, defaultButton);
    msgBox.setIcon(XV_APP_MSG_WARNING);
    if (msgBox.exec() == -1)
        return QMessageBox::Cancel;
    return msgBox.standardButton(msgBox.clickedButton());
}

QMessageBox::StandardButton XDMessageBox::showCheckBoxQuestion(QWidget *parent,
                                                                     const QString &title,
                                                                     const QString &text,
                                                                     QMessageBox::StandardButtons buttons,
                                                                     QMessageBox::StandardButton defaultButton)
{
    XDMessageBox msgBox(parent, title, text, buttons, defaultButton);
    msgBox.setIcon(XV_APP_MSG_QUESTION);

    QCheckBox *pCheckBox = new QCheckBox(&msgBox);
    pCheckBox->setText(text);
    msgBox.addWidget(pCheckBox);
    if (msgBox.exec() == -1)
        return QMessageBox::Cancel;

    QMessageBox::StandardButton standardButton = msgBox.standardButton(msgBox.clickedButton());
    if (standardButton == QMessageBox::Yes) {
        return pCheckBox->isChecked() ? QMessageBox::Yes : QMessageBox::No;
    }
    return QMessageBox::Cancel;
}

void XDMessageBox::setButtonStyleSheet(QDialogButtonBox::StandardButton button, const QString &styleSheet)
{
    m_buttonsStyleSheet[button] = styleSheet;
}

void XDMessageBox::setTitleStyleSheet(const QString &styleSheet)
{
    m_titleStyleSheet = styleSheet;
}

void XDMessageBox::onButtonClicked(QAbstractButton *button)
{
    m_pClickedButton = button;
    done(execReturnCode(button));
}

int XDMessageBox::execReturnCode(QAbstractButton *button)
{
    int nResult = m_pButtonBox->standardButton(button);
    return nResult;
}

void XDMessageBox::translateUI()
{
    QPushButton *pYesButton = m_pButtonBox->button(QDialogButtonBox::Yes);
    if (pYesButton != nullptr)
        pYesButton->setText(tr("Yes"));

    QPushButton *pNoButton = m_pButtonBox->button(QDialogButtonBox::No);
    if (pNoButton != nullptr)
        pNoButton->setText(tr("No"));

    QPushButton *pOkButton = m_pButtonBox->button(QDialogButtonBox::Ok);
    if (pOkButton != nullptr)
        pOkButton->setText(tr("Ok"));

    QPushButton *pCancelButton = m_pButtonBox->button(QDialogButtonBox::Cancel);
    if (pCancelButton != nullptr)
        pCancelButton->setText(tr("Cancel"));
}

void XDMessageBox::initStyle()
{
    QPushButton *pButton = nullptr;
    for (auto key : m_buttonsStyleSheet.keys()) {
        pButton = m_pButtonBox->button(key);
        if (pButton != nullptr)
            pButton->setStyleSheet(m_buttonsStyleSheet.value(key));
    }
	this->setStyleSheet(m_titleStyleSheet);
   // titleBar()/*->titleLabel()*/->setStyleSheet(m_titleStyleSheet);
}

