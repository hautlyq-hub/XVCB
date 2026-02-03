#include "StringUtility.h"

#include <QDebug>

int StringUtility::Split(std::string& data, std::vector<std::string> vecDest, char sep)
{
    // 1. 分割字符串获取键值对列表
    QStringList paramsList = QString::fromStdString(data).split(sep, Qt::SkipEmptyParts);
   int count = paramsList.size();
    // clear
    vecDest.clear();
    vecDest.reserve(count);
    // 2. 为空检查
    if(count==0)
    {
        return 0;
    }
    // 4. 遍历解析每个参数
    for (int i = 0; i < count; ++i) {
        QString keyValue = paramsList[i];
        vecDest.push_back(keyValue.toStdString());
    }
    return count;
}















float *StringUtility::DataCorrectPara(const std::string &data, int *count)
{
    // 1. 分割字符串获取键值对列表
    QStringList paramsList = QString::fromStdString(data).split(';', Qt::SkipEmptyParts);
    *count = paramsList.size();
    // 2. 为空检查
    if (count == 0){
        qWarning() << "Parameter conversion failed: no valid data";
        return nullptr;
    }
    // 3. 分配内存
    float* resultArray = new float[*count];
    // 4. 遍历解析每个参数
    for (int i = 0; i < *count; ++i) {
        QStringList keyValue = paramsList[i].split('=');
        bool conversionOK = false;
        float value = 0.0f;
        // 格式正确时尝试转换
        if (keyValue.size() >= 2) {
            value = keyValue[1].toFloat(&conversionOK);
        }
        // 转换失败时记录错误信息
        if (!conversionOK) {
            qWarning() << "Parameter conversion failed: " << paramsList[i];
        }
        resultArray[i] = value;
    }
    return resultArray;
}


float *StringUtility::ProcParaPara(const std::string &data, unsigned int *count)
{
   QString temp = QString::fromStdString(data);
    temp.replace(",", ";");
    temp.replace("(", "");
    temp.replace(")", "");
    temp.replace(" ", "");
    qDebug() <<"ProcParaPara: "<< temp;
    QStringList paramsList = temp.split(';', Qt::SkipEmptyParts);
    *count = paramsList.size();
    if (count == 0){
        qWarning() << "Parameter conversion failed: no valid data";
        return nullptr;
    }
    float* resultArray = new float[*count];
    // 4. 遍历解析每个参数
    for (int i = 0; i < *count; ++i) {
        QStringList keyValue = paramsList[i].split('=');
        bool conversionOK = false;
        float value = 0.0f;
        // 格式正确时尝试转换
        if (keyValue.size() >= 2)
        {
            value = keyValue[1].toFloat(&conversionOK);
        }
        else
        {
            value = keyValue[0].toFloat(&conversionOK);
        }
        // 转换失败时记录错误信息
        if (!conversionOK) {
            qWarning() << "Parameter conversion failed: " << paramsList[i];
        }
        resultArray[i] = value;
    }
    return resultArray;
}

QDateTime StringUtility::StringToDatetime(const QString datestring)
{
    QStringList formats;
    formats << "yyyy-MM-dd HH:mm:ss" << "dd/MM/yyyy HH:mm:ss" << "MM/dd/yyyy HH:mm:ss"
            << "yyyy.MM.dd HH:mm:ss" << "yyyyMMddHHmmss"
            << "yyyy-MM-ddTHH:mm:ss" << "dd/MM/yyyyTHH:mm:ss" << "MM/dd/yyyyTHH:mm:ss"
            << "yyyy.MM.ddTHH:mm:ss" << "yyyyMMddTHHmmss";
    QDateTime date;
    for (const QString &format : formats) {
        date = QDateTime::fromString(datestring, format);
        if (date.isValid()) {
            break;
        }
    }
    return date;
}

QDateTime StringUtility::StringToDatetime(const QString datestring, const QString format)
{
    QDateTime date = QDateTime::fromString(datestring, format);
    if (date.isValid()) {
        return date;
    } else {
        return StringToDatetime(datestring);
    }
}

QDate StringUtility::StringToDate(const QString datestring)
{
    QStringList formats;
    formats << "yyyy-MM-dd" << "dd/MM/yyyy" << "MM/dd/yyyy" << "yyyy.MM.dd" << "yyyyMMdd";
    QDate date;
    for (const QString &format : formats) {
        date = QDate::fromString(datestring, format);
        if (date.isValid()) {
            break;
        }
    }
    return date;
}

QDate StringUtility::StringToDate(const QString datestring, const QString format)
{
    QDate date = QDate::fromString(datestring, format);
    if (date.isValid()) {
        return date;
    } else {
        return StringToDate(datestring);
    }
}

QTime StringUtility::StringToTime(const QString datestring)
{
    QStringList formats;
    formats << "HH:mm:ss" << "HHmmss";
    QTime date;
    for (const QString &format : formats) {
        date = QTime::fromString(datestring, format);
        if (date.isValid()) {
            break;
        }
    }
    return date;
}

QTime StringUtility::StringToTime(const QString datestring, const QString format)
{
    QTime date = QTime::fromString(datestring, format);
    if (date.isValid()) {
        return date;
    } else {
        return StringToTime(datestring);
    }
}

uint StringUtility::CalculateAge(const QDate &birthDate, const QDate &targetDate)
{
    uint age = targetDate.year() - birthDate.year();
    // 检查是否还未过生日
    QDate thisYearsBirthday = QDate(targetDate.year(), birthDate.month(), birthDate.day());
    if (thisYearsBirthday > targetDate) {
        age--; // 今年生日还没到
    }
    return age;
}
