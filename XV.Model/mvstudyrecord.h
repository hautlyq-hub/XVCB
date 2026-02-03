#ifndef STUDYRECORD_H
#define STUDYRECORD_H

#include <QtCore/QString>
#include <QtCore/QDate>
#include <QtCore/QTime>

struct StudyRecord
{
    // 数据成员（与数据库字段对应）
    QString patientName;
    QString patientId;
    QString patientSex;
    QString patientBirth;      // "yyyy-MM-dd"格式
    QString age;
    QString accNumber;
    QString studyId;
    QString procId;
    QString protocalCode;
    QString protocalMeaning;
    QString studyDate;         // "yyyy-MM-dd"格式
    QString studyTime;         // "hh:mm:ss"格式
    QString modality;
    QString reqPhysician;
    QString perPhysician;
    QString studyDesc;
    QString studyUID;
    QString ExposeStatus;
    QString ID;
    
    // 默认构造函数
    StudyRecord() = default;
    
    // 带UID的构造函数
    StudyRecord(const QString &uid) : studyUID(uid) {}
    
    // 方便使用的构造函数
    StudyRecord(const QString &name, 
               const QString &id, 
               const QString &uid)
        : patientName(name)
        , patientId(id)
        , studyUID(uid) {}
};
#endif // STUDYRECORD_H