#ifndef SETTINGSHELPER_H
#define SETTINGSHELPER_H

#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QtCore/QString>
#include <QtCore/QVariant>

/**
 * @brief The SettingsHelper 类
 * 提供线程安全的 INI 文件读写功能。
 */
class SettingsHelper
{
public:
    /**
     * @brief 构造函数
     * @param fileName 配置文件路径
     * @param format 配置格式，默认为 IniFormat
     */
    explicit SettingsHelper(const QString& fileName,
                            QSettings::Format format = QSettings::IniFormat);

    /**
     * @brief 设置某个键值（线程安全）
     * @param key 键名（支持 group/key 形式，如 "User/name"）
     * @param value 要设置的值
     */
    void setValue(const QString& key, const QVariant& value);

    /**
     * @brief 获取某个键的值（线程安全）
     * @param key 键名（支持 group/key 形式，如 "User/name"）
     * @param defaultValue 默认值
     * @return QVariant 实际值或默认值
     */
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /**
     * @brief 同步：将缓存中的更改写入磁盘
     */
    void sync();

private:
    mutable QMutex m_mutex; // mutable 允许在 const 方法中加锁
    QSettings m_settings;   // QSettings 实例
};

#endif // SETTINGSHELPER_H
