// XRayProtocol.h
#ifndef XRAYPROTOCOL_H
#define XRAYPROTOCOL_H

#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include "ModbusCRC16.h"

// 错误码定义
enum XRayErrorCode {
    XRAY_SUCCESS = 0,
    XRAY_ERROR_OVER_KV = 1,
    XRAY_ERROR_LOW_MA = 2,
    XRAY_ERROR_OVER_MA = 3,
    XRAY_ERROR_OVER_CURRENT = 5,
    XRAY_ERROR_LOW_FIL_I = 6,
    XRAY_ERROR_OVER_KV_S = 7,
    XRAY_ERROR_KEY_RELEASE = 8,
    XRAY_ERROR_KEY_NO_RELEASE = 9,
    XRAY_ERROR_LOW_BATTERY = 10,
    XRAY_ERROR_HARDWARE_CHECK = 11,
    XRAY_ERROR_COOLING = 12,
    XRAY_ERROR_EXPOSURE_TIMEOUT = 13
};

// 设备状态
enum XRayDeviceStatus {
    DEVICE_IDLE = 0,
    DEVICE_READY = 1,
    DEVICE_EXPOSING = 2,
    DEVICE_COOLING = 3,
    DEVICE_ERROR = 4
};

// 曝光参数结构体
struct ExposureParams
{
    float ma_min;        // mA最小值
    float ma_max;        // mA最大值
    float ma_val;        // mA设定值
    float kvs_min;       // KV设置最小值
    float kvs_max;       // KV设置最大值
    float kv_min;        // KV最小值
    float kv_max;        // KV最大值
    float kv_val;        // KV设定值
    quint32 exp_time_ms; // 曝光时间(毫秒)

    bool waitXray;

    ExposureParams()
    {
        ma_min = 0.01f;
        ma_max = 3.00f;
        ma_val = 0.0f;
        kvs_min = 0.0f;
        kvs_max = 2.60f;
        kv_min = 0.01f;
        kv_max = 2.50f;
        kv_val = 1.6666f; // 对应50KV
        exp_time_ms = 1000;
        waitXray = true;
    }
};

struct ExposureStatus
{
    quint16 exposureEnable; // 曝光使能
    quint16 exposureMode;   // 曝光模式
    quint8 lowBattery;      // 低电量
    quint8 hardwareCheck;   // 硬件检测
    quint16 delayShutdown;  // 延迟关机时间(ms)
    quint16 cooldownTime;   // 曝光冷却时间(s)
    quint8 exposureStep;    // 曝光处理步骤
};

// ADC采集值结构体
struct ADCValues
{
    quint16 charge_current; // 充电电流ADC值
    quint16 ovc2;           // OVC2 ADC值
    quint16 battery_div;    // 电池分压ADC值
    quint16 kv_ref;         // KV参考ADC值
    quint16 ma;             // mA ADC值
    quint16 fill;           // 填充ADC值
    quint16 kv;             // KV ADC值
    float mcu_temp;         // MCU温度(℃)
    float mcu_vref;         // MCU参考电压(V)
    float device_temp;      // 设备温度(℃)
};

class XRayProtocol : public QObject
{
    Q_OBJECT

public:
    explicit XRayProtocol(QObject *parent = nullptr);
    ~XRayProtocol();

    // 设备控制接口
    bool initSerialPort(const QString &portName, int baudRate = 921600);
    bool isConnected() const;
    QString portName() const;

    // 设备信息查询
    QString getVersion();
    QString getSerialNumber();
    ADCValues getADCValues();

    // 波形数据获取（只保留实际实现的方法）
    QByteArray getWaveformData(quint16 waveformType);
    QByteArray getKVWaveform();

    // 曝光控制
    bool setExposureParams(const ExposureParams &params);
    bool startExposure();
    bool stopExposure();
    bool checkExposureReady();
    bool resetDevice();

    // 错误处理
    bool clearErrorFlags();
    XRayErrorCode getLastError() const;
    QString getErrorString(XRayErrorCode error) const;

    // 设备状态
    XRayDeviceStatus getDeviceStatus() const;
    int getCoolingRemainingTime() const; // 返回冷却剩余时间(秒)

    // 内部方法（移到private？）
    QByteArray extractCompletePacket(QByteArray &buffer);

signals:
    // 设备状态信号
    void deviceConnected();
    void deviceDisconnected();
    void deviceStatusChanged(XRayDeviceStatus status);

    // 曝光相关信号
    void exposureStarted();
    void exposureStopped();
    void exposureProgress(int percent);
    void exposureCompleted();
    void exposureError(XRayErrorCode error);

    // 数据更新信号
    void adcValuesUpdated(const ADCValues &values);
    void temperatureUpdated(float deviceTemp, float mcuTemp);
    void batteryStatusUpdated(float voltage, bool isLow); // 可能未使用，但保留

    // 错误信号
    void errorOccurred(XRayErrorCode error, const QString &description);
    void warningMessage(const QString &message);

    void coolingTimeUpdated(int remainingSeconds);
    void exposureCoolingRemaining(int seconds);

private slots:
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void onExposureTimeout();
    void updateDeviceStatus(); // 移到private slots

    void closeSerialPort();

private:
    // 串口通信
    QSerialPort *m_serialPort;
    QByteArray m_receiveBuffer;
    QMutex m_serialMutex;

    // 命令定义
    static const QByteArray CMD_VERSION;
    static const QByteArray CMD_SERIAL_NUMBER;
    static const QByteArray CMD_ADC_VALUES;
    static const QByteArray CMD_ERROR_QUERY;
    static const QByteArray CMD_ERROR_CLEAR;

    // 曝光命令
    static const QByteArray CMD_EXPOSURE_START;
    static const QByteArray CMD_EXPOSURE_STOP;
    static const QByteArray CMD_EXPOSURE_QUERY;
    static const QByteArray CMD_EXPOSURE_QUERY_EMPTY;

    // 内部方法
    QByteArray buildCommand(quint16 cmdCode, quint8 operate, const QByteArray &data);
    QByteArray executeCommand(quint16 cmdCode,
                              const QByteArray &data = QByteArray(),
                              int responseTimeout = 1000);
    QByteArray getWaveformCommandTemplate(quint16 waveformType, const QByteArray &data);

    // 数据解析
    ADCValues parseADCValues(const QByteArray &data);
    QString parseVersion(const QByteArray &data);
    QString parseSerialNumber(const QByteArray &data);

    // CRC校验
    bool verifyCRC(const QByteArray &data);

    // 状态管理
    XRayDeviceStatus m_deviceStatus;
    XRayErrorCode m_lastError;
    ExposureParams m_currentParams;
    QTimer *m_exposureTimer;
    QTimer *m_statusTimer;
    int m_coolingRemaining;
    int m_expTimeMs;
    // 线程安全
    QMutex m_statusMutex;

    // 配置
    QString m_portName;

    bool m_over18SecSent;
    QDateTime m_exposureStartTime;
    QByteArray m_exposureStartData;
    QByteArray m_exposureStopData;

    // 私有辅助方法
    bool waitForResponse(int timeout);
    void handleExposureResponse(const QByteArray &response);
    void logCommunication(const QString &direction, const QByteArray &data);

    bool checkExposureStatus(ExposureStatus &status);
    bool clearFaults();
    bool enableExposure();
    bool sendStartExposureCommand(quint8 &exposureStep);
};
#endif // XRAYPROTOCOL_H
