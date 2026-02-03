#ifndef MVNEWPATIENTDIALOG_H
#define MVNEWPATIENTDIALOG_H

#include <QtWidgets/QDialog>

#include "XV.Model/mvworklistitemmodel.h"
#include "XV.DataAccess/mvdicomdatabase.h"
#include "XV.Control/XDCustomWindow.h"
#include "XV.Model/mvstudyrecord.h"
#include "XV.Tool/mvSessionHelper.h"
#include "XV.Tool/mvXmlOptionItem.h"
#include "XV.Tool/mvdatalocations.h"

namespace Ui {
    class mvNewPatientDialog;
}

namespace mv
{

    class mvNewPatientDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit mvNewPatientDialog(QWidget *parent = 0);
        ~mvNewPatientDialog();

        void initView();
        void initConnect();

        void clearData();
        void setModifyMode(bool mode);
        void setProtocalState(QString  protocal);
        void setPatientInfo(QString patientID, QString name, QString sex, QString bitthDate, QString age, QString accNumber, QString studyID, QString procID,QString procCode,QString procMeaning,QString reqPhysician, QString schPhysician, QString studyDate, QString studyTime, QString schModality, QString studyDesc, QString studyUID);
        void autoFillData();

        XmlOptionFile getLocalSettings();

    protected:
        bool eventFilter(QObject *target, QEvent *event) override;
        void showEvent(QShowEvent *);

    signals:
        void setNewPatient1(StudyRecord &study, bool added);
        void setNewPatientStartExam(StudyRecord &study);
        void setNewPatient(QString name, QString id, uint sex, QString age, QString height, QString weight, QString date, QString remarks, bool added = true);
        void setEditPatient(QString name, QString id, uint sex, QString age, QString height, QString weight, QString date, QString remarks);
        void sigSwitchPatMagnPage(int index);

public slots:
        void acceptDialog(bool addResult);
        void confirmButtonClicked();
        void cancelButtonClicked();
        void startExamButtonClicked();

    private:


    private:
        Ui::mvNewPatientDialog *ui;

        bool modifyMode;
        StudyRecord study;
        QString mPatientID;
        QString mdirPath;
        QString mStudyUID;
        QString mProtocal;
    };
}

#endif // MVNEWPATIENTDIALOG_H
