#include "mvloginwindow.h"
#include <QtCore/QDebug>
#include "mvaboutwidget.h"

#include "XV.Tool/mvdatalocations.h"

#include <QBitmap>
#include <QImage>
#include <QMap>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QtCore/QTimer>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace mv
{
	mvLoginWindow *mvLoginWindow::mInstance = NULL;
	mvLoginWindow* mvLoginWindow::getInstance()
	{
		if (mInstance == NULL)
		{
			mInstance = new mvLoginWindow();
		}
		return mInstance;
	}
	mvLoginWindow::mvLoginWindow(QWidget *parent)
		:QDialog(parent)
	{
		ui.setupUi(this);
		this->setObjectName("mLoginWidget");
        setWindowFlags(Qt::FramelessWindowHint);

        initView();
        initConnect();

        readSettings();

	}

	mvLoginWindow::~mvLoginWindow()
	{

	}

   void mvLoginWindow::initView()
   {
       QRegularExpression rx("[a-zA-Z0-9]{3,10}");

       QRegularExpressionValidator *validator = new QRegularExpressionValidator(rx, this);
       ui.leUid->setValidator(validator);
       ui.leUid->setView(new QListView());
       ui.lePwd->setContextMenuPolicy(Qt::NoContextMenu);
       ui.lePwd->setPlaceholderText(tr("Password"));
       ui.lePwd->setEchoMode(QLineEdit::Password);
       ui.lePwd->setMaxLength(10);

       ui.pbLogin->setDefault(true);

   }

   void mvLoginWindow::initConnect()
   {
       connect(ui.pbLogin, SIGNAL(clicked()), this, SLOT(loginButton()));
       connect(ui.mBtnCloseLoginWidget, SIGNAL(clicked()), this, SLOT(close()));

   }

   void mvLoginWindow::readSettings()
   {
       QString organization = "Xpect Vision";
       QString appName = "XVBVThickness";
       QSettings settings(organization, appName);
       bool saved = settings.value("Saved", false).toBool();
       m_InUser = settings.value("UserName").toString();
       m_InUsers = settings.value("UserNames").toString();
       m_InUserList = m_InUsers.split("|");

       m_InPswd = settings.value("Password", "QKBg6Ov68iyTcTLrYvyyCg==").toString();
       ui.leUid->addItems(m_InUserList);

       if (saved)
       {
           QString DecrptPswd = SimpleCrypto::decrypt(m_InPswd);
           ui.leUid->setCurrentText(m_InUser);
           ui.lePwd->setText(DecrptPswd);
           ui.mCheckBoxRemmberPassword->setChecked(saved);
       }
       else
       {
           ui.leUid->setCurrentText("");
           ui.lePwd->setText("");
           ui.mCheckBoxRemmberPassword->setChecked(saved);
       }

   }

   void mvLoginWindow::writeSettings()
   {
       QSettings settings("Xpect Vision", "XVBVThickness");
       settings.setValue("UserName", m_InUser);
       settings.setValue("Password", m_InPswd);
       bool isExistUser = true;
       for (int i = 0; i < m_InUserList.length(); i++)
       {
           if (m_InUser == m_InUserList.at(i))
           {
               isExistUser = false;
           }
       }
       if (isExistUser)
       {
           settings.setValue("UserNames", m_InUsers + "|" + m_InUser);
       }

       settings.setValue("Saved", ui.mCheckBoxRemmberPassword->isChecked());//保存用户名按钮被勾选后

   }

   void mvLoginWindow::paintEvent(QPaintEvent *event)
   {
       QPainterPath path;
       path.setFillRule(Qt::WindingFill);
       path.addRect(3, 3, width() - 6, height() - 6);

       QPainter painter(this);
       painter.setRenderHint(QPainter::Antialiasing, true);
       painter.fillPath(path, QBrush(QColor(255, 255, 255, 255)));
       QColor color(0, 0, 0);
       for (int i = 0; i<3; i++)
       {
           QPainterPath path;
           path.setFillRule(Qt::WindingFill);
           path.addRect(3 - i, 3 - i, this->width() - (3 - i) * 2, this->height() - (3 - i) * 2);
           color.setAlpha(150 - qSqrt(i) * 50);
           painter.setPen(color);
           painter.drawPath(path);
       }
   }

   void mvLoginWindow::closeEvent(QCloseEvent * event)
   {
       done(Rejected);
   }

   bool mvLoginWindow::eventFilter(QObject *target, QEvent *event)
   {
       return QWidget::eventFilter(target, event);
   }


   void mvLoginWindow::mouseDoubleClickEvent(QMouseEvent *event)
   {
       Q_UNUSED(event);
       //this->isMaximized() ? this->showNormal() : this->showMaximized();
       //event->accept();
   }

   void mvLoginWindow::mousePressEvent(QMouseEvent *event)
   {
       Q_UNUSED(event);

       mIsPressed = true;
       mStartMovePos = event->globalPos();

       event->accept();
   }

   void mvLoginWindow::mouseMoveEvent(QMouseEvent *event)
   {
       if (mIsPressed)
       {
           QPoint movePoint = event->globalPos() - mStartMovePos;
           QPoint widgetPos = this->pos();
           mStartMovePos = event->globalPos();

           this->move(widgetPos.x() + movePoint.x(), widgetPos.y() + movePoint.y());
       }

       event->accept();
   }

   void mvLoginWindow::mouseReleaseEvent(QMouseEvent *event)
   {
       mIsPressed = false;

       event->accept();
   }

   void mvLoginWindow::loginButton()
   {
       if (checkStateHardDisk(DataLocations::getDicomPath()) == 0)
       {
           return;
       }

       QString lbUser = ui.leUid->currentText().trimmed();
       QString lbPswd = ui.lePwd->text().trimmed();
       QString EncrptLnPswd = SimpleCrypto::encrypt(lbPswd);

       if (DicomDatabase::instance()->isExistsUseNameFormUserName(lbUser))
       {
           m_DisplayName = DicomDatabase::instance()->getUserDisplayNameFormUserName(lbUser);
           QString mDbLnPswd = DicomDatabase::instance()->getPasswordFormUserName(lbUser);
           if (EncrptLnPswd == mDbLnPswd)
           {
               QStringList LoginInfos = DicomDatabase::instance()->getUserLoginInfoFormUserName(lbUser);

               SessionHelper* session = SessionHelper::getInstance();
               session->setCurrentUserInfo(
                   LoginInfos.at(0),  // UserID
                   LoginInfos.at(1),  // UserName
                   LoginInfos.at(2),  // DisplayName
                   LoginInfos.at(3),  // Token
                   LoginInfos.at(4),  // Status
                   LoginInfos.at(5),  // RoleID
                   LoginInfos.at(6),  // RoleName
                   LoginInfos.at(7)   // RoleDescription
               );

               QString LoginID = QUuid::createUuid().toString().remove("{").remove("}");
               DicomDatabase::instance()->insertUserLogin(LoginID, LoginInfos.at(0), QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"), "");

               m_InUser = lbUser;
               m_InPswd = EncrptLnPswd;
               ui.mLbPasswordError->setText("");
               ui.pbLogin->setDefault(false);
               writeSettings();

               done(Accepted);

               return;
           }
           else
           {
               ui.mLbPasswordError->setText(tr("account or password error "));
           }
       }
       else
       {
           ui.mLbPasswordError->setText(tr("The user name does not exist "));
       }

       QSettings settings("Xpect Vision", "XVBVThickness");
       settings.setValue("Saved", false);
   }

   int mvLoginWindow::checkStateHardDisk(QString sPath)
   {
       QDir dir(sPath);
       if (!dir.exists()
           || sPath.isEmpty())
       {
           QMessageBox::critical(this, "", QObject::tr("Dicom storage path does not exist, please use System Settings ->advanced setting ->Local database configuration->DICOM file cache address change path !"));
           return -1;
       }
       else
       {
           HardDiskInformation diskInfo;
           quint64 nFreeSize = diskInfo.getNumberOfFreeMBytes(sPath) / 1024;
           if (nFreeSize < 2)
           {
               QMessageBox::critical(this, "", QObject::tr("The available capacity of Disk %1 is insufficient, please clean up the disk, thank you!").arg(sPath.at(0)));
               return 0;
           }
           else if (nFreeSize < 5)
           {
               QMessageBox::warning(this, "", QObject::tr("The available capacity of Disk %1 is less than 5G, please pay attention to cleaning up the disk to avoid affecting the use of the software. Thank you.").arg(sPath.at(0)));
               return 1;
           }
           else
           {
               return 2;
           }
       }
   }
}
