#include "mvdicomdatabase.h"
#include <QMutex>
#include <QtCore/QDate>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

DicomDatabase *DicomDatabase::s_instance = nullptr;

DicomDatabase* DicomDatabase::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    static DicomDatabase* instance = nullptr;
    if (!instance) {
        instance = new DicomDatabase();
    }
    return instance;
}


DicomDatabase::DicomDatabase(QObject *parent)
    : QObject(parent)
    , m_database(QSqlDatabase::addDatabase("QSQLITE"))
    , m_lastError("")
{
    Q_ASSERT(DicomDatabase::s_instance == nullptr);
    DicomDatabase::s_instance = this;
}

DicomDatabase::DicomDatabase(const QString& databaseFile)
    : QObject(nullptr)
    , m_database(QSqlDatabase::addDatabase("QSQLITE"))
    , m_lastError("")
{
    Q_ASSERT(DicomDatabase::s_instance == nullptr);
    DicomDatabase::s_instance = this;
    openDatabase(databaseFile);
}

DicomDatabase::~DicomDatabase()
{
    if (DicomDatabase::s_instance == this) {
        DicomDatabase::s_instance = nullptr;
    }
    closeDatabase();
}

QString DicomDatabase::lastError() const
{
    return m_lastError;
}

bool DicomDatabase::isOpen() const
{
    return m_database.isOpen();
}

bool DicomDatabase::isInMemory() const
{
    return m_database.databaseName() == ":memory:";
}

const QSqlDatabase& DicomDatabase::database() const
{
    return m_database;
}


bool DicomDatabase::openDatabase(const QString& databaseFile, const QString& connectionName)
{
    if (m_database.isOpen()) {
        closeDatabase();
    }

    QString verifiedConnectionName = connectionName.isEmpty()
        ? QString("DICOM_DB_%1").arg(reinterpret_cast<quintptr>(this))
        : connectionName;

    m_database = QSqlDatabase::addDatabase("QSQLITE", verifiedConnectionName);
    m_database.setDatabaseName(databaseFile);

    if (!m_database.open()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    return true;
}

void DicomDatabase::closeDatabase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool DicomDatabase::checkDatabaseOpen()
{
    if (!m_database.isOpen()) {
        m_lastError = "Database is not open";
        return false;
    }
    return true;
}

QString DicomDatabase::escapeString(const QString& str)
{
    // 简单的SQL转义
    QString escaped = str;
    escaped.replace("'", "''");
    return escaped;
}

QString DicomDatabase::buildWhereClause(const QMap<QString, QString>& conditions)
{
    if (conditions.isEmpty()) {
        return "";
    }

    QStringList whereParts;
    for (auto it = conditions.constBegin(); it != conditions.constEnd(); ++it) {
        whereParts.append(QString("%1 = '%2'").arg(it.key()).arg(escapeString(it.value())));
    }

    return "WHERE " + whereParts.join(" AND ");
}

int DicomDatabase::executeQuery(const QString& sql)
{
    if (!checkDatabaseOpen()) return 0;

    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return 0;
    }
    return 1;
}

int DicomDatabase::executeQueries(const QStringList& sqlList)
{
    if (!checkDatabaseOpen()) return 0;

    m_database.transaction();
    QSqlQuery query(m_database);

    for (const QString& sql : sqlList) {
        if (!query.exec(sql)) {
            m_database.rollback();
            m_lastError = query.lastError().text();
            return 0;
        }
    }

    m_database.commit();
    return 1;
}

int DicomDatabase::insert(const QString& table, const QMap<QString, QString>& data)
{
    if (!checkDatabaseOpen()) return 0;

    if (data.isEmpty()) {
        m_lastError = "No data to insert";
        return 0;
    }

    QStringList columns, values;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        columns.append(it.key());
        values.append(QString("'%1'").arg(escapeString(it.value())));
    }

    QString sql = QString("INSERT INTO %1 (%2) VALUES (%3)")
                     .arg(table)
                     .arg(columns.join(", "))
                     .arg(values.join(", "));

    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return 0;
    }

    return query.numRowsAffected();
}

int DicomDatabase::remove(const QString& table, const QMap<QString, QString>& conditions)
{
    if (!checkDatabaseOpen()) return 0;

    QString whereClause = buildWhereClause(conditions);
    QString sql = QString("DELETE FROM %1 %2").arg(table).arg(whereClause);

    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return 0;
    }

    return query.numRowsAffected();
}

int DicomDatabase::update(const QString& table, const QMap<QString, QString>& conditions, const QMap<QString, QString>& data)
{
    if (!checkDatabaseOpen()) return 0;

    if (data.isEmpty()) {
        m_lastError = "No data to update";
        return 0;
    }

    QStringList setParts;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        setParts.append(QString("%1 = '%2'").arg(it.key()).arg(escapeString(it.value())));
    }

    QString whereClause = buildWhereClause(conditions);
    QString sql = QString("UPDATE %1 SET %2 %3")
                     .arg(table)
                     .arg(setParts.join(", "))
                     .arg(whereClause);

    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return 0;
    }

    return query.numRowsAffected();
}

QList<QStringList> DicomDatabase::select(const QString& table, const QList<QString>& columns, const QMap<QString, QString>& conditions)
{
    QList<QStringList> result;

    if (!checkDatabaseOpen()) return result;

    QString columnList = columns.isEmpty() ? "*" : columns.join(", ");
    QString whereClause = buildWhereClause(conditions);

    QString sql = QString("SELECT %1 FROM %2 %3")
                     .arg(columnList)
                     .arg(table)
                     .arg(whereClause);

    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return result;
    }

    int columnCount = columns.isEmpty() ? query.record().count() : columns.size();

    while (query.next()) {
        QStringList row;
        for (int i = 0; i < columnCount; ++i) {
            row.append(query.value(i).toString());
        }
        result.append(row);
    }

    return result;
}

QStringList DicomDatabase::getPatients(bool ascending)
{
    QStringList result;
    if (!checkDatabaseOpen()) return result;

    QString order = ascending ? "ASC" : "DESC";
    QString sql = QString("SELECT UID FROM Patients ORDER BY PatientsRegisterDate %1").arg(order);

    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(query.value(0).toString());
    }

    return result;
}

QString DicomDatabase::getPatientByUID(const QString& patientUID)
{
    if (!checkDatabaseOpen()) return "";

    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM Patients WHERE UID = ?");
    query.bindValue(0, patientUID);

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return "";
    }

    // 返回患者信息，可以根据需要格式化
    return QString("Name: %1, ID: %2")
           .arg(query.value("PatientsName").toString())
           .arg(query.value("PatientID").toString());
}

QString DicomDatabase::getPatientByStudyUID(const QString& studyUID)
{
    if (!checkDatabaseOpen()) return "";

    // 首先获取PatientUID
    QSqlQuery query(m_database);
    query.prepare("SELECT PatientUID FROM Studies WHERE StudyInstanceUID = ?");
    query.bindValue(0, studyUID);

    if (!query.exec() || !query.next())
    {
        m_lastError = query.lastError().text();
        return "";
    }

    QString patientUID = query.value(0).toString();

    return patientUID;
}

bool DicomDatabase::patientExists(const QString& name, const QString& id)
{
    if (!checkDatabaseOpen()) return false;

    QSqlQuery query(m_database);
    if (!id.isEmpty()) {
        query.prepare("SELECT UID FROM Patients WHERE PatientsName = ? AND PatientID = ?");
        query.bindValue(0, name);
        query.bindValue(1, id);
    } else {
        query.prepare("SELECT UID FROM Patients WHERE PatientsName = ?");
        query.bindValue(0, name);
    }

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return query.next();
}

int DicomDatabase::insertPatient(const QString name, const QString id, const QString sex,
                                 const QString date, const QString age, const QString recordDate)
{
    QSqlQuery insertPatientStatement(m_database);
    insertPatientStatement.prepare(
        "INSERT INTO Patients ('UID', 'PatientName', 'PatientID', 'PatientBirthDate', "
        "'PatientBirthTime', 'PatientSex', 'PatientAge', 'PatientsRegisterDate') "
        "VALUES (NULL, ?, ?, ?, ?, ?, ?, ?)"
    );

    insertPatientStatement.addBindValue(name);
    insertPatientStatement.addBindValue(id);
    insertPatientStatement.addBindValue(date);
    insertPatientStatement.addBindValue("");  // PatientBirthTime
    insertPatientStatement.addBindValue(sex);
    insertPatientStatement.addBindValue(age);
    insertPatientStatement.addBindValue(recordDate);

    if (!insertPatientStatement.exec()) {
        qDebug() << "Failed to insert patient:" << insertPatientStatement.lastError().text();
        return -1;
    }

    int dbPatientID = insertPatientStatement.lastInsertId().toInt();
    return dbPatientID;
}

bool DicomDatabase::updatePatient(const QString name, const QString id, const QString sex, const QString date, const QString age, const int uid)
{
    QSqlQuery query(m_database);

        query.prepare("UPDATE Patients SET "
                      "PatientName = :name, "
                      "PatientID = :id, "
                      "PatientSex = :sex, "
                      "PatientBirthDate = :birthdate, "
                      "PatientAge = :age "
                      "WHERE UID = :uid");

        // 使用命名参数更清晰
        query.bindValue(":name", name);
        query.bindValue(":id", id);
        query.bindValue(":sex", sex);
        query.bindValue(":birthdate", date);
        query.bindValue(":age", age);
        query.bindValue(":uid", uid);

        if (!query.exec()) {
            qWarning() << "Failed to update patient:" << query.lastError().text();
            return false;
        }

        return true;
}

int DicomDatabase::insertStudy(int patientUID, const QString studyInstanceUID, const QString studyID, const QString age, const QString studyDate, const QString studyTime, const QString accessionNumber, const QString modality,
    const QString referringPhysician, const QString performingPhysician, const QString studyDescription, const QString proID,const QString protocal, const QString protocalDes)
{
    QString mStudyID = studyID;
    QSqlQuery checkStudyExistsQuery(m_database);
    checkStudyExistsQuery.prepare("SELECT * FROM Studies WHERE StudyInstanceUID = ?");
    checkStudyExistsQuery.bindValue(0, studyInstanceUID);
    checkStudyExistsQuery.exec();
    if (!checkStudyExistsQuery.next())
    {
        QString admitDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QString admitTime = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString pregnancyStatus = "0001";
        QString regSource = "L";

        QSqlQuery insertStudyStatement(m_database);
        insertStudyStatement.prepare("INSERT INTO Studies ( 'StudyInstanceUID', 'PatientUID', 'StudyID', 'ScheduledStartDate', 'ScheduledStartTime', 'AccessionNumber', 'ModalitiesInStudy', 'PregnancyStatus', 'AdmittingDate', 'AdmittingTime', "
            "'RegSource', 'Priority', 'PerformedProtocolCodeValue', 'PerformedProtocolCodeMeaning', 'ReferringPhysician', 'PerformingPhysiciansName', 'StudyDescription' ) VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        insertStudyStatement.bindValue(0, studyInstanceUID);
        insertStudyStatement.bindValue(1, patientUID);
        insertStudyStatement.bindValue(2, studyID);
        insertStudyStatement.bindValue(3, studyDate);
        insertStudyStatement.bindValue(4, studyTime);
        insertStudyStatement.bindValue(5, accessionNumber);
        insertStudyStatement.bindValue(6, modality);
        insertStudyStatement.bindValue(7, pregnancyStatus);
        insertStudyStatement.bindValue(8, admitDate);
        insertStudyStatement.bindValue(9, admitTime);
        insertStudyStatement.bindValue(10, regSource);
        insertStudyStatement.bindValue(11, proID);
        insertStudyStatement.bindValue(12, protocal);
        insertStudyStatement.bindValue(13, protocalDes);
        insertStudyStatement.bindValue(14, referringPhysician);
        insertStudyStatement.bindValue(15, performingPhysician);
        insertStudyStatement.bindValue(16, studyDescription);
        if (!insertStudyStatement.exec())
        {
            qCritical() << ("Error executing statament: " + insertStudyStatement.lastQuery() + " Error: " + insertStudyStatement.lastError().text());
        }
        return patientUID;
    }
    else
    {
        int result = -1;

        QSqlQuery query(m_database);
        query.prepare("SELECT PatientUID FROM Studies WHERE StudyInstanceUID= ?");
        query.bindValue(0, studyInstanceUID);
        if (query.exec())
        {
            if (query.next())
            {
                result = query.value(0).toInt();
            }

            return result;
        }
    }

    return -1;
}
void DicomDatabase::updateStudy(int patientUID, const QString studyID, const QString age, const QString studyDate, const QString studyTime, const QString accessionNumber, const QString modality,const QString referringPhysician, const QString performingPhysician, const QString studyDescription,const QString proID,const QString protocal, const QString protocalDes, const QString studyUID)
{
    QSqlQuery insertPatientStatement(m_database);
    insertPatientStatement.prepare("UPDATE Studies SET StudyID = ?, AccessionNumber = ?, Priority = ?, PerformedProtocolCodeValue = ?, PerformedProtocolCodeMeaning = ?, ScheduledStartDate = ?,  ScheduledStartTime = ?, ReferringPhysician = ?, PerformingPhysiciansName = ?, StudyDescription = ?, ModalitiesInStudy = ? WHERE PatientUID = ? and StudyInstanceUID = ?");
    insertPatientStatement.bindValue(0, studyID);
    insertPatientStatement.bindValue(1, accessionNumber);
    insertPatientStatement.bindValue(2, proID);
    insertPatientStatement.bindValue(3, protocal);
    insertPatientStatement.bindValue(4, protocalDes);
    insertPatientStatement.bindValue(5, studyDate);
    insertPatientStatement.bindValue(6, studyTime);
    insertPatientStatement.bindValue(7, referringPhysician);
    insertPatientStatement.bindValue(8, performingPhysician);
    insertPatientStatement.bindValue(9, studyDescription);
    insertPatientStatement.bindValue(10, modality);
    insertPatientStatement.bindValue(11, patientUID);
    insertPatientStatement.bindValue(12, studyUID);
    insertPatientStatement.exec();
}

int DicomDatabase::insertSeries(const QString studyUID, const QString SeriesInstanceUID, const QString seriesID, const QString displayName, const QString modality, const QString seriesNumber, const QString Laterality, const QString ProtocolName,
    const QString BodyPartExamined, const QString ExposeStatus)
{
    QString mStudyUID = studyUID;
    QSqlQuery checkStudyExistsQuery(m_database);
    checkStudyExistsQuery.prepare("SELECT * FROM Series WHERE SeriesInstanceUID = ?");
    checkStudyExistsQuery.bindValue(0, SeriesInstanceUID);
    checkStudyExistsQuery.exec();
    if (!checkStudyExistsQuery.next())
    {
        //qDebug() << "Need to insert new study: " << SeriesInstanceUID;

        QString admitDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QString admitTime = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString pregnancyStatus = "0001";
        QString regSource = "L";

        QSqlQuery insertSeriesStatement(m_database);
        insertSeriesStatement.prepare("INSERT INTO Series ( 'StudyInstanceUID', 'SeriesInstanceUID', 'SeriesID', 'DisplayName', 'Modality', 'SeriesNumber', 'Laterality', 'ProtocolName', 'BodyPartExamined', 'ExposeStatus') VALUES ( ?, ?, ?, ?, ?, ?, ?, ?, ?, ? )");
        insertSeriesStatement.bindValue(0, studyUID);
        insertSeriesStatement.bindValue(1, SeriesInstanceUID);
        insertSeriesStatement.bindValue(2, seriesID);
        insertSeriesStatement.bindValue(3, displayName);
        insertSeriesStatement.bindValue(4, modality);
        insertSeriesStatement.bindValue(5, seriesNumber);
        insertSeriesStatement.bindValue(6, Laterality);
        insertSeriesStatement.bindValue(7, ProtocolName);
        insertSeriesStatement.bindValue(8, BodyPartExamined);
        insertSeriesStatement.bindValue(9, ExposeStatus);
        if (!insertSeriesStatement.exec())
        {
            qCritical() << ("Error executing statament: " + insertSeriesStatement.lastQuery() + " Error: " + insertSeriesStatement.lastError().text());
        }
        else
        {

        }
        return 1;
    }
    else
    {
        //qDebug() << "Used existing series: " << SeriesInstanceUID;
        int result = -1;

        QSqlQuery query(m_database);
        query.prepare("SELECT PatientUID FROM Studies WHERE StudyID= (SELECT StudyID FROM Series WHERE  SeriesInstanceUID= ?)");
        query.bindValue(0, SeriesInstanceUID);
        if (query.exec())
        {
            if (query.next())
            {
                result = query.value(0).toInt();
            }
            return result;
        }
    }
    return -1;
}

int DicomDatabase::insertSeries(const QString studyUID, const QString SeriesInstanceUID, const QString modality, const QString seriesNumber, const QString BodyPartExamined, const QString ExposeStatus)
{
    QString mStudyUID = studyUID;
    QSqlQuery checkStudyExistsQuery(m_database);
    checkStudyExistsQuery.prepare("SELECT * FROM Series WHERE SeriesInstanceUID = ?");
    checkStudyExistsQuery.bindValue(0, SeriesInstanceUID);
    checkStudyExistsQuery.exec();
    if (!checkStudyExistsQuery.next())
    {
        QString admitDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QString admitTime = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString pregnancyStatus = "0001";
        QString regSource = "L";

        QSqlQuery insertSeriesStatement(m_database);
        insertSeriesStatement.prepare("INSERT INTO Series ( 'StudyInstanceUID', 'SeriesInstanceUID', 'SeriesDate', 'SeriesTime', 'Modality', 'SeriesNumber', 'BodyPartExamined', 'ExposeStatus') VALUES ( ?, ?, ?, ?, ?, ?, ?, ?)");
        insertSeriesStatement.bindValue(0, studyUID);
        insertSeriesStatement.bindValue(1, SeriesInstanceUID);
        insertSeriesStatement.bindValue(2, admitDate);
        insertSeriesStatement.bindValue(3, admitTime);
        insertSeriesStatement.bindValue(4, modality);
        insertSeriesStatement.bindValue(5, seriesNumber);
        insertSeriesStatement.bindValue(6, BodyPartExamined);
        insertSeriesStatement.bindValue(7, ExposeStatus);
        if (!insertSeriesStatement.exec())
        {
            qCritical() << ("Error executing statament: " + insertSeriesStatement.lastQuery() + " Error: " + insertSeriesStatement.lastError().text());
        }
        else
        {

        }
        return 1;
    }
    else
    {
        int result = -1;

        QSqlQuery query(m_database);
        query.prepare("SELECT PatientUID FROM Studies WHERE StudyID= (SELECT StudyID FROM Series WHERE  SeriesInstanceUID= ?)");
        query.bindValue(0, SeriesInstanceUID);
        if (query.exec())
        {
            if (query.next())
            {
                result = query.value(0).toInt();
            }
            return result;
        }
    }
    return -1;
}

int DicomDatabase::insertImage(const QString SeriesInstanceUID, const QString SOPInstanceUID, const QString PatientOrientation, const QString ImageLaterality, const QString ImageFileFullName, const QString ViewName, const QString ViewDescription, const QString ViewPosition, const QString InstanceNumber)
{
    QSqlQuery checkStudyExistsQuery(m_database);
    checkStudyExistsQuery.prepare("SELECT * FROM Images WHERE SOPInstanceUID = ?");
    checkStudyExistsQuery.bindValue(0, SOPInstanceUID);
    checkStudyExistsQuery.exec();
    if (!checkStudyExistsQuery.next())
    {

        QSqlQuery insertImagesStatement(m_database);
        insertImagesStatement.prepare("INSERT INTO Images ( 'SeriesInstanceUID', 'SOPInstanceUID', 'PatientOrientation', 'ImageLaterality', 'ImageFileFullName','ViewName','ViewDescription','ViewPosition','InstanceNumber' ) VALUES ( ?, ?, ?, ?, ? , ?, ?, ?, ?)");
        insertImagesStatement.bindValue(0, SeriesInstanceUID);
        insertImagesStatement.bindValue(1, SOPInstanceUID);
        insertImagesStatement.bindValue(2, PatientOrientation);
        insertImagesStatement.bindValue(3, ImageLaterality);
        insertImagesStatement.bindValue(4, ImageFileFullName);
        insertImagesStatement.bindValue(5, ViewName);
        insertImagesStatement.bindValue(6, ViewDescription);
        insertImagesStatement.bindValue(7, ViewPosition);
        insertImagesStatement.bindValue(8, InstanceNumber);

        if (!insertImagesStatement.exec())
        {
            qCritical() << ("Error executing statament: " + insertImagesStatement.lastQuery() + " Error: " + insertImagesStatement.lastError().text());
        }

        return 1;
    }
    else
    {
        return -1;
    }
    return -1;
}

QStringList DicomDatabase::getSopUIDByStudyUID(QString StudyUID)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT SOPInstanceUID FROM Series left join Images on Series.SeriesInstanceUID = Images.SeriesInstanceUID WHERE StudyInstanceUID= ? ");
    query.bindValue(0, StudyUID);
    query.exec();
    QStringList result;
    while (query.next())
    {
        QString tem = query.value(query.record().indexOf("SOPInstanceUID")).toString();
        result << tem;
    }

    return(result);
}

QStringList DicomDatabase::getImageStatusBySOPUID(const QString SOPInstanceUID)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT ImageStatus FROM Images WHERE SOPInstanceUID= ?");
    query.bindValue(0, SOPInstanceUID);
    query.exec();
    QStringList result;
    while (query.next())
    {
        result << query.value(0).toString();
    }

    return result;
}

QString DicomDatabase::getImageFileFullNameBySOPUID(const QString SOPUID)
{
    QString result;
    QSqlQuery query(m_database);
    query.prepare("SELECT ImageFileFullName FROM Images WHERE SOPInstanceUID= ?");
    query.bindValue(0, SOPUID);
    query.exec();
    if (query.next())
    {
        result = query.value(0).toString();
    }
    return result;
}

bool DicomDatabase::updateImagesInstanceNumberBySOPUID(QString SOPInstanceUID,  QString InstanceNumber)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE Images SET InstanceNumber = ? WHERE SOPInstanceUID = ?");
    query.bindValue(0, InstanceNumber);
    query.bindValue(1, SOPInstanceUID);
    return query.exec();
}

QStringList DicomDatabase::getSOPUIDAndSeriesUIDAndViewNameForStudy(const QString studyUID)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT Images.SOPInstanceUID,Images.SeriesInstanceUID, Images.ViewName, Images.InstanceNumber FROM Series LEFT JOIN Images ON Series.SeriesInstanceUID =Images.SeriesInstanceUID  WHERE StudyInstanceUID=? order by  Images.InstanceNumber asc");
    query.bindValue(0, studyUID);
    query.exec();
    QStringList result;
    while (query.next())
    {
        result << query.value(0).toString() + "|" + query.value(1).toString() + "|" + query.value(2).toString() + "|" + query.value(3).toString();
    }
    return(result);
}

QStringList DicomDatabase::viewDescriptionAndFileForInstance(QString sopInstanceUID)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT ImageFileFullName ,ViewDescription FROM Images WHERE SOPInstanceUID=?");
    query.bindValue(0, sopInstanceUID);
    query.exec();
    QStringList result;
    if (query.next())
    {
        result.append(query.value(0).toString());
        result.append(query.value(1).toString());
    }
    return result;
}

bool DicomDatabase::isExistStudyId(const QString id, bool isCaseSensitive)
{
    QSqlQuery query(m_database);
    QString sqlstr = QString("SELECT StudyInstanceUID FROM Studies WHERE StudyID = \"%1\"").arg(id);
    if (isCaseSensitive)
        sqlstr += QString(" or UPPER(StudyID) = \"%1\" or LOWER(StudyID) = \"%2\"").arg(id.toUpper()).arg(id.toLower());
    query.prepare(sqlstr);
    query.exec();
    return query.next();
}

QStringList DicomDatabase::getStudiesForPatient(const QString& patientUID)
{
    QStringList result;
    if (!checkDatabaseOpen()) return result;

    QSqlQuery query(m_database);
    query.prepare("SELECT StudyInstanceUID FROM Studies WHERE PatientUID = ?");
    query.bindValue(0, patientUID);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(query.value(0).toString());
    }

    return result;
}

QString DicomDatabase::getStudyLockedFromStudyUID(QString StudyUID)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT StudyLocked FROM Studies WHERE StudyInstanceUID= ?");
    query.bindValue(0, StudyUID);
    query.exec();
    QString result;
    if (query.next())
    {
        result = query.value(0).toString();
    }
    return result;
}

bool DicomDatabase::updateIsDeleteFromStudyUID(QString StudyUID, QString yes)
{
    QSqlQuery query(m_database);
    query.prepare("UPDATE Studies SET  IsDeleted = ? WHERE StudyInstanceUID = ?");
    query.bindValue(0, yes);
    query.bindValue(1, StudyUID);
    return query.exec();
}

QString DicomDatabase::getStudyUIDBySeriesUID(const QString& seriesUID)
{
    if (!checkDatabaseOpen()) return "";

    QSqlQuery query(m_database);
    query.prepare("SELECT StudyInstanceUID FROM Series WHERE SeriesInstanceUID = ?");
    query.bindValue(0, seriesUID);

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return "";
    }

    return query.value(0).toString();
}

QString DicomDatabase::getModalityForStudy(const QString& studyUID)
{
    if (!checkDatabaseOpen()) return "";

    QSqlQuery query(m_database);
    query.prepare("SELECT ModalitiesInStudy FROM Studies WHERE StudyInstanceUID = ?");
    query.bindValue(0, studyUID);

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return "";
    }

    return query.value(0).toString();
}

QStringList DicomDatabase::getSeriesForStudy(const QString& studyUID)
{
    QStringList result;
    if (!checkDatabaseOpen()) return result;

    QSqlQuery query(m_database);
    query.prepare("SELECT SeriesInstanceUID FROM Series WHERE StudyInstanceUID = ?");
    query.bindValue(0, studyUID);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(query.value(0).toString());
    }

    return result;
}

bool DicomDatabase::deleteForImagebySopUID(QString SopUID)
{
    QSqlQuery query(m_database);
    query.prepare("delete  FROM Images WHERE SOPInstanceUID= ?");
    query.bindValue(0, SopUID);
    return 	query.exec();
}

QStringList DicomDatabase::getInstancesForSeries(const QString& seriesUID)
{
    QStringList result;
    if (!checkDatabaseOpen()) return result;

    QSqlQuery query(m_database);
    query.prepare("SELECT SOPInstanceUID FROM Images WHERE SeriesInstanceUID = ?");
    query.bindValue(0, seriesUID);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(query.value(0).toString());
    }

    return result;
}

QString DicomDatabase::getFileForInstance(const QString& sopInstanceUID)
{
    if (!checkDatabaseOpen()) return "";

    QSqlQuery query(m_database);
    query.prepare("SELECT ImageFileFullName FROM Images WHERE SOPInstanceUID = ?");
    query.bindValue(0, sopInstanceUID);

    if (!query.exec() || !query.next()) {
        m_lastError = query.lastError().text();
        return "";
    }

    return query.value(0).toString();
}

QStringList DicomDatabase::getFilesForSeries(const QString& seriesUID)
{
    QStringList result;
    if (!checkDatabaseOpen()) return result;

    QSqlQuery query(m_database);
    query.prepare("SELECT ImageFileFullName FROM Images WHERE SeriesInstanceUID = ?");
    query.bindValue(0, seriesUID);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.append(query.value(0).toString());
    }

    return result;
}

bool DicomDatabase::deleteSeries(const QString& seriesUID)
{
    if (!checkDatabaseOpen()) return false;

    // 先删除所有图像
    QSqlQuery query(m_database);
    query.prepare("DELETE FROM Images WHERE SeriesInstanceUID = ?");
    query.bindValue(0, seriesUID);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    // 再删除序列
    query.prepare("DELETE FROM Series WHERE SeriesInstanceUID = ?");
    query.bindValue(0, seriesUID);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool DicomDatabase::deleteImage(const QString& sopInstanceUID)
{
    if (!checkDatabaseOpen()) return false;

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM Images WHERE SOPInstanceUID = ?");
    query.bindValue(0, sopInstanceUID);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

bool DicomDatabase::isExistsUseNameFormUserName(QString UserName)
{
    QSqlQuery query(m_database);
    QString sqlstr = QString("SELECT UserID FROM User WHERE UserName = '%1' ").arg(UserName);
    query.prepare(sqlstr);
    query.exec();
    return query.next();
}

QString DicomDatabase::getUserDisplayNameFormUserName(QString UserName)
{
    QSqlQuery query(m_database);
    QString sqlstr = QString("SELECT UserDisplayName FROM User WHERE UserName = '%1' ").arg(UserName);
    query.prepare(sqlstr);
    query.exec();
    QString result;
    if (query.next())
    {
        result = query.value(0).toString();
    }
    return result;
}

QString DicomDatabase::getPasswordFormUserName(QString UserName)
{
    QSqlQuery query(m_database);
    QString sqlstr = QString("SELECT Password FROM User WHERE UserName = '%1' ").arg(UserName);
    query.prepare(sqlstr);
    query.exec();
    QString result;
    if (query.next())
    {
        result = query.value(0).toString();
    }
    return result;
}

QStringList DicomDatabase::getUserLoginInfoFormUserName(const QString UserName)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT User.UserID, \
                        User.UserName,\
                        User.UserDisplayName,\
                        User.UserToken,\
                        User.Status ,\
                        Role.RoleID, \
                        Role.RoleName, \
                        Role.RoleDescription\
                        FROM User, UserRole, Role WHERE User.UserID =UserRole.UserID and  UserRole.RoleID = Role.RoleID and UserName=?");
    query.bindValue(0, UserName);
    query.exec();
    QStringList result;
    while (query.next())
    {
        result << query.value(0).toString() << query.value(1).toString() << query.value(2).toString() << query.value(3).toString() << query.value(4).toString() << query.value(5).toString() << query.value(6).toString() << query.value(7).toString();
    }
    return(result);
}


bool DicomDatabase::insertUserLogin(QString LoginID, QString UserID, QString LoginDateTime, QString LoginState)
{
    QSqlQuery insertStatement(m_database);
    insertStatement.prepare("INSERT INTO UserLogin ('LoginID', 'UserID','LoginDateTime' ,'LoginState' ) values ( ?, ?, ?, ?)");
    insertStatement.bindValue(0, LoginID);
    insertStatement.bindValue(1, UserID);
    insertStatement.bindValue(2, LoginDateTime);
    insertStatement.bindValue(3, LoginState);
    return insertStatement.exec();
}
