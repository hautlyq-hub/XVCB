#ifndef SERIALCOMMUNICATION_H
#define SERIALCOMMUNICATION_H

#include <QMutex>
#include <QObject>
#include <QVector>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtEndian>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

// 命令定义
enum Command {
    CMD_MCU_SN = 0x000A,
    CMD_SOFTWARE_VERSION = 0x000C,
    CMD_ADC = 0x000D,
    CMD_KV_WAVEFORM = 0x000F,
    CMD_MA_WAVEFORM = 0x0010,
    CMD_FI_WAVEFORM = 0x0011,
    CMD_OVER_C_WAVEFORM = 0x0012,
    CMD_ERROR_INFO = 0x0018,
    CMD_EXPOSURE_CMD2 = 0x001B,
    CMD_EXPOSURE_PARAMS = 0x001C,
    CMD_EXPOSURE_ERROR_FLAG = 0x001D
};

enum Operate {
    OPERATE_READ = 0x00,
    OPERATE_WRITE = 0x01,
    OPERATE_SAVE = 0x02,
    OPERATE_REPLY = 0x03
};

// 数据解析接口
struct MCUSNData {
    uint32_t uid2;
    uint32_t uid1;
    uint32_t uid0;
};

struct ADCData {
    uint16_t chargeCurrent;
    uint16_t ovc2;
    uint16_t batteryVoltage;
    uint16_t kvRef;
    uint16_t ma;
    uint16_t fill;
    uint16_t kv;
    float mcuTemperature;
    float mcuVref;
    float deviceTemperature;
};

struct WaveformData {
    uint16_t packetSeq;
    uint16_t totalBytes;
    QVector<uint16_t> adcValues;
};

struct ErrorInfoData {
    uint32_t fillData0;
    uint32_t errorCode;
    uint32_t exposureCount;
    uint32_t totalExposureCount;
    uint32_t totalExposureTime;
    uint32_t noneErrorCount;
    uint32_t overKVErrorCount;
    uint32_t lowMAErrorCount1;
    uint32_t overMAErrorCount;
    uint32_t lowMAErrorCount2;
    uint32_t overCurrentErrorCount;
    uint32_t lowFILIErrorCount;
    uint32_t overKVSErrorCount;
    uint32_t keyReleaseErrorCount;
    uint32_t keyNotReleaseErrorCount;
    float lastErrorBatteryVoltage;
    float lastErrorDeviceTemperature;
    uint16_t fillData1;
};

struct ExposureStatusData {
    bool enable;
    bool mode;
    bool lowBattery;
    uint8_t hardwareStatus;
    uint16_t shutdownDelay;
    uint16_t exposureCooldownTime;
    uint8_t exposureStep;
};

class SerialCommunication : public QObject
{
    Q_OBJECT

public:
    // 单例获取接口
    static SerialCommunication* instance();

    // 串口操作
    bool openSerialPort(const QString& portName);
    void closeSerialPort();
    bool isOpen() const;

    // 协议命令发送接口
    bool sendCommand(uint16_t cmd, uint8_t operate, const QByteArray& data = QByteArray());

    // 具体命令封装
    bool readMCUSN();
    bool readSoftwareVersion();
    bool readADC();
    bool readKVWaveform(uint16_t packetSeq, uint16_t packetLen = 64);
    bool readMAWaveform(uint16_t packetSeq, uint16_t packetLen = 64);
    bool readFIWaveform(uint16_t packetSeq, uint16_t packetLen = 64);
    bool readOverCWaveform(uint16_t packetSeq, uint16_t packetLen = 64);
    bool readErrorInfo();
    bool sendExposureCommand2(bool enable, bool mode);
    bool setExposureParams(const QByteArray& paramsData);
    bool readExposureErrorFlag();
    bool clearExposureErrorFlag();



signals:
    void dataReceived(const QByteArray& data);
    void mcuSNReceived(const MCUSNData& data);
    void softwareVersionReceived(const QString& version);
    void adcDataReceived(const ADCData& data);
    void waveformDataReceived(uint16_t cmd, const WaveformData& data);
    void errorInfoReceived(const ErrorInfoData& data);
    void exposureStatusReceived(const ExposureStatusData& data);
    void exposureErrorFlagReceived(bool errorFlag);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString& error);

private:
    explicit SerialCommunication(QObject* parent = nullptr);
    ~SerialCommunication();

    // 禁止拷贝和赋值
    SerialCommunication(const SerialCommunication&) = delete;
    SerialCommunication& operator=(const SerialCommunication&) = delete;

    // CRC计算
    uint16_t calculateCRC16(const QByteArray& data);

    // 数据包构建
    QByteArray buildPacket(uint16_t cmd, uint8_t operate, const QByteArray& data);

    // 数据解析
    bool parseResponse(const QByteArray& data);
    void parseMCUSN(const QByteArray& data);
    void parseSoftwareVersion(const QByteArray& data);
    void parseADC(const QByteArray& data);
    void parseWaveform(uint16_t cmd, const QByteArray& data);
    void parseErrorInfo(const QByteArray& data);
    void parseExposureStatus(const QByteArray& data);
    void parseExposureErrorFlag(const QByteArray& data);

    // 串口数据接收处理
    void handleReadyRead();
    void processReceivedData();

private:
    static SerialCommunication* m_instance;
    static QMutex m_mutex;

    QSerialPort* m_serialPort;
    QByteArray m_receivedData;
    QTimer* m_receiveTimer;
    bool m_isReceiving;

    // 协议常量
    const uint8_t HEAD0 = 0x55;
    const uint8_t HEAD1 = 0xAA;
    const uint32_t DEFAULT_TX_UID = 0xFFFFFFFF;
    const uint32_t DEFAULT_RX_UID = 0x00000000;
    const uint32_t DEVICE_RX_UID = 0xFFFFFFFF;


};

#endif // SERIALCOMMUNICATION_H
