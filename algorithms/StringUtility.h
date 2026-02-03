#ifndef STRINGUTILITY_H
#define STRINGUTILITY_H

#include <string>
#include <vector>
#include <QString>
#include <QStringList>

#include <QDateTime>

namespace StringUtility {

int Split(std::string&,std::vector<std::string>,char sep);//, Qt::SplitBehavior behavior = Qt::KeepEmptyParts


float* DataCorrectPara(const std::string & data, int * count);

float* ProcParaPara(const std::string & data, unsigned int * count);

QDateTime StringToDatetime(const QString datestring);
QDateTime StringToDatetime(const QString datestring, const QString format);

QDate StringToDate(const QString datestring);
QDate StringToDate(const QString datestring, const QString format);

QTime StringToTime(const QString datestring);
QTime StringToTime(const QString datestring, const QString format);

uint CalculateAge(const QDate &birthDate, const QDate &targetDate);
}

#endif // STRINGUTILITY_H
