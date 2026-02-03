#pragma once

#include "mvconfig.h"
#include "mvdatalocations.h"
#include "vector"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>

#include <QFile>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QtXml>

class ConfigFileManager : public QObject
{
    Q_OBJECT

public:
    static ConfigFileManager* getInstance()
    {
        if (m_pInstance == NULL) {
            QMutexLocker mlocker(&m_Mutex);
            if (m_pInstance == NULL) {
                m_pInstance = new ConfigFileManager();
            }
        }
        return m_pInstance;
    };
    class CGarbo
    {
    public:
        ~CGarbo()
        {
            if (m_pInstance != NULL) {
                delete m_pInstance;
                m_pInstance = NULL;
            }
        }
    };

private:
    static CGarbo m_Garbo;
    static ConfigFileManager* m_pInstance;
    static QMutex m_Mutex;

public:
    ConfigFileManager();
    ~ConfigFileManager();

    QString getValue(QString key);
    void setValue(QString key, QString value);

private:
    QSettings* mQs;
};
