// XSensorProtocol.h - 精简版本
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
    bool checkActive();
    void resetDeviceState();

    void rectifyDeviceParam();
    void cleanupAfterImageAcquisition(bool success);

    // 曝光相关方法
    bool enableExposure(bool isMainSensor, bool waitXray);
    void acquireImage();
    QByteArray retrieveImageByte(QString& errorCode, int& expectedBagNum, int packageIndex);
    QByteArray processImageData(const QByteArray& rawData, int cols, int rows);

    // 校准相关
    void setCalibrationBefore();

    // 工作模式控制
    bool stopWorkMode(); // 停止工作模式

    // 获取端口名
    QString portName() const;

    // 设备控制
    bool powerOn();
    bool powerOff();
    bool setupWorkMode(bool callbackF5F5 = true);

    // 曝光控制
    void stopExposure();

    // 设备信息
    DeviceInfo getDeviceInfo() const;
    QString getLastError() const;

    // 自动初始化
    bool autoInitialize(const QString& preferredPort = "", int timeoutMs = 5000);
    bool isInitialized() const;

    // 配置
    void setBaudRate(int baudRate);
    void setTimeouts(int readTimeout, int writeTimeout, int echoTimeout, int exposureTimeout);

    // 端口相关
    QStringList findAvailablePorts();
    QString detectDevicePort(int timeoutMs = 3000);

    QByteArray buildCommand(const QByteArray& cmdData,
                            char endChar1 = '\x0D',
                            char endChar2 = '\x0A');
    bool switchToLowPowerMode();

signals:
    void deviceConnected();
    void deviceDisconnected();
    void deviceReady();
    void deviceInitialized(bool success);
    void exposureStarted();
    void exposureCompleted();
    void imageReady(const QByteArray& imageData);
    void errorOccurred(const QString& errorMessage);
    void warningOccurred(const QString& warningMessage);
    void commandExecuted(quint32 commandCode);
    void statusChanged(const QString& status);
    void cmdExecuted(uint32_t cmd);

    void exposureF5Ready(QString serial);
    void exposureF6Ready(QString serial);

    // 添加缺失的信号
    void commandFeedback(uint32_t cmd);
    void comError(const QString& msgID);
    void imageAvailable(const QByteArray& image);
    void calibrationRequired();

protected slots:
    void onSerialReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);

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
        static QByteArray F5_VOLTAGE_CONFIG;
        static QByteArray F6_ENABLE_EXPOSE;
        static QByteArray F6_ENABLE_EXPOSE_02;
        static QByteArray F6_ENABLE_EXPOSE_CALI;
        static QByteArray F6_ENABLE_EXPOSE_1MIN;

        static QByteArray F8_REQUEST_IMAGE;
        static QByteArray F8_TO_LOWER_POWER;
    };

    // 内部方法（对应C# GetTranducertCOM）
    QSerialPort* internalGetTransducerCOM(const QString& comPort);
    bool echoSerialPort(QSerialPort* serialPort);
    void setupSerialPortCallbacks(QSerialPort* serialPort);
    void releaseCOM(QSerialPort* serialPort);

    bool echoDevice();
    bool sendCommand(const QByteArray& cmd);
    QByteArray readResponse(int timeout);
    bool validateCRC(const QByteArray& data);
    QByteArray calculateCRC(const QByteArray& data);
    bool validatePowerOffResponse(const QByteArray& response);
    QByteArray cropImage(
        const QByteArray& image, int cols, int rows, int top, int right, int bottom, int left);

    // 协议处理
    DeviceInfo parseDeviceInfo(const QByteArray& data);
    bool sendF5Config();
    bool sendF8ImageRequest();

    // 工具方法
    static QByteArray hexStringToBytes(const QString& hexStr);
    static QString bytesToHexString(const QByteArray& bytes);
    static quint16 bytesToUInt16(const QByteArray& bytes, bool littleEndian = true);
    static quint32 bytesToUInt32(const QByteArray& bytes, bool littleEndian = true);

private:
    QSerialPort* m_serial;
    QRecursiveMutex m_mutex;

    bool m_deviceInitialized;
    bool m_exposing;
    bool m_poweredOn;
    bool m_autoInitialized;

    DeviceInfo m_deviceInfo;
    QString m_lastError;

    bool m_initInProgress = false; // 互斥标志
    QMutex m_initMutex;

    // 配置参数
    int m_baudRate;
    int m_readTimeout = 1000;
    int m_openTimeout = 500;
    int m_retryTimes = 3;
    int m_echoTimeout = 500;
    int m_normalCmdTimeout = 2000;
    int m_acquireImageDelay = 0;
    bool m_rectifyF5Enabled = false;
    bool m_rectifyAECEnabled = false;
    bool m_imageLogEnabled = false;

    // 超时配置
    int m_writeTimeout;
    int m_exposureTimeout;

    bool m_cancelRequested;

    // 状态标志
    std::atomic<bool> m_isBusy;
};

#endif // XSENSORPROTOCOL_H
