#include "SettingsHelper.h"

SettingsHelper::SettingsHelper(const QString& fileName, QSettings::Format format)
    : m_settings(fileName, format)
{}

void SettingsHelper::setValue(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&m_mutex);
    m_settings.setValue(key, value);
}

QVariant SettingsHelper::value(const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&m_mutex);
    return m_settings.value(key, defaultValue);
}

void SettingsHelper::sync()
{
    QMutexLocker locker(&m_mutex);
    m_settings.sync();
}
