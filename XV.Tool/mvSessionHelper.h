#ifndef MV_SESSION_HELPER_H
#define MV_SESSION_HELPER_H

#include <QtCore/QString>
#include <QObject>
#include <QtCore/QDateTime>
#include "XV.Model/mvstudyrecord.h"

namespace mv
{
    class SessionHelper : public QObject
    {
        Q_OBJECT

    public:
        static SessionHelper* getInstance();
        static void initialize();
        static void shutdown();

        // 患者相关方法
        void selectCurrentPatient(const QString& name, const QString& id,
                                const QString& studyUID, const QString& birthDate,
                                const QString& relativeDir);
        void selectCurrentPatientUID(QString uid);
        void setCurrentStudy(StudyRecord &mstudy);
        StudyRecord getCurrentStudy();

        // 患者信息获取
        QString getCurrentPatientName() const;
        QString getCurrentPatientID() const;
        QString getCurrentStudyUID() const;
        void setCurrentSeriesUID(QString seriesUID);
        QString getCurrentSeriesUID() const;
        QString getCurrentPatientBirthDate() const;
        QString getCurrentRelativeDir() const { return currentRelativeDir; }
        QString getCurrentPatientUID() const;
        QString getCurrentPatientName_temporary() const;
        QString getCurrentPatientID_temporary() const;
        QString getCurrentRelativeDir_temporary() const { return currentRelativeDir_temporary; }

        // ========== 新增用户会话方法 ==========
        // 设置当前用户信息
        void setCurrentUserInfo(const QString& userId, const QString& userName,
                              const QString& displayName, const QString& token,
                              const QString& status, const QString& roleId,
                              const QString& roleName, const QString& roleDescription);

        // 获取当前用户信息
        QString getCurrentUserID() const { return currentUserID; }
        QString getCurrentUserName() const { return currentUserName; }
        QString getCurrentUserDisplayName() const { return currentUserDisplayName; }
        QString getCurrentUserToken() const { return currentUserToken; }
        QString getCurrentUserStatus() const { return currentUserStatus; }
        QString getCurrentUserRoleID() const { return currentUserRoleID; }
        QString getCurrentUserRoleName() const { return currentUserRoleName; }
        QString getCurrentUserRoleDescription() const { return currentUserRoleDescription; }

        // 用户登录状态
        bool isUserLoggedIn() const { return !currentUserID.isEmpty(); }
        void clearUserInfo();

        // 获取登录时间
        QDateTime getLoginTime() const { return loginTime; }

        // 检查权限
        bool hasPermission(const QString& permission) const;
        bool isAdmin() const { return currentUserRoleName == "Administrator"; }
        bool isDoctor() const { return currentUserRoleName == "Doctor"; }
        bool isTechnician() const { return currentUserRoleName == "Technician"; }

    signals:
        // 用户登录/登出信号
        void userLoggedIn();
        void userLoggedOut();
        void userInfoChanged();

    private:
        SessionHelper();
        ~SessionHelper();

        static SessionHelper* mInstance;

        // 项目信息
        QString mProjectName;

        // 当前患者信息
        QString currentPatientName;
        QString currentPatientId;
        QString currentPatientUID;
        QString currentStudyUID;
        QString currentSeriesUID;
        QString currentPatientBirthDate;
        QString currentRelativeDir;

        // 临时患者信息
        QString currentPatientName_temporary;
        QString currentPatientId_temporary;
        QString currentRelativeDir_temporary;

        // 当前检查信息
        StudyRecord study;

        // ========== 新增用户会话数据 ==========
        // 用户信息
        QString currentUserID;
        QString currentUserName;
        QString currentUserDisplayName;
        QString currentUserToken;
        QString currentUserStatus;
        QString currentUserRoleID;
        QString currentUserRoleName;
        QString currentUserRoleDescription;

        // 会话信息
        QDateTime loginTime;
        QDateTime lastActivityTime;

        // 权限列表（可以根据需要扩展）
        QStringList userPermissions;
    };

}
#endif // MV_SESSION_HELPER_H
