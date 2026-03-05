#ifndef XPROTOCOLMANAGER_H
#define XPROTOCOLMANAGER_H

#include <QAtomicInteger>
#include <QDomDocument>
#include <QFutureWatcher>
#include <QMutex>
#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include "XRayProtocol.h"
#include "XSensorProtocol.h"
#include "XV.Tool/XDConfigFileManager.h"
#include "XV.Tool/mvdatalocations.h"
#include "algorithms/XPectAlgorithmic.h"

struct HWImageData
{
    QByteArray imageData;  // 图像数据
    int width = 0;         // 宽度
    int height = 0;        // 高度
    int orientation = 0;   // 方向
    QString deviceVersion; // 设备版本
    QString deviceSN;      // 序列号
    QString filePath;      // 文件路径
    int bitDepth = 16;     // 位深度

    bool isValid() const { return !imageData.isEmpty() && width > 0 && height > 0; }
};

// 端口设置结构
struct PortSetting
{
    QString deviceKey;   // 设备键名，如 "sensor1", "xray1"
    QString portName;    // 串口名称，如 "COM3"
    QString description; // 设备描述
    int orientation;     // 方向/配置
    bool connected;      // 是否已连接
};

// 设备状态结构
struct DeviceStatus
{
    bool sensor1Connected = false;
    bool sensor2Connected = false;
    bool xray1Connected = false;
    bool xray2Connected = false;

    // 判断所有设备是否都已连接
    bool allDevicesConnected() const
    {
        return sensor1Connected && sensor2Connected && xray1Connected && xray2Connected;
    }

    // 转换为字符串表示
    QString toString() const
    {
        return QString("Sensor1: %1, Sensor2: %2, Xray1: %3, Xray2: %4")
            .arg(sensor1Connected ? "Connected" : "Disconnected")
            .arg(sensor2Connected ? "Connected" : "Disconnected")
            .arg(xray1Connected ? "Connected" : "Disconnected")
            .arg(xray2Connected ? "Connected" : "Disconnected");
    }
    bool operator==(const DeviceStatus& other) const
    {
        return sensor1Connected == other.sensor1Connected
               && sensor2Connected == other.sensor2Connected
               && xray1Connected == other.xray1Connected && xray2Connected == other.xray2Connected;
    }

    // 添加不相等运算符（可选）
    bool operator!=(const DeviceStatus& other) const { return !(*this == other); }
};

class XProtocolManager : public QObject
{
    Q_OBJECT

public:
    // 常量定义
    static const QString SENSOR1_KEY;
    static const QString SENSOR2_KEY;
    static const QString XRAY1_KEY;
    static const QString XRAY2_KEY;

    QThread* m_sensor1Thread;
    QThread* m_sensor2Thread;
    QThread* m_xray1Thread;
    QThread* m_xray2Thread;

    static XProtocolManager* m_instance;
    static std::mutex m_mutex;
    static XProtocolManager* getInstance();

    template<typename Func>
    void invokeInThread(QThread* targetThread, QObject* targetObj, Func func)
    {
        if (targetThread && targetObj) {
            if (QThread::currentThread() != targetThread) {
                QMetaObject::invokeMethod(targetObj, func, Qt::BlockingQueuedConnection);
            } else {
                func();
            }
        }
    }

    explicit XProtocolManager(QObject* parent = nullptr);
    ~XProtocolManager();

    // 硬件控制
    bool initSerialPort();
    void unInitSerialPort();

    // 工作模式设置
    bool isHardwareReady() const;
    bool setupWorkMode(bool calibrationBefore = true, int xRayIndex = 0);
    void stopWorkMode();

    // 曝光控制
    void startExposure(const ExposureParams& params);
    void stopExposure();

    // 设备连接管理
    bool connectSensor(const QString& sensorKey);
    bool connectXray(const QString& xrayKey);
    void disconnectSensor(const QString& sensorKey);
    void disconnectXray(const QString& xrayKey);

    // 状态查询
    bool isConnected() const;
    bool isExposing() const;
    bool isInitialized() const;
    DeviceStatus getDeviceStatus();
    QString getStatusString() const;

    // 参数设置
    void setExposureParams(const ExposureParams& params);
    void setXRayParams(float kv, int timeMs, float ma);

    // 取消操作
    void cancelAllOperations();

    // 传感器版本信息
    QString getSensorVersion(const QString& sensorKey);
    QString getSensorSerialNumber(const QString& sensorKey);
    XSensorProtocol::DeviceInfo getSensorInfo(const QString& sensorKey);

    // X光机版本信息
    QString getXRayVersion(int xrayIndex);
    QString getXRaySerialNumber(int xrayIndex);

    // 错误信息
    QString getErrorInfo(const QString& errorCode);

    bool checkConnected();
    bool executeCalibrationBefore();
    bool startExposure();
    bool checkSensorCalibrateFile();
    int checkSensorCalibrateFileCount();

    void updateSerialPortStatus();

signals:

    // 曝光控制信号
    void exposureError(const QString& error);
    void exposureProcess(ExposureState state);
    // 图像相关信号
    void imagesReady(const QList<HWImageData>& images);

    // 信息通知
    void info(const QString& message);
    void warning(const QString& message);
    void errorOccurred(const QString& error);
    void notification(const QString& message);

private slots:
    // 定时器槽函数
    void onSerialPortsCheck();
    void onExposureF5Ready(const QString& portName);
    void onExposureF6Ready(const QString& portName);

    // 传感器信号处理
    void onSensorDeviceDisconnected();
    void onSensorStatusChanged(const QString& status);

    // X光信号处理（带索引参数）
    void onXrayDeviceConnected(int index);
    void onXrayDeviceDisconnected(int index);
    void onXrayExposureStarted(int index);
    void onXrayExposureStopped(int index);
    void onXrayErrorOccurred(XRayErrorCode error, const QString& description, int index);
    void onXrayCoolingRemaining(int seconds, int index);
    void onXrayTemperatureUpdated(float deviceTemp, float mcuTemp, int index);

    // 图像处理
    void onSensorImageAvailable(HWImageData& image, const QString& portName);

private:
    // 初始化函数
    void initializeSensors();
    void initializeXray();
    void loadConfig();

    // 图像处理
    void checkAllImagesReceived();

    void runSensor1Operation(std::function<void(XSensorProtocol*)> operation);
    void runSensor2Operation(std::function<void(XSensorProtocol*)> operation);
    void runXray1Operation(std::function<void(XRayProtocol*)> operation);
    void runXray2Operation(std::function<void(XRayProtocol*)> operation);

    // 辅助函数
    XRayProtocol* getCurrentXRay();

    // 设备实例
    XSensorProtocol* m_sensor1 = nullptr;
    XSensorProtocol* m_sensor2 = nullptr;
    XRayProtocol* m_xrayProtocol1 = nullptr;
    XRayProtocol* m_xrayProtocol2 = nullptr;

    // 线程管理
    QThreadPool* m_threadPool = nullptr;
    QFutureWatcher<bool>* m_setupWatcher = nullptr;
    QFutureWatcher<void>* m_exposureWatcher = nullptr;

    // 定时器
    QTimer* m_serialPortTimer = nullptr;

    // 状态变量
    DeviceStatus m_deviceStatus;
    QAtomicInteger<int> m_isExposing = 0;
    QAtomicInteger<int> m_isSettingUp = 0;
    QAtomicInteger<int> m_cancelRequested = 0;

    // 参数和配置
    ExposureParams m_exposureParams;
    QList<PortSetting> m_portSettings;
    QMap<QString, HWImageData> m_imageBuffer;

    // X光机选择索引（0: xray1, 1: xray2）
    int m_xRayIndex = 0;
    int m_exposureWait = 0;

    // 超时配置
    int m_exposureTimeoutMs = 30000; // 曝光超时（毫秒）
    int m_setupTimeoutMs = 10000;    // 设置超时（毫秒）
    int m_nextSecondsInterval = 5;   // 下一个秒间隔

    // 互斥锁
    QMutex m_imageMutex;
    QMutex m_statusMutex;
    QMutex m_configMutex;
};

#endif // XPROTOCOLMANAGER_H
