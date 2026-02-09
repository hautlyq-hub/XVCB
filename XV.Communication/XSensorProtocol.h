// XSensorProtocol.h - 更新版本
#ifndef XSENSORPROTOCOL_H
#define XSENSORPROTOCOL_H

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QSerialPort>
#include <atomic>

#include "SensorCRC16.h"

class XSensorProtocol : public QObject
{
    Q_OBJECT

public:
    struct DeviceInfo
    {
        QString version;
        QString sn;
        QString powerVersion;
        QString powerSN;
    };

    explicit XSensorProtocol(QObject* parent = nullptr);
    ~XSensorProtocol();

    // 设备连接管理
    bool initSerialPort(const QString& portName);
    bool isConnected() const;
    void resetDeviceState();

    // 图像采集相关
    void acquireImage();
    QByteArray retrieveImageByte(QString& errorCode, int& expectedBagNum, int packageIndex);
    QByteArray processImageData(const QByteArray& rawData, int cols, int rows);
    void cleanupAfterImageAcquisition(bool success);

    // 曝光相关方法
    bool sendExposureF6(bool wait = false, bool isCalibration = false);
    void stopExposure();
    bool stopWorkMode();

    // 校准相关
    void setCalibrationBefore();

    // 获取端口名
    QString portName() const;

    // 设备控制
    bool getVersion();
    bool powerOn();
    bool powerOff();
    bool setupWorkMode(bool b = true);

    // 错误信息
    QString getLastError() const;

    QString getDeviceInfoVersion();
    QString getDeviceInfoSN();
    DeviceInfo getDeviceInfo();

signals:
    void statusChanged(const QString& status);
    void errorOccurred(const QString& errorMessage);
    void imageReady(const QByteArray& imageData);
    void exposureCompleted();
    void exposureF5Ready(const QString& serial);
    void exposureF6Ready(const QString& serial);

private slots:
    void closeSerialPort();

private:
    // 协议命令
    struct Commands
    {
        static QByteArray F4_POWER_ON_3V3;
        static QByteArray F4_POWER_ON;
        static QByteArray F4_POWER_OFF;
        static QByteArray FA_FIND_DEVICE;
        static QByteArray F5_CONFIG;
        static QByteArray F5_READ_CONFIG;
        static QByteArray F5_EXTERNAL_BIAL;
        static QByteArray F5_READ_EXTERNAL_BIAL;

        static QByteArray F5_VOLTAGE_CONFIG;
        static QByteArray F6_ENABLE_EXPOSE;
        static QByteArray F6_ENABLE_EXPOSE_02;
        static QByteArray F6_ENABLE_EXPOSE_CALI;
        static QByteArray F6_ENABLE_EXPOSE_1MIN;
        static QByteArray F8_REQUEST_IMAGE;
        static QByteArray F8_TO_LOWER_POWER;
    };

    // 串口通信
    bool echoSerialPort();
    bool sendCommand(const QByteArray& cmd);
    QByteArray readResponse(int timeout);
    QByteArray buildCommand(const QByteArray& cmdData,
                            char endChar1 = '\x0D',
                            char endChar2 = '\x0A');

    // 协议处理
    bool parseDeviceInfo(const QByteArray& data);
    bool sendF5Config();
    bool sendF8ImageRequest();

    // CRC校验
    bool validateCRC(const QByteArray& data);
    QByteArray calculateCRC(const QByteArray& data);

    // 工具方法
    static QString bytesToHexString(const QByteArray& bytes);

private:
    QSerialPort* m_serialPort;
    mutable QMutex m_mutex;

    DeviceInfo m_deviceInfo;
    QString m_lastError;

    // 状态标志
    bool m_deviceInitialized;
    bool m_exposing;
    bool m_poweredOn;
    bool m_autoInitialized;
    bool m_cancelRequested;
    std::atomic<bool> m_isBusy;

    // 配置参数
    int m_baudRate;
    int m_readTimeout;
    int m_writeTimeout;
    int m_echoTimeout;
    int m_exposureTimeout;
};

#endif // XSENSORPROTOCOL_H
