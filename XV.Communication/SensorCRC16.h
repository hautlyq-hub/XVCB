#ifndef SENSORCRC16_H
#define SENSORCRC16_H

#include <QtCore/QByteArray>

class SensorCRC16
{
public:
    // 计算传感器协议使用的CRC（基于字节累加和）
    static QByteArray calculate(const QByteArray& data);

    // 计算并返回十六进制字符串格式（像C#的CRC16HexStr）
    static QString calculateHexStr(const QByteArray& data);

private:
    SensorCRC16() = delete;
    ~SensorCRC16() = delete;
};

#endif // SENSORCRC16_H
