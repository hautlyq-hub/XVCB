#ifndef MVPATIENTRECORDWIDGET_H
#define MVPATIENTRECORDWIDGET_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QtCore/QFile>
#include <QtSql/QSqlQuery>
#include <QtWidgets/QListView>
#include <QtWidgets/QScroller>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QWidget>

#include "XV.Model/mvworklistitemmodel.h"
#include "XV.DataAccess/mvdicomdatabase.h"
#include "XV.Control/XDCustomWindow.h"
#include "XV.Model/mvstudyrecord.h"
#include "XV.Tool/mvSessionHelper.h"
#include "XV.Tool/mvXmlOptionItem.h"
#include "XV.Tool/mvdatalocations.h"


namespace Ui {
    class mvPatientRecordWidget;  // UI类的前向声明
}

namespace mv
{
    class mvPatientRecordWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit mvPatientRecordWidget(QWidget *parent = nullptr);
        ~mvPatientRecordWidget();

        void initView();
        void initConnect();

        void initData();

        void createPatientStudy(const StudyRecord &study);
        void updatePatientStudy(const StudyRecord &study);

        XmlOptionFile getXmlSettings();
        XmlOptionFile getLocal1Settings();
        void addLocalSettings();
        QModelIndexList deleteRepeatList(QModelIndexList indexList);


    protected:
        virtual void showEvent(QShowEvent *event);
        bool eventFilter(QObject* obj, QEvent* e);


    Q_SIGNALS:
        void finishedAddNewPatient(bool);
        void sigPatientSwitchPage(int index);
        void emitCurrentPatInfo(StudyRecord study);
        void emitClearStateMsg();
        void sigEditPatientInfo(QString patientID, QString name, QString sex, QString bitthDate, QString age, QString accNumber, QString studyID, QString procID,QString procCode,QString procMeaning,QString reqPhysician, QString schPhysician, QString studyDate, QString studyTime, QString Modality, QString studyDesc, QString studyUID);

    public slots:
        void onApplySearch();
        void onGetScuSearch(bool checked);
        void deletePatientClicked();
        void openUpdatePatientClicked();

        void onSwitchQueryPage();
        void onCloseQueryPage();
        void onResetTableWidget();

        void OnPidClicked();
        void OnAccessionClicked();
        void onCurrentItemChanged(QModelIndex index);
        bool setNewPatientInfo(StudyRecord &study, bool added);
        void setNewPatientStartExam(StudyRecord &study);
        void onPreviewBeginStudy();
        void onWlistBeginStudy();
        void onRemoteOrLocalClicked(bool);
        void onRemoteOrLocal2Clicked(bool);

        void onWlistDoubleClicked(const QModelIndex &index);

        void enableDateWidget_All(int state);
        void enableDateWidget_Today(int state);
        void enableDateWidget_Three(int state);

    private:
        Ui::mvPatientRecordWidget *ui;

        WorklistItemModel *wlistModel;
        QSortFilterProxyModel *wlistProxyModel;

        int mCurrentSelectRow = -1;
        int mIsExposeOrPreviewFlag = 0;

        bool mIsLocalData;

        QString mInsertPatientID;

    };
}

#endif // MVPATIENTRECORDWIDGET_H
