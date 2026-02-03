#ifndef MVIMAGEACQUISITWIDGET_H
#define MVIMAGEACQUISITWIDGET_H

#include "mvnewpatientdialog.h"
#include "XV.DataAccess/mvdicomdatabase.h"
#include "XV.Control/XDCustomWindow.h"
#include "XV.Control/XDSliderControl.h"
#include "XV.Control/XDCableDiameterWidget.h"

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


	private:
        Ui::mvImageAcquisitWidget *ui;

        SliderStyle *s1;
        SliderStyle *s2;

        int mIsExposeOrPreviewFlag;
        int currentExamID;

        QVector<exam_type*> examList;

        QString currentPatientName;
        QString currentPatientId;
        QString currentPatientUID;
        QString currentStudyUID;
        QString currentPatientBirthDate;

        StudyRecord mCurrentStudyRecord;

	};
}

#endif // MVIMAGEACQUISITWIDGET_H
