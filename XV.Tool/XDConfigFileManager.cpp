

#include "XDConfigFileManager.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

ConfigFileManager* ConfigFileManager::m_pInstance = NULL;
ConfigFileManager::CGarbo m_Garbo;
QMutex ConfigFileManager::m_Mutex;

ConfigFileManager::ConfigFileManager()
{
    QString pathIni = DataLocations::getRootConfigPath() + XV_APPSETTING;

    // 2. 直接测试文件读取
    QFile file(pathIni);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Can read file, first line:" << file.readLine().trimmed();
        file.close();
    } else {
        qDebug() << "Cannot open file:" << file.errorString();
    }

    // 3. QSettings 测试
    mQs = new QSettings(pathIni, QSettings::IniFormat);
}

ConfigFileManager::~ConfigFileManager()
{
    if (mQs) {
        delete mQs;
        mQs = nullptr;
    }
}
QString ConfigFileManager::getValue(QString key)
{
    return mQs->value(key).toString();
}
void ConfigFileManager::setValue(QString key, QString value)
{
    mQs->setValue(key, value);
}
