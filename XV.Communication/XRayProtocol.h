// XRayProtocol.h - 更新版本
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
        ma_val = 0.5f;
        kvs_min = 0.0f;
        kvs_max = 2.40f;
        kv_min = 0.01f;
        kv_max = 2.29f;
        kv_val = 1.6666f;
        exp_time_ms = 400;
        waitXray = true;
    }
};

enum class ExposureState {
    Idle = 0,   // 空闲
    SettingUp,  // 设置中
    Exposing,   // 曝光中
    Acquiring,  // 采集中
    Processing, // 处理中
    Completed,  // 完成
    Fault,
    Timeout
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

    // 设备连接管理
    bool initSerialPort(const QString &portName, int baudRate = 921600);
    bool isConnected() const;
    QString portName() const;

    // 设备信息查询
    QString getVersion();
    QString getSerialNumber();
    ADCValues getADCValues();

    // 波形数据获取
    QByteArray getWaveformData(quint16 waveformType);
    QByteArray getKVWaveform();

    // 曝光控制
    bool setExposureParams(const ExposureParams &params);
    bool startExposure();
    bool stopExposure();
    bool checkExposureReady();

    // 错误处理
    bool clearErrorFlags();
    XRayErrorCode getLastError() const;
    QString getErrorString(XRayErrorCode error) const;

    // 数据包处理
    QByteArray extractCompletePacket(QByteArray &buffer);

signals:
    // 设备状态信号
    void deviceConnected();
    void deviceDisconnected();
    void deviceStatusChanged(ExposureState status);

    // 曝光相关信号
    void exposureStarted();
    void exposureStopped();
    void exposureError(XRayErrorCode error);

    // 数据更新信号
    void adcValuesUpdated(const ADCValues &values);
    void temperatureUpdated(float deviceTemp, float mcuTemp);

    // 错误信号
    void errorOccurred(XRayErrorCode error, const QString &description);

private slots:
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void onExposureTimeout();
    void updateDeviceStatus();
    void closeSerialPort();

private:
    // 串口通信
    QSerialPort *m_serialPort;
    QMutex m_serialMutex;

    // 命令定义
    static const QByteArray CMD_VERSION;
    static const QByteArray CMD_SERIAL_NUMBER;
    static const QByteArray CMD_ADC_VALUES;
    static const QByteArray CMD_ERROR_QUERY;
    static const QByteArray CMD_ERROR_CLEAR;
    static const QByteArray CMD_EXPOSURE_QUERY;
    static const QByteArray CMD_EXPOSURE_START;
    static const QByteArray CMD_EXPOSURE_STOP;

    // 内部通信方法
    QByteArray buildCommand(quint16 cmdCode, quint8 operate, const QByteArray &data);
    bool sendCommand(quint16 cmdCode, const QByteArray &data = QByteArray());
    QByteArray readResponse(int timeout);

    // 数据解析
    ADCValues parseADCValues(const QByteArray &data);
    QString parseVersion(const QByteArray &data);
    QString parseSerialNumber(const QByteArray &data);
    ExposureParams parseExposureParams(const QByteArray &data);

    bool verifyCRC(const QByteArray &data);
    void handleExposureResponse(const QByteArray &response);

    // 状态管理
    XRayErrorCode m_lastError;
    ExposureParams m_currentParams;
    QTimer *m_exposureTimer;
    int m_coolingRemaining;
    int m_expTimeMs;
    int m_respTimeout;

    // 曝光控制
    bool m_over18SecSent;
    QDateTime m_exposureStartTime;
    QByteArray m_exposureStartData;
    QByteArray m_exposureStopData;

    // 辅助方法
    bool clearFaults();
};

#endif // XRAYPROTOCOL_H
