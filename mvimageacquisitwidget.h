#ifndef MVIMAGEACQUISITWIDGET_H
#define MVIMAGEACQUISITWIDGET_H

#include "mvnewpatientdialog.h"
#include "XV.DataAccess/mvdicomdatabase.h"
#include "XV.Control/XDCustomWindow.h"
#include "XV.Control/XDSliderControl.h"
#include "XV.Control/XDCableDiameterWidget.h"

#include "XV.Communication/XProtocolManager.h"

#include <QDropEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QPainter>
#include <QQueue>
#include <QRandomGenerator>
#include <QTextDocument>
#include <QtCore/QDate>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTime>
#include <QtPrintSupport/qprinter.h>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLayout>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

#include <QtWidgets/QCalendarWidget>
#include <QtWidgets/QProxyStyle>
#include <QtWidgets/QStyleOption>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Ui {
    class mvImageAcquisitWidget;
}

namespace mv
{
    struct exam_type {
        int id;
        QString SeriesInstanceUID;
        QString SeriesID;
        QString SOPUID;
        QString ImageFileFullName;
        bool isExam;
        bool isRemote;
    };

	class mvImageAcquisitWidget : public QWidget
	{
		Q_OBJECT
	public:
		explicit mvImageAcquisitWidget(QWidget* parent = 0);
		~mvImageAcquisitWidget();
		
        void initView();
        void initConnect();
        void initializePreviewFrame();
        void initComm();

        void setCurrentPatient();
        void setCurrentPage(int);
        void getImagePath(QVector<exam_type*>& examList, QString studyUID);
        void autoSwitchNextPosition();

        void updateDeviceState(ExposureState state);
        void updateInfoPanel(const QString& message, int type);

        void StartExposure();
        void StopExposure();
        bool canStartExposure() const;
        void resetUI();

        void AlgSetCorrectionFile(int mOralMajor, QString mOralAddr);

    signals:
        void sigReturnMainPage(int index = 0);

    public slots:
        void slotWWVSliderValueChanged(int);
        void slotWLVSliderValueChanged(int);

        void onWWAddClicked();
        void onWWSubClicked();
        void onWLAddClicked();
        void onWLSubClicked();
        void onAddViewButton();
        void onDeleteViewButton();
        void onFinishedButton();
        void onStartButton();

        void onInfoReceived(const QString& message);
        void onWarningReceived(const QString& message);
        void onErrorReceived(const QString& message);
        void onNotificationReceived(const QString& message);
        void onExposureError(const QString& error);
        void onImagesReady(const QVector<HWImageData>& images);
        void onExposureProcess(ExposureState state);

    private:
        Ui::mvImageAcquisitWidget *ui;

        enum XPInfoType { Normal = 0, Warning = 1, Error = 2 };

        SliderStyle *s1;
        SliderStyle *s2;
        XProtocolManager* mManager;

        int mIsExposeOrPreviewFlag;
        int currentExamID;

        QVector<exam_type*> examList;

        QString currentPatientName;
        QString currentPatientId;
        QString currentPatientUID;
        QString currentStudyUID;
        QString currentPatientBirthDate;

        StudyRecord mCurrentStudyRecord;

        bool mIsExposing = false;
        QString mOralAddr;
        int mOralMajor = 3;

        bool mcalibrationFileExists = false;
    };
}

#endif // MVIMAGEACQUISITWIDGET_H
