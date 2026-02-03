#pragma once

// 使用正确的 Qt6 头文件包含
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QStandardPaths>

#include <string>

namespace mv
{
    class DataLocations
    {
    public:
        static bool mRunFromBuildFolder;
        static bool mBuildFolderChecked;

        static QString getBundlePath();
        static QString getRootConfigPath();
        static QStringList getRootConfigPaths();
        static QString getDicomPath();
    };
}
