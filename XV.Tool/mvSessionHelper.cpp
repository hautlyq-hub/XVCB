
#include "mvSessionHelper.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#include <QtCore/QDir>
#include <QtWidgets/QApplication>
#include <iostream>

#include <QtWidgets/QFileDialog>

namespace mv
{
    SessionHelper *SessionHelper::mInstance = NULL;

    SessionHelper* SessionHelper::getInstance()
    {
        if (mInstance == NULL)
        {
            mInstance = new SessionHelper();
        }
        return mInstance;
    }

    void SessionHelper::initialize()
    {
        SessionHelper::getInstance();
    }

    void SessionHelper::shutdown()
    {
        delete mInstance;
        mInstance = NULL;
    }


    void SessionHelper::selectCurrentPatient(const QString& name, const QString& id, const QString& studyUID, const QString& birthDate, const QString& relativeDir)
    {
        currentPatientName = name;
        currentPatientId = id;
        currentStudyUID = studyUID;
        currentPatientBirthDate = birthDate;
        currentRelativeDir = relativeDir;
        return;

        QString filename = QString("%1_%2")
            .arg(name)
            .arg(id);
        currentPatientName = name;
        currentPatientId = id;
    }

    void SessionHelper::selectCurrentPatientUID(QString uid)
    {
        currentPatientUID = uid;
    }

    void SessionHelper::setCurrentStudy(StudyRecord &mstudy)
    {

        this->study.accNumber = mstudy.accNumber;
        this->study.age = mstudy.age;
        this->study.modality = mstudy.modality;
        this->study.patientBirth = mstudy.patientBirth;
        this->study.patientId = mstudy.patientId;
        this->study.patientName = mstudy.patientName;
        this->study.patientSex = mstudy.patientSex;
        this->study.perPhysician = mstudy.perPhysician;
        this->study.procId = mstudy.procId;
        this->study.protocalCode = mstudy.protocalCode;
        this->study.protocalMeaning = mstudy.protocalMeaning;
        this->study.reqPhysician = mstudy.reqPhysician;
        this->study.studyDate = mstudy.studyDate;
        this->study.studyDesc = mstudy.studyDesc;
        this->study.studyId = mstudy.studyId;
        this->study.studyTime = mstudy.studyTime;
        this->study.studyUID = mstudy.studyUID;
        this->study.ExposeStatus = mstudy.ExposeStatus;

    }

    StudyRecord SessionHelper::getCurrentStudy()
    {
        return this->study;
    }

    QString SessionHelper::getCurrentPatientName() const
    {
        return currentPatientName;
    }

    QString SessionHelper::getCurrentPatientID() const
    {
        return currentPatientId;
    }

    QString SessionHelper::getCurrentStudyUID() const
    {
        return this->currentStudyUID;
    }
    void SessionHelper::setCurrentSeriesUID(QString seriesUID)
    {
         this->currentSeriesUID = seriesUID;
    }
    QString SessionHelper::getCurrentSeriesUID() const
    {
        return this->currentSeriesUID;
    }
    QString SessionHelper::getCurrentPatientBirthDate() const
    {
        return this->currentPatientBirthDate;
    }

    QString SessionHelper::getCurrentPatientUID() const
    {
        return  currentPatientUID;
    }

    QString SessionHelper::getCurrentPatientName_temporary() const
    {
        return currentPatientName_temporary;
    }

    QString SessionHelper::getCurrentPatientID_temporary() const
    {
        return  currentPatientId_temporary;
    }



    SessionHelper::SessionHelper()
    {
        currentPatientUID = "";
        currentPatientBirthDate = "";
        currentPatientId = "";
        currentPatientName = "";
        currentStudyUID = "";
        currentSeriesUID = "";

        clearUserInfo();
    }

    SessionHelper::~SessionHelper()
    {

    }


    void SessionHelper::setCurrentUserInfo(const QString& userId, const QString& userName,
                                            const QString& displayName, const QString& token,
                                            const QString& status, const QString& roleId,
                                            const QString& roleName, const QString& roleDescription)
       {
           // 保存用户信息
           currentUserID = userId;
           currentUserName = userName;
           currentUserDisplayName = displayName;
           currentUserToken = token;
           currentUserStatus = status;
           currentUserRoleID = roleId;
           currentUserRoleName = roleName;
           currentUserRoleDescription = roleDescription;

           // 设置登录时间
           loginTime = QDateTime::currentDateTime();
           lastActivityTime = loginTime;

           // 根据角色设置权限（示例）
           userPermissions.clear();
           if (roleName == "Administrator") {
               userPermissions << "view_all" << "edit_all" << "delete_all" << "system_config";
           } else if (roleName == "Doctor") {
               userPermissions << "view_patients" << "edit_records" << "view_reports";
           } else if (roleName == "Technician") {
               userPermissions << "view_patients" << "create_exams" << "edit_images";
           } else {
               userPermissions << "view_basic";
           }

           qDebug() << "User logged in:" << currentUserDisplayName
                    << "Role:" << currentUserRoleName
                    << "Login time:" << loginTime.toString("yyyy-MM-dd hh:mm:ss");

           // 发出登录信号
           emit userLoggedIn();
       }

       void SessionHelper::clearUserInfo()
       {
           // 清空用户信息
           currentUserID.clear();
           currentUserName.clear();
           currentUserDisplayName.clear();
           currentUserToken.clear();
           currentUserStatus.clear();
           currentUserRoleID.clear();
           currentUserRoleName.clear();
           currentUserRoleDescription.clear();

           // 清空会话信息
           loginTime = QDateTime();
           lastActivityTime = QDateTime();
           userPermissions.clear();

           // 发出登出信号
           emit userLoggedOut();
       }

       bool SessionHelper::hasPermission(const QString& permission) const
       {
           // 检查用户是否有指定权限
           if (isAdmin()) {
               return true;  // 管理员拥有所有权限
           }
           return userPermissions.contains(permission);
       }
}
