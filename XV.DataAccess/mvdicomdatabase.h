#ifndef __DicomDatabase_h
#define __DicomDatabase_h

#include <QMap>
#include <QObject>
#include <QStringList>
#include <QtCore/QDate>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

class DicomDatabase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isOpen READ isOpen)
    Q_PROPERTY(bool isInMemory READ isInMemory)
    Q_PROPERTY(QString lastError READ lastError)

public:
    static DicomDatabase* instance();
    explicit DicomDatabase(QObject *parent = nullptr);
    explicit DicomDatabase(const QString& databaseFile);
    virtual ~DicomDatabase();

    // 基本数据库操作
    QString lastError() const;
    bool isOpen() const;
    bool isInMemory() const;
    const QSqlDatabase& database() const;

    Q_INVOKABLE bool openDatabase(const QString& databaseFile, const QString& connectionName = "");
    Q_INVOKABLE void closeDatabase();

    // 通用数据库操作
    Q_INVOKABLE int executeQuery(const QString& sql);
    Q_INVOKABLE int executeQueries(const QStringList& sqlList);
    Q_INVOKABLE int insert(const QString& table, const QMap<QString, QString>& data);
    Q_INVOKABLE int remove(const QString& table, const QMap<QString, QString>& conditions);
    Q_INVOKABLE int update(const QString& table,
                          const QMap<QString, QString>& conditions,
                          const QMap<QString, QString>& data);
    Q_INVOKABLE QList<QStringList> select(const QString& table,
                                         const QList<QString>& columns = {},
                                         const QMap<QString, QString>& conditions = {});

    // 病人相关操作
    Q_INVOKABLE QStringList getPatients(bool ascending = true);
    Q_INVOKABLE QString getPatientByUID(const QString& patientUID);
    Q_INVOKABLE QString getPatientByStudyUID(const QString& studyUID);
    Q_INVOKABLE bool patientExists(const QString& name, const QString& id = "");
    Q_INVOKABLE int insertPatient(const QString name, const QString id, const QString sex, const QString date, const QString age, const QString recordDate);
    Q_INVOKABLE bool updatePatient(const QString name, const QString id, const QString sex, const QString date, const QString age, const int uid);

    // 检查相关操作
    Q_INVOKABLE QStringList getStudiesForPatient(const QString& patientUID);
    Q_INVOKABLE QString getStudyUIDBySeriesUID(const QString& seriesUID);
    Q_INVOKABLE QString getModalityForStudy(const QString& studyUID);
    Q_INVOKABLE bool isExistStudyId(const QString id, bool isCaseSensitive);
    Q_INVOKABLE int insertStudy(int patientUID, const QString studyInstanceUID, const QString studyID, const QString age, const QString studyDate, const QString studyTime, const QString accessionNumber, const QString modality, const QString referringPhysician, const QString performingPhysician, const QString studyDescription, const QString proID, const QString protocal, const QString protocalDes);
    Q_INVOKABLE void updateStudy(int patientUID, const QString studyID, const QString age, const QString studyDate, const QString studyTime, const QString accessionNumber, const QString modality,const QString referringPhysician, const QString performingPhysician, const QString studyDescription,const QString proID, const QString protocal, const QString protocalDes, const QString studyUID);
    Q_INVOKABLE QString getStudyLockedFromStudyUID(QString StudyUID);
    Q_INVOKABLE bool updateIsDeleteFromStudyUID(QString StudyUID, QString yes);

    // 序列相关操作
    Q_INVOKABLE QStringList getSeriesForStudy(const QString& studyUID);
    Q_INVOKABLE QStringList getInstancesForSeries(const QString& seriesUID);
    Q_INVOKABLE int insertSeries(const QString studyUID, const QString SeriesInstanceUID, const QString seriesID, const QString displayName, const QString modality, const QString seriesNumber, const QString Laterality, const QString ProtocolName, const QString BodyPartExamined, const QString ExposeStatus);
    Q_INVOKABLE int insertSeries(const QString studyUID, const QString SeriesInstanceUID, const QString modality, const QString seriesNumber, const QString BodyPartExamined, const QString ExposeStatus);
    Q_INVOKABLE bool deleteForImagebySopUID(QString SopUID);

    // 图像文件操作
    Q_INVOKABLE QString getFileForInstance(const QString& sopInstanceUID);
    Q_INVOKABLE QStringList getFilesForSeries(const QString& seriesUID);
    Q_INVOKABLE int insertImage(const QString SeriesInstanceUID, const QString SOPInstanceUID, const QString PatientOrientation, const QString ImageLaterality, const QString ImageFileFullName, const QString ViewName, const QString ViewDescription, const QString ViewPosition, const QString InstanceNumber);
    Q_INVOKABLE QStringList getSopUIDByStudyUID(QString StudyUID);
    Q_INVOKABLE QStringList getImageStatusBySOPUID(const QString SOPInstanceUID);
    Q_INVOKABLE QString getImageFileFullNameBySOPUID(const QString SOPUID);
    Q_INVOKABLE bool updateImagesInstanceNumberBySOPUID(QString SOPInstanceUID,  QString InstanceNumber);
    Q_INVOKABLE QStringList getSOPUIDAndSeriesUIDAndViewNameForStudy(const QString studyUID);
    Q_INVOKABLE QStringList viewDescriptionAndFileForInstance(QString sopInstanceUID);

    // 删除操作
    Q_INVOKABLE bool deleteSeries(const QString& seriesUID);
    Q_INVOKABLE bool deleteImage(const QString& sopInstanceUID);

    Q_INVOKABLE bool isExistsUseNameFormUserName(QString UserName);
    Q_INVOKABLE QString getPasswordFormUserName(QString UserName);
    Q_INVOKABLE QString getUserDisplayNameFormUserName(QString UserName);
    Q_INVOKABLE QStringList getUserLoginInfoFormUserName(const QString UserName);
    Q_INVOKABLE bool insertUserLogin(QString LoginID, QString UserID, QString LoginDateTime, QString LoginState);

private:
    static DicomDatabase* s_instance;
    QSqlDatabase m_database;
    QString m_lastError;

    // 私有辅助方法
    QString buildWhereClause(const QMap<QString, QString>& conditions);
    QString escapeString(const QString& str);
    bool checkDatabaseOpen();
};

#endif
