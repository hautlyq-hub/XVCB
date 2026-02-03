#include "mvdatalocations.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>

#include "XV.Tool/mvconfig.h"

bool DataLocations::mRunFromBuildFolder = false;
bool DataLocations::mBuildFolderChecked = false;

QString DataLocations::getBundlePath()
{
    // 树莓派上直接返回应用目录
    return QCoreApplication::applicationDirPath();
}

QString DataLocations::getRootConfigPath()
{
    QStringList paths = getRootConfigPaths();
    if (paths.empty()) {
        // 返回一个默认配置路径
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }
    return paths.back();
}

QStringList DataLocations::getRootConfigPaths()
{
    QString path = QString("%1/%2")
                       .arg(QCoreApplication::applicationDirPath())
                       .arg(MV_CONFIG_ROOT_RELATIVE_INSTALLED);

    QDir dir(path);
    if (dir.exists()) {
        return QStringList() << dir.canonicalPath();
    } else {
        // 尝试创建目录
        if (dir.mkpath(".")) {
            qDebug() << "Created config directory:" << dir.canonicalPath();
            return QStringList() << dir.canonicalPath();
        } else {
            qWarning() << "Failed to create config directory:" << path;
            return QStringList();
        }
    }
}

QString DataLocations::getDicomPath()
{
    // 方法1：使用相对路径
    QString relativePath
        = QString("%1/%2").arg(QCoreApplication::applicationDirPath()).arg("../ImageDB");
    QDir dir(relativePath);

    if (dir.exists()) {
        return dir.canonicalPath();
    }

    // 方法2：尝试创建目录
    if (dir.mkpath(".")) {
        qDebug() << "Created DICOM directory:" << dir.canonicalPath();
        return dir.canonicalPath();
    }

    // 方法3：使用备用位置（用户目录）
    qWarning() << "Failed to create DICOM directory at" << relativePath
               << ", using fallback location";

    QString fallbackPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                           + "/ImageDB";
    QDir fallbackDir(fallbackPath);

    if (fallbackDir.mkpath(".")) {
        return fallbackDir.canonicalPath();
    }

    // 方法4：最后使用/tmp
    QString tmpPath = "/tmp/ImageDB";
    QDir tmpDir(tmpPath);
    tmpDir.mkpath(".");
    return tmpDir.canonicalPath();
}
