# ============ Qt 版本配置 ============
QT_VERSION = 6
QT_MAJOR_VERSION = 6
QT_MINOR_VERSION = 5

# ============ Qt 模块 ============
QT += core gui sql xml serialport concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# ============ 项目配置 ============
CONFIG += c++11 link_pkgconfig
TARGET = XVBVThickness
TEMPLATE = app

# ============ zlib 和 minizip 配置（树莓派专用）============
message("Checking for zlib and minizip libraries...")

# 添加 pkg-config 支持
PKGCONFIG += zlib

# 手动添加 minizip
isEmpty(MINIZIP_PATH) {
    # 检查可能的 minizip 库位置
    exists(/usr/lib/aarch64-linux-gnu/libminizip.so) {
        MINIZIP_PATH = /usr/lib/aarch64-linux-gnu
    } else:exists(/usr/lib/arm-linux-gnueabihf/libminizip.so) {
        MINIZIP_PATH = /usr/lib/arm-linux-gnueabihf
    } else:exists(/usr/lib/libminizip.so) {
        MINIZIP_PATH = /usr/lib
    }
}

!isEmpty(MINIZIP_PATH) {
    LIBS += -L$$MINIZIP_PATH -lminizip
    message("Found minizip at: $$MINIZIP_PATH")
} else {
    # 如果找不到，尝试链接 z 库
    LIBS += -lz
    message("minizip not found, using -lz instead")
}

# ============ 必要的包含路径 ============
INCLUDEPATH += /usr/include

# ============ DataProc 库配置 ============
DataProc_DIR = /home/pi/algorithm/DataProc-build

# 添加包含路径
INCLUDEPATH += /home/pi/algorithm/DataProc/include

# 添加库路径和库文件
LIBS += -L$$DataProc_DIR -lDataProc

# 添加运行时库路径（树莓派需要）
QMAKE_RPATHDIR += $$DataProc_DIR

# ============ OpenCV 配置（如果使用）============
# PKGCONFIG += opencv4
# 或者手动指定
# INCLUDEPATH += /usr/include/opencv4
# LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui

# ============ VTK 配置（如果使用）============
# 根据你的安装路径调整
# INCLUDEPATH += /home/pi/vtk-9.4.2-build/include
# LIBS += -L/home/pi/vtk-9.4.2-build/lib -lvtkCommonCore -lvtkFiltersCore

# ============ 源文件配置 ============
SOURCES += \
    XV.Communication/ModbusCRC16.cpp \
    XV.Communication/SensorCRC16.cpp \
    XV.Communication/XProtocolManager.cpp \
    XV.Communication/XRayProtocol.cpp \
    XV.Communication/XSensorProtocol.cpp \
    XV.Control/InteractiveImageLabel.cpp \
    XV.Control/XDCableDiameterWidget.cpp \
    XV.Control/XDCursorPosCalculator.cpp \
    XV.Control/XDCustomWindow.cpp \
    XV.Control/XDDiameterHistoryWidget.cpp \
    XV.Control/XDFlowLayout.cpp \
    XV.Control/XDFramelessHelper.cpp \
    XV.Control/XDShadowWidget.cpp \
    XV.Control/XDSliderControl.cpp \
    XV.Control/XDThumbnailLabel.cpp \
    XV.Control/XDThumbnailListWidget.cpp \
    XV.Control/XDTitleBar.cpp \
    XV.Control/XDWidgetData.cpp \
    XV.Control/XVComboBoxItemDelegate.cpp \
    XV.DataAccess/mvdicomdatabase.cpp \
    XV.Model/mvworklistitemmodel.cpp \
    XV.Tool/SettingsHelper.cpp \
    XV.Tool/WidgetHelper.cpp \
    XV.Tool/XDConfigFileManager.cpp \
    XV.Tool/XPCompression.cpp \
    XV.Tool/XVMessageBox.cpp \
    XV.Tool/mvSessionHelper.cpp \
    XV.Tool/mvXmlOptionItem.cpp \
    XV.Tool/mvdatalocations.cpp \
    XV.Tool/mvharddiskinformation.cpp \
    algorithms/CImageProcess.cpp \
    algorithms/ImageProConfig.cpp \
    algorithms/PostProcPara.cpp \
    algorithms/StringUtility.cpp \
    algorithms/XPectAlgorithmic.cpp \
    algorithms/tinyxml2.cpp \
    main.cpp \
    mainwindow.cpp \
    mvaboutwidget.cpp \
    mvexitappdialog.cpp \
    mvimageacquisitwidget.cpp \
    mvloginwindow.cpp \
    mvnewpatientdialog.cpp \
    mvpatientrecordwidget.cpp \
    mvstatusbar.cpp \
    mvsystemsettings.cpp

HEADERS += \
    XV.Communication/ModbusCRC16.h \
    XV.Communication/SensorCRC16.h \
    XV.Communication/XProtocolManager.h \
    XV.Communication/XRayProtocol.h \
    XV.Communication/XSensorProtocol.h \
    XV.Control/InteractiveImageLabel.h \
    XV.Control/SignalWaiter.h \
    XV.Control/XDCableDiameterWidget.h \
    XV.Control/XDCursorPosCalculator.h \
    XV.Control/XDCustomWindow.h \
    XV.Control/XDDiameterHistoryWidget.h \
    XV.Control/XDFlowLayout.h \
    XV.Control/XDFramelessHelper.h \
    XV.Control/XDFramelessHelperPrivate.h \
    XV.Control/XDShadowWidget.h \
    XV.Control/XDShadowWindow.h \
    XV.Control/XDSliderControl.h \
    XV.Control/XDThumbnailLabel.h \
    XV.Control/XDThumbnailListWidget.h \
    XV.Control/XDTitleBar.h \
    XV.Control/XDWidgetData.h \
    XV.Control/XVComboBoxItemDelegate.h \
    XV.DataAccess/mvdicomdatabase.h \
    XV.Model/mvstudyrecord.h \
    XV.Model/mvworklistitemmodel.h \
    XV.Tool/SettingsHelper.h \
    XV.Tool/WidgetHelper.h \
    XV.Tool/XDConfigFileManager.h \
    XV.Tool/XPCompression.h \
    XV.Tool/XVMessageBox.h \
    XV.Tool/mvSessionHelper.h \
    XV.Tool/mvSimpleCrypto.h \
    XV.Tool/mvXmlOptionItem.h \
    XV.Tool/mvconfig.h \
    XV.Tool/mvdatalocations.h \
    XV.Tool/mvharddiskinformation.h \
    XV.Tool/mvqss.h \
    algorithms/CImageProcess.h \
    algorithms/GlobalDef.h \
    algorithms/ImageProConfig.h \
    algorithms/PostProcPara.h \
    algorithms/StringUtility.h \
    algorithms/XPectAlgorithmic.h \
    algorithms/tinyxml2.h \
    mainwindow.h \
    mvaboutwidget.h \
    mvexitappdialog.h \
    mvimageacquisitwidget.h \
    mvloginwindow.h \
    mvnewpatientdialog.h \
    mvpatientrecordwidget.h \
    mvstatusbar.h \
    mvsystemsettings.h

FORMS += \
    XV.Control/XDThumbnailLabel.ui \
    XV.Control/XDThumbnailListWidget.ui \
    mvaboutwidget.ui \
    mvexitappdialog.ui \
    mvimageacquisitwidget.ui \
    mvloginwindow.ui \
    mvnewpatientdialog.ui \
    mvpatientrecordwidget.ui \
    mvsystemsettings.ui

# ============ 包含路径 ============
INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/XV.Control
INCLUDEPATH += $$PWD/XV.Communication
INCLUDEPATH += $$PWD/XV.DataAccess
INCLUDEPATH += $$PWD/XV.Model
INCLUDEPATH += $$PWD/XV.Tool
INCLUDEPATH += $$PWD/algorithms

# ============ 编译设置 ============
# 根据树莓派架构优化
isEqual(QMAKE_HOST.arch, arm*) {
    # ARM架构优化（适用于树莓派4B）
    QMAKE_CXXFLAGS += -march=armv8-a+crc -mtune=cortex-a72 -mfpu=neon-fp-armv8 -mfloat-abi=hard
}

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS_RELEASE += -O2 -pipe
QMAKE_CXXFLAGS_DEBUG += -g -O0 -Wall -Wextra

# 如果是 QT6，使用C++17
greaterThan(QT_MAJOR_VERSION, 5) {
    QT += widgets
    QMAKE_CXXFLAGS += -std=c++17
}

# ============ 资源文件 ============
RESOURCES += \
    mvResources.qrc

# ============ 翻译文件 ============
TRANSLATIONS += \
    XVBVThickness_zh_CN.ts

# ============ 部署设置 ============
# 树莓派运行时库路径
QMAKE_LFLAGS += -Wl,-rpath,'$$ORIGIN/lib'

# ============ 分 Debug/Release 的输出路径 ============
# 基础构建路径
BUILD_BASE = $$PWD/build

# 根据配置选择路径
CONFIG(debug, debug|release) {
    BUILD_TYPE = debug
    # 可选：给 Debug 版本添加后缀
    # TARGET = $$join(TARGET,,,_debug)
} else {
    BUILD_TYPE = release
}

# 设置所有输出路径
BUILD_PATH = $${BUILD_BASE}/$${BUILD_TYPE}
DESTDIR = $${BUILD_PATH}
MOC_DIR = $${BUILD_PATH}/.moc
RCC_DIR = $${BUILD_PATH}/.rcc
UI_DIR = $${BUILD_PATH}/.ui
OBJECTS_DIR = $${BUILD_PATH}/.obj

# 确保目录存在
!exists($$BUILD_PATH) {
    mkpath($$BUILD_PATH)
}

# 也可以为中间文件创建子目录
!exists($${BUILD_PATH}/.moc) { mkpath($${BUILD_PATH}/.moc) }
!exists($${BUILD_PATH}/.rcc) { mkpath($${BUILD_PATH}/.rcc) }
!exists($${BUILD_PATH}/.ui) { mkpath($${BUILD_PATH}/.ui) }
!exists($${BUILD_PATH}/.obj) { mkpath($${BUILD_PATH}/.obj) }

unix:QMAKE_LFLAGS += -Wl,-rpath,\'\$$ORIGIN/libs\'
