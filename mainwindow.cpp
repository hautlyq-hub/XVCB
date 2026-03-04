#include "mainwindow.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("BVThickness");
    this->setObjectName("MainWindow");

     setWindowFlags(Qt::FramelessWindowHint);

    initView();
    initConnect();
    initQSS();
    initDB();

#ifdef QT_NO_DEBUG // Release模式
#define WIDGETWIDTH (QGuiApplication::primaryScreen()->availableGeometry().width())
#define WIDGETHEIGHT (QGuiApplication::primaryScreen()->availableGeometry().height())
    showMaximized();
#else
#define WIDGETWIDTH 1024 // Debug模式用稍小的尺寸
#define WIDGETHEIGHT 600
#endif

#define LEFT_TITLE_WIDTH 80
#define LEFT_TITLE_MIN_BUTTON_HEIGHT 35
#define LEFT_TITLE_MAX_BUTTON_HEIGHT 60
#define TOP_TITLE_HEIGHT 40

}

MainWindow::~MainWindow()
{

}

void MainWindow::initView()
{
    QFont font;
    font.setFamily("Microsoft YaHei");
    font.setPointSize(10);

    mNew =  tr("    New");
    mEdit = tr("    Edit");;
    mList = tr("    List");;
    mMain = tr("    Main");;
    mSet =  tr("    Set");;

    mCentralWidget = new QWidget();
    SET_OBJ_NAME(mCentralWidget);


    mvSystemSettingsWidget = new mvsystemsettings();
    SET_OBJ_NAME(mvSystemSettingsWidget);


    mPatientRecordWidget = new mvPatientRecordWidget();

    mNewPatientDialog = new mvNewPatientDialog();

    mAboutWidget = new mvAboutWidget();
    mAboutWidget->setMinimumSize(QSize(WIDGETWIDTH - LEFT_TITLE_WIDTH, WIDGETHEIGHT -LEFT_TITLE_MIN_BUTTON_HEIGHT));

    mTitleWidget = new QWidget();
    SET_OBJ_NAME(mTitleWidget);
    mTitleWidget->setStyleSheet("background-color: qlineargradient(spread:reflect, x1:0.0, y1:0.0, x2:0.0, y2:1.0, stop:0 rgba(255,255,255,255), stop:1 rgba(200, 200, 200, 255))");//200, 240, 255, 255
    mTitleWidget->setMinimumHeight(TOP_TITLE_HEIGHT);

    mStatusBar = new StatusBar();
    SET_OBJ_NAME(mStatusBar);
    QHBoxLayout *hTitleLayout = new QHBoxLayout();
    hTitleLayout->setContentsMargins(0, 0, 0, 0);
    hTitleLayout->setSpacing(0);
    hTitleLayout->addWidget(mStatusBar);
    mTitleWidget->setLayout(hTitleLayout);

    mImageAcquisitWidget = new mvImageAcquisitWidget();
    SET_OBJ_NAME(mImageAcquisitWidget);

    mLeftWidget = new QWidget();
    SET_OBJ_NAME(mLeftWidget);
    mLeftWidget->setMinimumWidth(LEFT_TITLE_WIDTH);

    QLabel* LogoLabel = new QLabel();
    SET_OBJ_NAME(LogoLabel);
    LogoLabel->setMinimumSize(LEFT_TITLE_WIDTH - 20, LEFT_TITLE_MIN_BUTTON_HEIGHT - 19);

    QHBoxLayout* hLayout1 = new QHBoxLayout();
    hLayout1->addStretch();
    hLayout1->addWidget(LogoLabel);
    hLayout1->addStretch();

    mBtnLogin = new QPushButton();
    SET_OBJ_NAME(mBtnLogin);
    mBtnLogin->setText(tr("Login"));
    mBtnLogin->setFont(font);

    mBtnExitLogin = new QPushButton();
    SET_OBJ_NAME(mBtnExitLogin);
    mBtnExitLogin->setText(tr("ExitLogin"));
    mBtnExitLogin->setFont(font);
//    mBtnExitLogin->setIcon(QPixmap(":/Res/Image/mainwidget/253.png"));
    mBtnExitLogin->setIconSize(QSize(30,LEFT_TITLE_MIN_BUTTON_HEIGHT));
    mBtnExitLogin->setLayoutDirection(Qt::RightToLeft);

    mLoginStackWidget = new QStackedWidget();
    SET_OBJ_NAME(mLoginStackWidget);
    mLoginStackWidget->setFixedSize(QSize(LEFT_TITLE_WIDTH, LEFT_TITLE_MIN_BUTTON_HEIGHT));
    mLoginStackWidget->setContentsMargins(QMargins(10, 0, 10, 0));
    mLoginStackWidget->addWidget(mBtnLogin);
    mLoginStackWidget->addWidget(mBtnExitLogin);
    QHBoxLayout* hLayout2 = new QHBoxLayout();
    hLayout2->setContentsMargins(0, 0, 0, 0);
    hLayout2->addStretch();
    hLayout2->addWidget(mLoginStackWidget);
    hLayout2->addStretch();
    mBtnNewCheck = new QPushButton();
    SET_OBJ_NAME(mBtnNewCheck);
    mBtnNewCheck->setText(mNew);
    mBtnNewCheck->setFont(font);
    mBtnNewCheck->setFixedSize(QSize(LEFT_TITLE_WIDTH, LEFT_TITLE_MAX_BUTTON_HEIGHT));
    mBtnNewCheck->setCheckable(true);

    mBtnPatientRecord = new QPushButton();
    SET_OBJ_NAME(mBtnPatientRecord);
    mBtnPatientRecord->setText(mList);
    mBtnPatientRecord->setFont(font);
    mBtnPatientRecord->setFixedSize(QSize(LEFT_TITLE_WIDTH,  LEFT_TITLE_MAX_BUTTON_HEIGHT));
    mBtnPatientRecord->setCheckable(true);

    mBtnExp = new QPushButton();
    SET_OBJ_NAME(mBtnExp);
    mBtnExp->setText(mMain);
    mBtnExp->setFont(font);
    mBtnExp->setFixedSize(QSize(LEFT_TITLE_WIDTH,  LEFT_TITLE_MAX_BUTTON_HEIGHT));
    mBtnExp->setCheckable(true);

    mBtnSysSettings = new QPushButton();
    SET_OBJ_NAME(mBtnSysSettings);
    mBtnSysSettings->setText(mSet);
    mBtnSysSettings->setFont(font);
    mBtnSysSettings->setFixedSize(QSize(LEFT_TITLE_WIDTH,  LEFT_TITLE_MAX_BUTTON_HEIGHT));
    mBtnSysSettings->setCheckable(true);

    mBtnExitSys = new QPushButton();
    SET_OBJ_NAME(mBtnExitSys);
    mBtnExitSys->setFixedSize(QSize(LEFT_TITLE_WIDTH,  LEFT_TITLE_MIN_BUTTON_HEIGHT));

    QVBoxLayout *vLeftLayout = new QVBoxLayout();
    vLeftLayout->setSpacing(0);
    vLeftLayout->setContentsMargins(0, 0, 0, 0);
    vLeftLayout->setContentsMargins(0, 25, 0, 10);
    vLeftLayout->addLayout(hLayout1);
    vLeftLayout->addStretch(1);
    vLeftLayout->addLayout(hLayout2);
    vLeftLayout->addStretch(4);
    vLeftLayout->addWidget(mBtnNewCheck);
    vLeftLayout->addWidget(mBtnPatientRecord);
    vLeftLayout->addWidget(mBtnExp);
    vLeftLayout->addWidget(mBtnSysSettings);
    vLeftLayout->addStretch(4);
    vLeftLayout->addWidget(mBtnExitSys);
    mLeftWidget->setLayout(vLeftLayout);

    mStackedWidget = new QStackedWidget(this);
    SET_OBJ_NAME(mStackedWidget);
    mStackedWidget->setFixedSize(QSize(WIDGETWIDTH - LEFT_TITLE_WIDTH, WIDGETHEIGHT -LEFT_TITLE_MIN_BUTTON_HEIGHT));
    mStackedWidget->addWidget(mAboutWidget);
    mStackedWidget->addWidget(mNewPatientDialog);
    mStackedWidget->addWidget(mPatientRecordWidget);
    mStackedWidget->addWidget(mvSystemSettingsWidget);
    mStackedWidget->addWidget(mImageAcquisitWidget);
    mStackedWidget->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *vLayout = new QVBoxLayout();
    vLayout->setSpacing(0);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->addWidget(mTitleWidget);
    vLayout->addWidget(mStackedWidget);

    QHBoxLayout* mMainLayout = new QHBoxLayout();
    mMainLayout->setSpacing(0);
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->addWidget(mLeftWidget);
    mMainLayout->addLayout(vLayout);
    mCentralWidget->setLayout(mMainLayout);

    MainWidget = new QWidget();
    QHBoxLayout* hl = new QHBoxLayout();
    QVBoxLayout *vl = new QVBoxLayout();
    hl->setSpacing(0);
    hl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->addStretch(1);
    vl->addWidget(mCentralWidget);
    vl->addStretch(1);
    hl->addStretch(1);
    hl->addLayout(vl);
    hl->addStretch(1);
    MainWidget->setLayout(hl);
    this->setCentralWidget(MainWidget);

}

void MainWindow::initConnect()
{
    connect(mImageAcquisitWidget, SIGNAL(sigReturnMainPage(int)), this, SLOT(onImageAcquisitSwitchPage(int)));

    connect(mNewPatientDialog, SIGNAL(setNewPatient1(StudyRecord &, bool)), mPatientRecordWidget, SLOT(setNewPatientInfo(StudyRecord &, bool)));
    connect(mNewPatientDialog, SIGNAL(setNewPatientStartExam(StudyRecord &)), mPatientRecordWidget, SLOT(setNewPatientStartExam(StudyRecord &)));

    connect(mPatientRecordWidget, SIGNAL(sigPatientSwitchPage(int)), this, SLOT(onPatientSwitchPage(int)));

    connect(mPatientRecordWidget, SIGNAL(sigEditPatientInfo(QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString)), this, SLOT(slotEditPatientInfo(QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString, QString)));

    connect(mBtnExitSys, SIGNAL(clicked()), this, SLOT(close()));
    connect(mBtnSysSettings, SIGNAL(clicked(bool)), this, SLOT(OnSysSettingsClicked(bool)));
    connect(mBtnPatientRecord, SIGNAL(clicked(bool)), this, SLOT(OnPatientRecordClicked(bool)));
    connect(mBtnExp, SIGNAL(clicked(bool)), this, SLOT(OnImageAcquisitClicked(bool)));

    connect(mBtnNewCheck, SIGNAL(clicked(bool)), this, SLOT(OnNewCheckClicked(bool)));
    connect(mBtnExitLogin, SIGNAL(clicked()), this, SLOT(OnExitLoginClicked()));
    connect(mBtnLogin, SIGNAL(clicked()), this, SLOT(OnLoginClicked()));

}

void MainWindow::initQSS()
{
    QStringList list = QStringList()
        << ":/Res/QSS/specimen_custom.qss"
        << ":/Res/QSS/specimen_mainwidget.qss"
        << ":/Res/QSS/specimen_login.qss"
        << ":/Res/QSS/specimen_image.qss"
        << ":/Res/QSS/specimen_patient.qss"
        << ":/Res/QSS/specimen_report.qss"
        << ":/Res/QSS/specimen_setting.qss"
        << ":/Res/QSS/specimen_maintain.qss";

    QString styleSheet;
    foreach(auto name, list)
    {
        QFile file(name);
        if (file.open(QIODevice::ReadOnly))
        {
            styleSheet += QLatin1String(file.readAll());
        }
        file.close();
    }
    qApp->setStyleSheet(styleSheet);
}

void MainWindow::initDB()
{
    QString databaseDirectory = DataLocations::getRootConfigPath() + "/Database";
    QDir qdir(databaseDirectory);
    if (!qdir.exists(databaseDirectory))
    {
        if (!qdir.mkpath(databaseDirectory))
        {
            qDebug() << "Could not create database directory \"" << databaseDirectory;
        }
    }
    auto db = DicomDatabase::instance();

    QString databaseFileName = databaseDirectory + "/" + MV_DATABASE_NAME ;

    try
    {
        db->openDatabase(databaseFileName);
    }
    catch (std::exception& e)
    {
        qDebug() << "Database error: " << qPrintable(db->lastError());
        db->closeDatabase();
        return;
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    this->setAttribute(Qt::WA_Mapped);
    QWidget::showEvent(event);

    QTimer::singleShot(1000, this, SLOT(LoginWidget()));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (mStackedWidget->indexOf(mImageAcquisitWidget) != mStackedWidget->currentIndex())
    {
        mvExitAppDialog* exitAsk = new mvExitAppDialog(this);
        exitAsk->setObjectName("mvExitAppDialog");
        if (exitAsk->exec() == QDialog::Accepted)
        {
            mManager = XProtocolManager::getInstance();
            mManager->unInitSerialPort();

            qApp->exit(0);
        }
        else
        {
            if (exitAsk->flag == 0)
            {
                event->ignore();
                return;
            }
            else if (exitAsk->flag == 1)
            {
                onRestart();
            }
        }

    }
}

void MainWindow::onRestart()
{
    qApp->exit(666);
}

void MainWindow::turnPagebyIndex(int index)
{

    if (index == mStackedWidget->indexOf(mNewPatientDialog))
    {
        mNewPatientDialog->setModifyMode(false);

        mStackedWidget->setMinimumWidth(WIDGETWIDTH - LEFT_TITLE_WIDTH);
        mLeftWidget->setVisible(true);

        mBtnNewCheck->setChecked(true);
        mBtnPatientRecord->setChecked(false);
        mBtnSysSettings->setChecked(false);

    }
    else if (index == mStackedWidget->indexOf(mPatientRecordWidget))
    {
        mPatientRecordWidget->initData();
        mStackedWidget->setMinimumWidth(WIDGETWIDTH - LEFT_TITLE_WIDTH);
        mTitleWidget->setMinimumWidth(WIDGETWIDTH - LEFT_TITLE_WIDTH);
        mLeftWidget->setVisible(true);

        mBtnNewCheck->setChecked(false);
        mBtnPatientRecord->setChecked(true);
        mBtnSysSettings->setChecked(false);
        mBtnExp->setChecked(false);

    }
    else if (index == mStackedWidget->indexOf(mvSystemSettingsWidget))
    {
        mStackedWidget->setMinimumWidth(WIDGETWIDTH - LEFT_TITLE_WIDTH);
        mTitleWidget->setMinimumWidth(WIDGETWIDTH - LEFT_TITLE_WIDTH);
        mLeftWidget->setVisible(true);

        mBtnNewCheck->setChecked(false);
        mBtnPatientRecord->setChecked(false);
        mBtnSysSettings->setChecked(true);
        mBtnExp->setChecked(false);

    }
    else if (index == mStackedWidget->indexOf(mImageAcquisitWidget))
    {
        if(0)
        {
            mStackedWidget->setMaximumWidth(WIDGETWIDTH);
            mTitleWidget->setMaximumWidth(WIDGETWIDTH);
            mLeftWidget->setVisible(false);
        }
        else
        {
            mStackedWidget->setMinimumWidth(WIDGETWIDTH - LEFT_TITLE_WIDTH);
            mTitleWidget->setMinimumWidth(WIDGETWIDTH - LEFT_TITLE_WIDTH);
            mLeftWidget->setVisible(true);

            mBtnNewCheck->setChecked(false);
            mBtnPatientRecord->setChecked(false);
            mBtnSysSettings->setChecked(false);
            mBtnExp->setChecked(true);
        }

        this->mImageAcquisitWidget->setCurrentPatient();

    }

    mBtnNewCheck->setText(mNew);
    mStackedWidget->setCurrentIndex(index);

}

void MainWindow::LoginWidget()
{
    mvLoginWindow* lg = new mvLoginWindow();
    if (QDialog::Accepted == lg->exec())
    {
        mBtnExitLogin->setText("  " + SessionHelper::getInstance()->getCurrentUserDisplayName());
        mLoginStackWidget->setCurrentIndex(1);
#ifdef QT_NO_DEBUG // Release模式
        this->showFullScreen();
#endif
    }
    else
    {
        mLoginStackWidget->setCurrentIndex(0);
    }
}

void MainWindow::OnLoginClicked()
{
    LoginWidget();
}

void MainWindow::OnExitLoginClicked()
{
    mStackedWidget->setCurrentIndex(0);
    mLoginStackWidget->setCurrentIndex(0);
}

void MainWindow::OnNewCheckClicked(bool va)
{
    if (va)
    {
        mBtnNewCheck->setChecked(!mBtnNewCheck->isChecked());
        turnPagebyIndex(mStackedWidget->indexOf(mNewPatientDialog));
    }
    else
    {
        mBtnNewCheck->setChecked(!mBtnNewCheck->isChecked());
    }
}

void MainWindow::OnPatientRecordClicked(bool va)
{
    if (va)
    {
        mBtnPatientRecord->setChecked(!mBtnPatientRecord->isChecked());
        turnPagebyIndex(mStackedWidget->indexOf(mPatientRecordWidget));
    }
    else
    {
        mBtnPatientRecord->setChecked(!mBtnPatientRecord->isChecked());
    }
}

void MainWindow::OnImageAcquisitClicked(bool va)
{
    if (va)
    {
        mBtnExp->setChecked(!mBtnExp->isChecked());
        turnPagebyIndex(mStackedWidget->indexOf(mImageAcquisitWidget));
    }
    else
    {
        mBtnExp->setChecked(!mBtnExp->isChecked());
    }
}
void MainWindow::OnSysSettingsClicked(bool va)
{
    if (va)
    {
        mBtnSysSettings->setChecked(!mBtnSysSettings->isChecked());
        turnPagebyIndex(mStackedWidget->indexOf(mvSystemSettingsWidget));
    }
    else
    {
        mBtnSysSettings->setChecked(!mBtnSysSettings->isChecked());
    }
}

void MainWindow::onFinishAddNewPatient(bool va)
{
    if (va)
        turnPagebyIndex(mStackedWidget->indexOf(mPatientRecordWidget));

    mBtnNewCheck->setText(mNew);
    mNewPatientDialog->acceptDialog(va);
}

void MainWindow::slotEditPatientInfo(QString patientID, QString name, QString sex, QString bitthDate, QString age, QString accNumber, QString studyID, QString procID,QString procCode,QString procMeaning, QString reqPhysician, QString schPhysician, QString studyDate, QString studyTime, QString Modality, QString studyDesc, QString studyUID)
{
    OnNewCheckClicked(true);
    mNewPatientDialog->setPatientInfo(patientID, name, sex, bitthDate, age, accNumber, studyID, procID,procCode,procMeaning, reqPhysician, schPhysician, studyDate, studyTime, Modality, studyDesc, studyUID);
    mBtnNewCheck->setText(mEdit);
}

void MainWindow::onPatientSwitchPage(int index)
{
    turnPagebyIndex(mStackedWidget->indexOf(mImageAcquisitWidget));
}

void MainWindow::onImageAcquisitSwitchPage(int)
{
    turnPagebyIndex(mStackedWidget->indexOf(mPatientRecordWidget));
}



void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->isFullScreen() ? this->showFullScreen() : this->showMaximized();
    event->accept();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
//    Q_UNUSED(event);

//    mIsPressed = true;
//    mStartMovePos = event->globalPos();

//    event->accept();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
//    if (mIsPressed)
//    {
//        QPoint movePoint = event->globalPos() - mStartMovePos;
//        QPoint widgetPos = this->pos();
//        mStartMovePos = event->globalPos();

//        this->move(widgetPos.x() + movePoint.x(), widgetPos.y() + movePoint.y());
//    }

//    event->accept();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
//    mIsPressed = false;

//    event->accept();
}
