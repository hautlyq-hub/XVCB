#include "mvaboutwidget.h"
#include <QDesktopServices>
#include <QUrl>



namespace mv
{
    mvAboutWidget::mvAboutWidget(QWidget *parent)
        : QDialog(parent)
    {
        ui.setupUi(this);

        setWindowFlags(Qt::FramelessWindowHint);
        this->setObjectName("mAboutWidget");

//        QString _version = cx::ConfigFileManager::getInstance()->GetValue("XepectVision/version");
//        ui.lbVisionValue->setText(_version);
//        QString _emile = cx::ConfigFileManager::getInstance()->GetValue("XepectVision/emile");
//        ui.lbsuportValue->setText(_emile);
//        QString _AuthorizeDate = cx::ConfigFileManager::getInstance()->mAuthorizeDate;
//        ui.mAuthorizeDate->setText(_AuthorizeDate);

        connect(ui.mBtnSpecification, SIGNAL(clicked()), this, SLOT(onSpecificationClicked()));
    }

    mvAboutWidget::~mvAboutWidget()
    {
    }
    void mvAboutWidget::onSpecificationClicked()
    {
        isChecked = !isChecked;
        if (!isChecked)
        {
            ui.stackedWidget->setCurrentIndex(0);
        }
        else
        {
            ui.stackedWidget->setCurrentIndex(1);
        }

    }
    void mvAboutWidget::paintEvent(QPaintEvent *event)
    {
        QWidget::paintEvent(event);
    }
}
