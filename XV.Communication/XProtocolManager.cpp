#include "XProtocolManager.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSerialPortInfo>
#include <QStandardPaths>
#include <QThread>
#include <QtConcurrent>
#include "ModbusCRC16.h"
#include <memory>
#include <stdexcept>

#ifdef QT_DEBUG
#define DEBUG_SINGLE_DEVICE // 调试模式下使用单套设备
#endif

// 常量定义
const QString XProtocolManager::SENSOR1_KEY = "sensor1";
const QString XProtocolManager::SENSOR2_KEY = "sensor2";
const QString XProtocolManager::XRAY1_KEY = "xray1";
const QString XProtocolManager::XRAY2_KEY = "xray2";

XProtocolManager::XProtocolManager(QObject* parent)
    : QObject(parent)
    , m_threadPool(new QThreadPool(this))
    , m_setupWatcher(new QFutureWatcher<bool>(this))
    , m_exposureWatcher(new QFutureWatcher<void>(this))
    , m_sensor1Thread(nullptr)
    , m_sensor2Thread(nullptr)
    , m_xray1Thread(nullptr)
    , m_xray2Thread(nullptr)
    , m_sensor1(nullptr)
    , m_sensor2(nullptr)
    , m_xrayProtocol1(nullptr)
    , m_xrayProtocol2(nullptr)
    , m_isExposing(0)
    , m_isSettingUp(0)
    , m_cancelRequested(0)
    , m_xRayIndex(0)
    , m_setupTimeoutMs(15000)
    , m_exposureTimeoutMs(35000)
{
    // 设置线程池
    m_threadPool->setMaxThreadCount(4);

    // 初始化定时器
    m_comCheckTimer = new QTimer(this);
    m_comCheckTimer->setInterval(3000);

    m_exposureTimer = new QTimer(this);
    m_exposureTimer->setSingleShot(true);

    m_setupTimer = new QTimer(this);
    m_setupTimer->setSingleShot(true);

    m_xrayCheckTimer = new QTimer(this);
    m_xrayCheckTimer->setInterval(3000);

    // 连接信号槽
    connect(m_comCheckTimer, &QTimer::timeout, this, &XProtocolManager::onCheckComPorts);
    connect(m_exposureTimer, &QTimer::timeout, this, &XProtocolManager::onExposureTimeout);
    connect(m_setupTimer, &QTimer::timeout, this, &XProtocolManager::onSetupTimeout);
    connect(m_xrayCheckTimer, &QTimer::timeout, this, &XProtocolManager::updateConnectionStatus);

    // 初始化默认参数
    m_exposureParams.kv_val = 50.0f;
    m_exposureParams.exp_time_ms = 500;
    m_exposureParams.ma_val = 0.5f;
    m_exposureParams.waitXray = true;
}

XProtocolManager::~XProtocolManager()
{
    unInitSerialPort();

    if (m_threadPool) {
        m_threadPool->waitForDone();
    }
}

bool XProtocolManager::isHardwareReady() const
{
#ifdef DEBUG_SINGLE_DEVICE
    // 调试模式：只需要一个传感器和一个X光机
    bool hasSensor = m_deviceStatus.sensor1Connected; // 只检查sensor1
    bool hasXray = m_deviceStatus.xray1Connected;     // 只检查xray1
#else
    // 发布模式：需要两个传感器和两个X光机
    bool hasSensor = m_deviceStatus.sensor1Connected || m_deviceStatus.sensor2Connected;
    bool hasXray = m_deviceStatus.xray1Connected || m_deviceStatus.xray2Connected;
#endif
    return hasSensor && hasXray;
}

bool XProtocolManager::initSerialPort()
{
    loadConfig();

    initializeSensors();

    initializeXray();

    if (m_sensor1Thread)
        m_sensor1Thread->start();
    if (m_sensor2Thread)
        m_sensor2Thread->start();
    if (m_xray1Thread)
        m_xray1Thread->start();
    if (m_xray2Thread)
        m_xray2Thread->start();

    // 使用 QFuture 收集所有连接任务
    QList<QFuture<bool>> futures;

    // 传感器1连接
    futures.append(
        QtConcurrent::run(m_threadPool, [this]() -> bool { return connectSensor(SENSOR1_KEY); }));

#ifndef DEBUG_SINGLE_DEVICE
    // 传感器2连接
    futures.append(
        QtConcurrent::run(m_threadPool, [this]() -> bool { return connectSensor(SENSOR2_KEY); }));
#endif

    // X光机1连接
    futures.append(
        QtConcurrent::run(m_threadPool, [this]() -> bool { return connectXray(XRAY1_KEY); }));

#ifndef DEBUG_SINGLE_DEVICE
    // X光机2连接
    futures.append(
        QtConcurrent::run(m_threadPool, [this]() -> bool { return connectXray(XRAY2_KEY); }));
#endif

    bool allConnected = true;
    for (int i = 0; i < futures.size(); i++) {
        bool result = futures[i].result();
        if (!result) {
            allConnected = false;
        }
    }

    updateConnectionStatus();

    // 启动定期检查
    // m_comCheckTimer->start();
    // m_xrayCheckTimer->start();

    return allConnected;
}

void XProtocolManager::unInitSerialPort()
{
    // 设置取消标志
    m_cancelRequested = 1;
    m_isExposing = 0;
    m_isSettingUp = 0;

    // 停止所有定时器
    m_comCheckTimer->stop();
    m_exposureTimer->stop();
    m_setupTimer->stop();
    m_xrayCheckTimer->stop();

    // 清空图像缓存
    m_imageBuffer.clear();

    if (m_sensor1) {
        QMetaObject::invokeMethod(m_sensor1, "closeSerialPort", Qt::BlockingQueuedConnection);
    }

#ifndef DEBUG_SINGLE_DEVICE
    if (m_sensor2) {
        QMetaObject::invokeMethod(m_sensor2, "closeSerialPort", Qt::BlockingQueuedConnection);
    }
#endif

    if (m_xrayProtocol1) {
        QMetaObject::invokeMethod(m_xrayProtocol1, "closeSerialPort", Qt::BlockingQueuedConnection);
    }

#ifndef DEBUG_SINGLE_DEVICE
    if (m_xrayProtocol2) {
        QMetaObject::invokeMethod(m_xrayProtocol2, "closeSerialPort", Qt::BlockingQueuedConnection);
    }
#endif

    QThread::msleep(1000);

    if (m_sensor1Thread && m_sensor1Thread->isRunning()) {
        m_sensor1Thread->quit();
        m_sensor1Thread->wait(1000);
    }

#ifndef DEBUG_SINGLE_DEVICE
    if (m_sensor2Thread && m_sensor2Thread->isRunning()) {
        m_sensor2Thread->quit();
        m_sensor2Thread->wait(1000);
    }
#endif

    if (m_xray1Thread && m_xray1Thread->isRunning()) {
        m_xray1Thread->quit();
        m_xray1Thread->wait(1000);
    }

#ifndef DEBUG_SINGLE_DEVICE
    if (m_xray2Thread && m_xray2Thread->isRunning()) {
        m_xray2Thread->quit();
        m_xray2Thread->wait(1000);
    }
#endif
}

void XProtocolManager::loadConfig()
{
    m_portSettings.clear();

    // 加载传感器1配置
    QString sensor1Port = ConfigFileManager::getInstance()->getValue("sensor1/value");
    QString sensor1Desc = ConfigFileManager::getInstance()->getValue("sensor1/description");
    int sensor1Ori = ConfigFileManager::getInstance()->getValue("sensor1/orientation").toInt();

    m_portSettings.append({"sensor1", sensor1Port, sensor1Desc, sensor1Ori, false});

    // 加载传感器2配置
    QString sensor2Port = ConfigFileManager::getInstance()->getValue("sensor2/value");
    QString sensor2Desc = ConfigFileManager::getInstance()->getValue("sensor2/description");
    int sensor2Ori = ConfigFileManager::getInstance()->getValue("sensor2/orientation").toInt();

    m_portSettings.append({"sensor2", sensor2Port, sensor2Desc, sensor2Ori, false});

    // 加载X光机1配置
    QString xray1Port = ConfigFileManager::getInstance()->getValue("xray1/value");
    QString xray1Desc = ConfigFileManager::getInstance()->getValue("xray1/description");
    int xray1Ori = ConfigFileManager::getInstance()->getValue("xray1/orientation").toInt();

    m_portSettings.append({"xray1", xray1Port, xray1Desc, xray1Ori, false});

    // 加载X光机2配置
    QString xray2Port = ConfigFileManager::getInstance()->getValue("xray2/value");
    QString xray2Desc = ConfigFileManager::getInstance()->getValue("xray2/description");
    int xray2Ori = ConfigFileManager::getInstance()->getValue("xray2/orientation").toInt();

    m_portSettings.append({"xray2", xray2Port, xray2Desc, xray2Ori, false});

    QString appDataPath = DataLocations::getRootConfigPath() + XV_APPDATA;
    QFile file(appDataPath);

    if (file.open(QIODevice::ReadOnly)) {
        QDomDocument doc;
        if (doc.setContent(&file)) {
            QDomElement root = doc.documentElement();

            QDomNodeList items = root.elementsByTagName("item"); // 或者根据实际XML结构调整
            for (int i = 0; i < items.count(); ++i) {
                QDomElement elem = items.at(i).toElement();
                if (elem.attribute("key") == "NextSecondsInterval") {
                    m_nextSecondsInterval = elem.attribute("value").toInt();
                    break;
                }
            }
        }
        file.close();
    } else {
        qWarning() << "Failed to load AppData config, using default interval";
    }
}

void XProtocolManager::initializeSensors()
{
    // 创建传感器实例
    m_sensor1Thread = new QThread(this);
    m_sensor1Thread->setObjectName("Sensor1-Thread");

    m_sensor2Thread = new QThread(this);
    m_sensor2Thread->setObjectName("Sensor2-Thread");

    m_sensor1 = new XSensorProtocol();
    m_sensor2 = new XSensorProtocol();

    // 移动传感器到专用线程
    m_sensor1->moveToThread(m_sensor1Thread);
    m_sensor2->moveToThread(m_sensor2Thread);

    connect(m_sensor1,
            &XSensorProtocol::exposureF5Ready,
            this,
            &XProtocolManager::onExposureF5Ready,
            Qt::QueuedConnection);

    connect(m_sensor1,
            &XSensorProtocol::exposureF6Ready,
            this,
            &XProtocolManager::onExposureF6Ready,
            Qt::QueuedConnection);

    connect(m_sensor2,
            &XSensorProtocol::exposureF5Ready,
            this,
            &XProtocolManager::onExposureF5Ready,
            Qt::QueuedConnection);

    connect(m_sensor2,
            &XSensorProtocol::exposureF6Ready,
            this,
            &XProtocolManager::onExposureF6Ready,
            Qt::QueuedConnection);

    connect(m_sensor1,
            &XSensorProtocol::statusChanged,
            this,
            &XProtocolManager::onSensorStatusChanged,
            Qt::QueuedConnection);

    connect(
        m_sensor1,
        &XSensorProtocol::imageReady,
        this,
        [this](const QByteArray& imageData) {
            HWImageData image;
            image.imageData = imageData;
            image.width = 344;
            image.height = 417;
            image.bitDepth = 16;
            onSensorImageAvailable(image, m_sensor1->portName());
        },
        Qt::QueuedConnection);

    connect(
        m_sensor1,
        &XSensorProtocol::exposureCompleted,
        this,
        [this]() { emit exposureProcess(ExposureState::Idle); },
        Qt::QueuedConnection);

    connect(m_sensor2,
            &XSensorProtocol::statusChanged,
            this,
            &XProtocolManager::onSensorStatusChanged,
            Qt::QueuedConnection);

    connect(
        m_sensor2,
        &XSensorProtocol::imageReady,
        this,
        [this](const QByteArray& imageData) {
            HWImageData image;
            image.imageData = imageData;
            image.width = 344;
            image.height = 417;
            image.bitDepth = 16;
            image.filePath = QString("sensor_%1.raw")
                                 .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz"));

            onSensorImageAvailable(image, m_sensor2->portName());
        },
        Qt::QueuedConnection);

    connect(
        m_sensor1,
        &XSensorProtocol::exposureCompleted,
        this,
        [this]() { emit exposureProcess(ExposureState::Idle); },
        Qt::QueuedConnection);

    qInfo() << "Sensors initialized";
}

void XProtocolManager::initializeXray()
{

    m_xray1Thread = new QThread(this);
    m_xray1Thread->setObjectName("Xray1-Thread");

    m_xray2Thread = new QThread(this);
    m_xray2Thread->setObjectName("Xray2-Thread");

    m_xrayProtocol1 = new XRayProtocol();
    m_xrayProtocol2 = new XRayProtocol();

    // 移动X光机到专用线程
    m_xrayProtocol1->moveToThread(m_xray1Thread);
    m_xrayProtocol2->moveToThread(m_xray2Thread);

    // 连接X光信号
    connect(
        m_xrayProtocol1,
        &XRayProtocol::deviceConnected,
        this,
        [this]() { onXrayDeviceConnected(0); },
        Qt::QueuedConnection);
    connect(
        m_xrayProtocol1,
        &XRayProtocol::deviceDisconnected,
        this,
        [this]() { onXrayDeviceDisconnected(0); },
        Qt::QueuedConnection);
    connect(
        m_xrayProtocol1,
        &XRayProtocol::deviceStatusChanged,
        this,
        [this](XRayDeviceStatus status) { onXrayStatusChanged(status, 0); },
        Qt::QueuedConnection);
    connect(
        m_xrayProtocol1,
        &XRayProtocol::exposureStarted,
        this,
        [this]() { onXrayExposureStarted(0); },
        Qt::QueuedConnection);
    connect(
        m_xrayProtocol1,
        &XRayProtocol::exposureStopped,
        this,
        [this]() { onXrayExposureStopped(0); },
        Qt::QueuedConnection);
    connect(
        m_xrayProtocol1,
        &XRayProtocol::exposureError,
        this,
        [this](XRayErrorCode error) { onXrayExposureError(error, 0); },
        Qt::QueuedConnection);
    connect(m_xrayProtocol1,
            &XRayProtocol::errorOccurred,
            this,
            [this](XRayErrorCode error, const QString& description) {
                onXrayErrorOccurred(error, description, 0);
            });
    connect(
        m_xrayProtocol1,
        &XRayProtocol::exposureCoolingRemaining,
        this,
        [this](int seconds) { onXrayCoolingRemaining(seconds, 0); },
        Qt::QueuedConnection);
    connect(
        m_xrayProtocol1,
        &XRayProtocol::temperatureUpdated,
        this,
        [this](float deviceTemp, float mcuTemp) {
            onXrayTemperatureUpdated(deviceTemp, mcuTemp, 0);
        },
        Qt::QueuedConnection);

    connect(
        m_xrayProtocol2,
        &XRayProtocol::deviceConnected,
        this,
        [this]() { onXrayDeviceConnected(1); },
        Qt::QueuedConnection);
    connect(
        m_xrayProtocol2,
        &XRayProtocol::deviceDisconnected,
        this,
        [this]() { onXrayDeviceDisconnected(1); },
        Qt::QueuedConnection);
    connect(m_xrayProtocol2,
            &XRayProtocol::deviceStatusChanged,
            this,
            [this](XRayDeviceStatus status) { onXrayStatusChanged(status, 1); });
    connect(
        m_xrayProtocol2,
        &XRayProtocol::exposureStarted,
        this,
        [this]() { onXrayExposureStarted(1); },
        Qt::QueuedConnection);
    connect(m_xrayProtocol2, &XRayProtocol::exposureStopped, this, [this]() {
        onXrayExposureStopped(1);
    });
    connect(
        m_xrayProtocol2,
        &XRayProtocol::exposureError,
        this,
        [this](XRayErrorCode error) { onXrayExposureError(error, 1); },
        Qt::QueuedConnection);
    connect(
        m_xrayProtocol2,
        &XRayProtocol::errorOccurred,
        this,
        [this](XRayErrorCode error, const QString& description) {
            onXrayErrorOccurred(error, description, 1);
        },
        Qt::QueuedConnection);
    connect(
        m_xrayProtocol2,
        &XRayProtocol::exposureCoolingRemaining,
        this,
        [this](int seconds) { onXrayCoolingRemaining(seconds, 1); },
        Qt::QueuedConnection);

    connect(
        m_xrayProtocol2,
        &XRayProtocol::temperatureUpdated,
        this,
        [this](float deviceTemp, float mcuTemp) {
            onXrayTemperatureUpdated(deviceTemp, mcuTemp, 1);
        },
        Qt::QueuedConnection);

}

bool XProtocolManager::setupWorkMode(bool calibrationBefore, int xRayIndex)
{
    m_xRayIndex = xRayIndex;

    if (m_isExposing.loadAcquire() != 0) {
        qWarning() << "SetupWorkMode repeat";
        return false;
    }

    m_isExposing = 1;
    m_cancelRequested = 0;

    QtConcurrent::run(m_threadPool, [this, calibrationBefore]() {
        try {
            if (!checkConnected()) {
                emit errorOccurred("设备未连接");
                return;
            }

            m_imageBuffer.clear();
            m_exposureWait = 0;
            bool success = startExposure();

            if (success) {
                emit info("工作模式设置完成");
            } else {
                emit errorOccurred("工作模式设置失败");
            }

        } catch (const std::exception& ex) {
            emit errorOccurred(QString("设置错误: %1").arg(ex.what()));
        }
    });

    return true;
}

void XProtocolManager::stopWorkMode()
{
    m_cancelRequested = 1;

    // 停止传感器工作模式
    if (m_sensor1 && m_sensor1->isConnected()) {
        m_sensor1->stopWorkMode();
    }

#ifndef DEBUG_SINGLE_DEVICE
    if (m_sensor2 && m_sensor2->isConnected()) {
        m_sensor2->stopWorkMode();
    }
#endif

    // 停止X光工作模式
    if (m_xrayProtocol1 && m_xrayProtocol1->isConnected()) {
        m_xrayProtocol1->stopExposure();
    }

#ifndef DEBUG_SINGLE_DEVICE
    if (m_xrayProtocol2 && m_xrayProtocol2->isConnected()) {
        m_xrayProtocol2->stopExposure();
    }
#endif

    // 停止定时器
    m_exposureTimer->stop();
    m_setupTimer->stop();

    // 重置状态
    m_isExposing = 0;
    m_isSettingUp = 0;
    m_imageBuffer.clear();

}

bool XProtocolManager::checkConnected()
{
    DeviceStatus status;
    int sensorCount = 0, xrayCount = 0;

    // 检查传感器1（调试模式和发布模式都需要）
    if (m_sensor1 && m_sensor1->isConnected()) {
        status.sensor1Connected = true;
        sensorCount++;
    }

#ifndef DEBUG_SINGLE_DEVICE
    // 调试模式下不检查第二个传感器
    if (m_sensor2 && m_sensor2->isConnected()) {
        status.sensor2Connected = true;
        sensorCount++;
    }
#endif

    // 检查X光1（调试模式和发布模式都需要）
    if (m_xrayProtocol1 && m_xrayProtocol1->isConnected()) {
        status.xray1Connected = true;
        xrayCount++;
    }

#ifndef DEBUG_SINGLE_DEVICE
    // 调试模式下不检查第二个X光机
    if (m_xrayProtocol2 && m_xrayProtocol2->isConnected()) {
        status.xray2Connected = true;
        xrayCount++;
    }
#endif

#ifdef DEBUG_SINGLE_DEVICE
    // 调试模式：只需要一个传感器和一个X光机
    bool result = (sensorCount >= 1 && xrayCount >= 1);
#else
    // 发布模式：需要两个传感器和两个X光机
    bool result = (sensorCount == 2 && xrayCount == 2);
#endif

    return result;
}

bool XProtocolManager::checkSensorCalibrateFile()
{
    int sensorCount = checkSensorCalibrateFileCount();

    if (sensorCount != 2) {
        emit warning("传感器未标定，请先进行标定");
        return false;
    }

    return true;
}

int XProtocolManager::checkSensorCalibrateFileCount()
{
    int sensorCount = 0;

    // 检查传感器1的标定文件
    if (m_sensor1 && m_sensor1->isConnected()) {
        // 这里需要调用你的算法模块检查标定文件
        // 假设有一个checkCalibrateFile方法
        // bool res = XPectAlgorithmic::CalibrateFileCheck(m_sensor1->deviceInfo());
        // 暂时先返回true，实际使用时需要实现
        bool res = true; // TODO: 实现标定文件检查
        if (res) {
            sensorCount++;
            qDebug() << "Sensor1 calibration file OK";
        }
    }

    // 检查传感器2的标定文件
    if (m_sensor2 && m_sensor2->isConnected()) {
        // bool res = XPectAlgorithmic::CalibrateFileCheck(m_sensor2->deviceInfo());
        bool res = true; // TODO: 实现标定文件检查
        if (res) {
            sensorCount++;
            qDebug() << "Sensor2 calibration file OK";
        }
    }

    return sensorCount;
}

bool XProtocolManager::startExposure()
{
    bool success1 = false;
    bool success2 = true;

    if (m_sensor1) {
        QMetaObject::invokeMethod(
            m_sensor1,
            [this, &success1]() {
                qDebug() << "[m_sensor1] Current thread:" << QThread::currentThread();

                success1 = m_sensor1->setupWorkMode(true);
            },
            Qt::BlockingQueuedConnection);
    }

#ifndef DEBUG_SINGLE_DEVICE
    if (m_sensor2) {
        QMetaObject::invokeMethod(
            m_sensor2,
            [this, &success2]() {
                qDebug() << "[m_sensor2] Current thread:" << QThread::currentThread();
                success2 = m_sensor2->setupWorkMode(true);
            },
            Qt::BlockingQueuedConnection);
    }
#endif

    if (success1 && success2) {
        return true;
    } else {
        qWarning() << "工作模式设置失败 - sensor1:" << success1 << "sensor2:" << success2;
        return false;
    }
}

bool XProtocolManager::executeCalibrationBefore()
{
    bool success1 = false;
    bool success2 = true;

    try {
        // 传感器1
        if (m_sensor1 && m_sensor1->isConnected()) {
            m_sensor1->setCalibrationBefore();
            success1 = true;
        }

        // 短暂延迟
        QThread::msleep(100);

        // 传感器2

#ifndef DEBUG_SINGLE_DEVICE
        if (m_sensor2 && m_sensor2->isConnected()) {
            m_sensor2->setCalibrationBefore();
            success2 = true;
        }
#endif

    } catch (...) {
        qCritical() << "校准前设置失败";
    }

    return success1 && success2;
}

void XProtocolManager::onCheckComPorts()
{
    // 限制检查频率
    static QElapsedTimer timer;
    if (timer.isValid() && timer.elapsed() < 2000) {
        return;
    }
    timer.restart();

    // 获取可用串口
    QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();

    // 快速处理
    for (auto& setting : m_portSettings) {
        QString portName = setting.portName;

        if (portName.isEmpty())
            continue;

        bool portExists = false;
        for (const auto& port : availablePorts) {
            if (port.portName() == portName) {
                portExists = true;
                break;
            }
        }

        if (portExists && !setting.connected) {
            // 快速尝试连接
            if (setting.deviceKey == SENSOR1_KEY || setting.deviceKey == SENSOR2_KEY) {
                connectSensor(setting.deviceKey);
            } else if (setting.deviceKey == XRAY1_KEY || setting.deviceKey == XRAY2_KEY) {
                connectXray(setting.deviceKey);
            }
        }
    }

    updateConnectionStatus();
}

bool XProtocolManager::connectSensor(const QString& sensorKey)
{
    // 查找端口配置
    QString portName;
    for (const auto& setting : m_portSettings) {
        if (setting.deviceKey == sensorKey) {
            portName = setting.portName;
            break;
        }
    }

    if (portName.isEmpty()) {
        qWarning() << "传感器" << sensorKey << "没有配置端口";
        return false;
    }

    // 获取传感器实例
    XSensorProtocol* sensor = (sensorKey == SENSOR1_KEY) ? m_sensor1 : m_sensor2;
    if (!sensor) {
        qWarning() << "传感器实例未初始化:" << sensorKey;
        return false;
    }

    qDebug() << "连接传感器:" << sensorKey << "端口:" << portName;

    // 在传感器线程中执行连接
    bool success = false;
    QMetaObject::invokeMethod(
        sensor,
        [sensor, portName, &success]() {
            qDebug() << "在线程" << QThread::currentThreadId() << "中连接传感器";
            success = sensor->initSerialPort(portName);
        },
        Qt::BlockingQueuedConnection);

    if (success) {
        // 更新状态
        for (auto& setting : m_portSettings) {
            if (setting.deviceKey == sensorKey) {
                setting.connected = true;
                break;
            }
        }
        QMetaObject::invokeMethod(
            this,
            [this, sensorKey]() { emit info(QString("传感器 %1 连接成功").arg(sensorKey)); },
            Qt::QueuedConnection);
    } else {
        qWarning() << "传感器" << sensorKey << "连接失败";
    }

    return success;
}

bool XProtocolManager::connectXray(const QString& xrayKey)
{
    // 查找端口设置
    QString portName;
    for (const auto& setting : m_portSettings) {
        if (setting.deviceKey == xrayKey) {
            portName = setting.portName;
            break;
        }
    }

    if (portName.isEmpty()) {
        qWarning() << "X光机" << xrayKey << "没有配置端口";
        return true;
    }

    XRayProtocol* xrayProtocol = (xrayKey == XRAY1_KEY) ? m_xrayProtocol1 : m_xrayProtocol2;

    if (!xrayProtocol) {
        qWarning() << "X光机" << xrayKey << "协议实例未初始化";
        return false;
    }

    // 在X光机线程中执行连接
    bool success = false;
    QMetaObject::invokeMethod(
        xrayProtocol,
        [xrayProtocol, portName, &success]() {
            qDebug() << "在线程" << QThread::currentThreadId() << "中连接X光机";
            success = xrayProtocol->initSerialPort(portName);
        },
        Qt::BlockingQueuedConnection);

    if (success) {
        // 更新状态
        for (auto& setting : m_portSettings) {
            if (setting.deviceKey == xrayKey) {
                setting.connected = true;
                break;
            }
        }
        QMetaObject::invokeMethod(
            this,
            [this, xrayKey]() { emit info(QString("X光机 %1 已连接").arg(xrayKey)); },
            Qt::QueuedConnection);
    } else {
        qWarning() << "X光机" << xrayKey << "连接失败";
    }

    return success;
}

void XProtocolManager::onExposureF5Ready(const QString& portName)
{
    static int exposureWait = 0;
    exposureWait++;

#ifdef DEBUG_SINGLE_DEVICE
    if (exposureWait < 1) {
        return;
    }
#else
    if (exposureWait < 2) {
        return;
    }
#endif

    exposureWait = 0;
    emit exposureProcess(ExposureState::Exposing);

#ifdef DEBUG_SINGLE_DEVICE
    if (m_sensor1) {
        QMetaObject::invokeMethod(
            m_sensor1, [this]() { m_sensor1->enableExposure(true, false); }, Qt::QueuedConnection);
    }

    if (m_xrayProtocol1 && m_xrayProtocol1->isConnected()) {
        QMetaObject::invokeMethod(
            m_xrayProtocol1,
            [this]() {
                ExposureParams xrayParams;
                xrayParams.kv_val = m_exposureParams.kv_val;
                xrayParams.ma_val = m_exposureParams.ma_val;
                xrayParams.exp_time_ms = m_exposureParams.exp_time_ms;

                m_xrayProtocol1->setExposureParams(xrayParams);
                m_xrayProtocol1->startExposure();
            },
            Qt::QueuedConnection);
    }

#else
    // 双设备模式的修改类似...
    {
        bool isMain = false;
        bool isX = false;
        for (auto& setting : m_portSettings) {
            QString name = setting.portName;
            if (portName == name) {
                isX == setting.orientation == 0;
                isMain = setting.description == "true" ? true : false;
                break;
            }
        }
        bool isXmain = isMain && isX;

        if (m_sensor1) {
            QMetaObject::invokeMethod(
                m_sensor1,
                [this, isXmain]() { m_sensor1->enableExposure(isXmain, m_xRayIndex != 0); },
                Qt::QueuedConnection);
        }

        QThread::msleep(10);

        if (m_sensor2) {
            QMetaObject::invokeMethod(
                m_sensor2,
                [this, isXmain]() { m_sensor2->enableExposure(!isXmain, m_xRayIndex != 0); },
                Qt::QueuedConnection);
        }

        QThread::msleep(500); // 新增延迟

        if (m_xRayIndex != 2 && m_xrayProtocol1) {
            QMetaObject::invokeMethod(
                m_xrayProtocol1,
                [this]() {
                    ExposureParams xrayParams;
                    xrayParams.kv_val = m_exposureParams.kv_val;
                    xrayParams.ma_val = m_exposureParams.ma_val;
                    xrayParams.exp_time_ms = m_exposureParams.exp_time_ms;

                    m_xrayProtocol1->setExposureParams(xrayParams);
                    m_xrayProtocol1->startExposure();
                },
                Qt::QueuedConnection);
        }
        if (m_xRayIndex != 1 && m_xrayProtocol2) {
            QMetaObject::invokeMethod(
                m_xrayProtocol2,
                [this]() {
                    ExposureParams xrayParams;
                    xrayParams.kv_val = m_exposureParams.kv_val;
                    xrayParams.ma_val = m_exposureParams.ma_val;
                    xrayParams.exp_time_ms = m_exposureParams.exp_time_ms;

                    m_xrayProtocol2->setExposureParams(xrayParams);
                    m_xrayProtocol2->startExposure();
                },
                Qt::QueuedConnection);
        }
    }

#endif
}

void XProtocolManager::onExposureF6Ready(const QString& portName)
{
    QString normalizedPortName = portName;
    if (!normalizedPortName.startsWith("/dev/")) {
        normalizedPortName = "/dev/" + normalizedPortName;
    }

    // 在线程池中执行图像采集
    QtConcurrent::run(m_threadPool, [this, normalizedPortName]() {
        // 查找对应的设备
        QString deviceKey;
        for (const auto& setting : m_portSettings) {
            if (setting.portName == normalizedPortName) {
                deviceKey = setting.deviceKey;
                break;
            }
        }

        if (deviceKey.isEmpty()) {
            qWarning() << "Unknown port for image acquisition:" << normalizedPortName;
            return;
        }

        // 在主线程更新状态
        QMetaObject::invokeMethod(this,
                                  [this]() { emit exposureProcess(ExposureState::Acquiring); });

        QCoreApplication::processEvents(QEventLoop::AllEvents);

        // 根据设备键调用相应的传感器（在线程中）
        if (deviceKey == SENSOR1_KEY) {
            runSensor1Operation([](XSensorProtocol* sensor) { sensor->acquireImage(); });
        } else if (deviceKey == SENSOR2_KEY) {
            runSensor2Operation([](XSensorProtocol* sensor) { sensor->acquireImage(); });
        }
    });
}

// 修改现有的运行操作方法，确保在正确的线程执行
void XProtocolManager::runSensor1Operation(std::function<void(XSensorProtocol*)> operation)
{
    if (m_sensor1 && m_sensor1->isConnected()) {
        // 确保在传感器1的线程中执行
        QMetaObject::invokeMethod(
            m_sensor1, [this, operation]() { operation(m_sensor1); }, Qt::QueuedConnection);
    }
}

void XProtocolManager::runSensor2Operation(std::function<void(XSensorProtocol*)> operation)
{
#ifndef DEBUG_SINGLE_DEVICE
    if (m_sensor2 && m_sensor2->isConnected()) {
        // 确保在传感器2的线程中执行
        QMetaObject::invokeMethod(
            m_sensor2, [this, operation]() { operation(m_sensor2); }, Qt::QueuedConnection);
    }
#endif
}

// 类似地修改X光操作方法
void XProtocolManager::runXray1Operation(std::function<void(XRayProtocol*)> operation)
{
    if (m_xrayProtocol1 && m_xrayProtocol1->isConnected()) {
        // 确保在X光1的线程中执行
        QMetaObject::invokeMethod(
            m_xrayProtocol1,
            [this, operation]() { operation(m_xrayProtocol1); },
            Qt::QueuedConnection);
    }
}

void XProtocolManager::runXray2Operation(std::function<void(XRayProtocol*)> operation)
{
#ifndef DEBUG_SINGLE_DEVICE
    if (m_xrayProtocol2 && m_xrayProtocol2->isConnected()) {
        // 确保在X光2的线程中执行
        QMetaObject::invokeMethod(
            m_xrayProtocol2,
            [this, operation]() { operation(m_xrayProtocol2); },
            Qt::QueuedConnection);
    }
#endif
}

void XProtocolManager::updateConnectionStatus()
{
    DeviceStatus newStatus;

    // 更新传感器状态
    if (m_sensor1) {
        newStatus.sensor1Connected = m_sensor1->isConnected();
    }

#ifndef DEBUG_SINGLE_DEVICE
    if (m_sensor2) {
        newStatus.sensor2Connected = m_sensor2->isConnected();
    }
#endif

    // 更新X光状态
    if (m_xrayProtocol1) {
        newStatus.xray1Connected = m_xrayProtocol1->isConnected();
    }

#ifndef DEBUG_SINGLE_DEVICE
    if (m_xrayProtocol2) {
        newStatus.xray2Connected = m_xrayProtocol2->isConnected();
    }
#endif
    // 更新状态
    m_deviceStatus = newStatus;
}

void XProtocolManager::onExposureTimeout()
{
    stopExposure();
}

void XProtocolManager::onSetupTimeout()
{
    stopWorkMode();
}

void XProtocolManager::onSensorImageAvailable(HWImageData& image, const QString& portName)
{
    QMutexLocker locker(&m_imageMutex);

    // 查找对应的传感器键
    QString sensorKey;
    for (const auto& setting : m_portSettings) {
        if (setting.portName.contains(portName)
            && (setting.deviceKey == SENSOR1_KEY || setting.deviceKey == SENSOR2_KEY)) {
            image.orientation = setting.orientation;
            sensorKey = setting.deviceKey;
            break;
        }
    }

    if (sensorKey.isEmpty()) {
        qWarning() << "Unknown sensor port:" << portName;
        return;
    }

    // 存储图像
    m_imageBuffer[sensorKey] = image;

    // 检查是否收到所有图像
    checkAllImagesReceived();

}

void XProtocolManager::checkAllImagesReceived()
{
#ifdef DEBUG_SINGLE_DEVICE
    // 调试模式：只需要一个图像
    int requiredImages = 1;
#else
    // 发布模式：需要两个图像
    int requiredImages = 2;
#endif

    if (m_imageBuffer.size() >= requiredImages) {
        // 收到所有图像，准备发送
        QList<HWImageData> images;

        // 按传感器顺序添加图像
        if (m_imageBuffer.contains(SENSOR1_KEY)) {
            images.append(m_imageBuffer[SENSOR1_KEY]);
        }

        if (m_imageBuffer.contains(SENSOR2_KEY)) {
            images.append(m_imageBuffer[SENSOR2_KEY]);
        }

        if (images.size() == requiredImages) {
            emit imagesReady(images);

            // 清空缓存
            m_imageBuffer.clear();

            // 在这里重置曝光状态
            m_isExposing = 0;
            m_cancelRequested = 0;
        }
    }
}

bool XProtocolManager::isExposing() const
{
    return m_isExposing.loadAcquire() != 0;
}

void XProtocolManager::stopExposure()
{
    m_cancelRequested = 1;
    m_isExposing = 0;

    if (m_isSettingUp.loadAcquire()) {
        m_isSettingUp = 0;
        if (m_sensor1) {
            QMetaObject::invokeMethod(m_sensor1, "stopWorkMode", Qt::BlockingQueuedConnection);
        }
        if (m_sensor2) {
            QMetaObject::invokeMethod(m_sensor2, "stopWorkMode", Qt::BlockingQueuedConnection);
        }
    }

    QtConcurrent::run(m_threadPool, [this]() {
        runSensor1Operation([](XSensorProtocol* sensor) { sensor->stopExposure(); });

#ifndef DEBUG_SINGLE_DEVICE
        runSensor2Operation([](XSensorProtocol* sensor) { sensor->stopExposure(); });
#endif

        runXray1Operation([](XRayProtocol* xray) { xray->stopExposure(); });

#ifndef DEBUG_SINGLE_DEVICE
        runXray2Operation([](XRayProtocol* xray) { xray->stopExposure(); });
#endif

        m_imageBuffer.clear();
    });
}

void XProtocolManager::cancelAllOperations()
{
    m_cancelRequested = 1;

    // 取消异步任务
    if (m_setupWatcher->isRunning()) {
        m_setupWatcher->cancel();
    }

    if (m_exposureWatcher->isRunning()) {
        m_exposureWatcher->cancel();
    }

}

void XProtocolManager::onXrayDeviceConnected(int index)
{
    QString xrayKey = (index == 0) ? XRAY1_KEY : XRAY2_KEY;
    emit info(QString("X光机 %1 已连接").arg(xrayKey));
}

void XProtocolManager::onXrayDeviceDisconnected(int index)
{
    QString xrayKey = (index == 0) ? XRAY1_KEY : XRAY2_KEY;
    emit warning(QString("X光机 %1 已断开").arg(xrayKey));
}

void XProtocolManager::onXrayStatusChanged(XRayDeviceStatus status, int index)
{
    QString statusStr;
    switch (status) {
    case DEVICE_IDLE:
        statusStr = "空闲";
        break;
    case DEVICE_READY:
        statusStr = "就绪";
        break;
    case DEVICE_EXPOSING:
        statusStr = "曝光中";
        break;
    case DEVICE_COOLING:
        statusStr = "冷却中";
        break;
    case DEVICE_ERROR:
        statusStr = "错误";
        break;
    default:
        statusStr = "未知";
        break;
    }

    QString xrayKey = (index == 0) ? XRAY1_KEY : XRAY2_KEY;
    emit info(QString("X光机 %1 状态: %2").arg(xrayKey).arg(statusStr));
}

void XProtocolManager::onXrayExposureStarted(int index)
{
    QString xrayKey = (index == 0) ? XRAY1_KEY : XRAY2_KEY;
    emit info(QString("X光机 %1 开始曝光").arg(xrayKey));
}

void XProtocolManager::onXrayExposureStopped(int index)
{
    QString xrayKey = (index == 0) ? XRAY1_KEY : XRAY2_KEY;
    emit info(QString("X光机 %1 停止曝光").arg(xrayKey));
}

void XProtocolManager::onXrayExposureError(XRayErrorCode error, int index)
{
    XRayProtocol* xray = nullptr;
    if (index == 0) {
        xray = m_xrayProtocol1;
    } else {
        xray = m_xrayProtocol2;
    }

    QString errorStr = xray ? xray->getErrorString(error) : "未知错误";
    QString xrayKey = (index == 0) ? XRAY1_KEY : XRAY2_KEY;
    emit errorOccurred(QString("X光机 %1 曝光错误: %2").arg(xrayKey).arg(errorStr));
}

void XProtocolManager::onXrayErrorOccurred(XRayErrorCode error,
                                           const QString& description,
                                           int index)
{
    XRayProtocol* xray = nullptr;
    if (index == 0) {
        xray = m_xrayProtocol1;
    } else {
        xray = m_xrayProtocol2;
    }

    QString errorStr = xray ? xray->getErrorString(error) : "未知错误";
    QString xrayKey = (index == 0) ? XRAY1_KEY : XRAY2_KEY;
    emit errorOccurred(QString("X光机 %1 错误: %2 - %3").arg(xrayKey).arg(errorStr).arg(description));
}

void XProtocolManager::onXrayCoolingRemaining(int seconds, int index)
{
    QString xrayKey = (index == 0) ? XRAY1_KEY : XRAY2_KEY;
    if (seconds > 0) {
        emit warning(QString("X光机 %1 冷却剩余: %2秒").arg(xrayKey).arg(seconds));
    }
}

void XProtocolManager::onXrayTemperatureUpdated(float deviceTemp, float mcuTemp, int index)
{
    QString xrayKey = (index == 0) ? XRAY1_KEY : XRAY2_KEY;
    if (deviceTemp > 50.0f) {
        emit warning(QString("X光机 %1 温度过高: %2°C").arg(xrayKey).arg(deviceTemp));
    }
}

// 传感器信号处理

void XProtocolManager::onSensorDeviceDisconnected()
{
    emit warning("传感器已断开");
    updateConnectionStatus();
}

void XProtocolManager::onSensorStatusChanged(const QString& status)
{
    emit info(QString("传感器状态: %1").arg(status));
}
