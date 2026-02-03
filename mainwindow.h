#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QPointer>
#include <QPointer>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTextBrowser>
#include <QProcess>
#include <QtCore/QTimer>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QPushButton>
#include <QResource>
#include <QCloseEvent>
#include <QScreen>

#include "mvstatusbar.h"
#include "mvimageacquisitwidget.h"
#include "mvnewpatientdialog.h"
#include "mvpatientrecordwidget.h"
#include "mvsystemsettings.h"
#include "mvaboutwidget.h"
#include "mvloginwindow.h"
#include "mvexitappdialog.h"

#include "XV.Tool/mvconfig.h"
#include "XV.Tool/mvdatalocations.h"
#include "XV.DataAccess/mvdicomdatabase.h"
#include "XV.Control/XDCustomWindow.h"

#define GB (1024 * 1024 * 1024)
#define MB (1024 * 1024)
#define KB (1024)

#define SET_OBJ_NAME(obj) (obj)->setObjectName(#obj)



using namespace mv;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initView();
    void initConnect();
    void initQSS();
    void initDB();
    void turnPagebyIndex(int index);
    void onRestart();

protected:
    virtual void showEvent(QShowEvent *event);
    virtual void closeEvent(QCloseEvent *event);

    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

protected slots:
    void LoginWidget();
    void OnLoginClicked();
    void OnExitLoginClicked();

    void OnNewCheckClicked(bool va);
    void OnPatientRecordClicked(bool va);
    void OnImageAcquisitClicked(bool va);
    void OnSysSettingsClicked(bool va);

    void onFinishAddNewPatient(bool va);
    void slotEditPatientInfo(QString patientID, QString name, QString sex, QString bitthDate, QString age, QString accNumber, QString studyID, QString procID,QString procCode,QString procMeaning, QString reqPhysician, QString schPhysician, QString studyDate, QString studyTime, QString Modality, QString studyDesc, QString studyUID);
    void onPatientSwitchPage(int index);
    void onImageAcquisitSwitchPage(int);

private:

    StatusBar *mStatusBar;
    QWidget* mCentralWidget;
    QWidget* MainWidget;
    QWidget* mTitleWidget;
    QStackedWidget* mStackedWidget;
    QStackedWidget* mLoginStackWidget;
    QPushButton* mBtnExitLogin;
    QPushButton* mBtnLogin;
    QWidget*mLeftWidget;
    QPushButton* mBtnNewCheck;
    QPushButton* mBtnPatientRecord;
    QPushButton* mBtnExp;
    QPushButton* mBtnSysSettings;
    QPushButton* mBtnExitSys;

    mvNewPatientDialog* mNewPatientDialog;
    mvPatientRecordWidget* mPatientRecordWidget;
    mvImageAcquisitWidget* mImageAcquisitWidget;
    mvsystemsettings* mvSystemSettingsWidget;
    mvAboutWidget* mAboutWidget;

    QString mNew;
    QString mEdit;
    QString mList;
    QString mMain;
    QString mSet;


    bool mIsPressed;

    QPoint mStartMovePos;

};

#endif // MAINWINDOW_H
