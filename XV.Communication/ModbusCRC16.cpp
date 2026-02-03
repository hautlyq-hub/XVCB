#include "ModbusCRC16.h"
#include <QtCore/QDebug>

// 初始化静态成员变量
QVector<quint16> ModbusCRC16::crcTable(256);
bool ModbusCRC16::tableInitialized = false;

void ModbusCRC16::init()
{
    if (!tableInitialized) {
        initCRCTable();
        tableInitialized = true;
    }
}

void ModbusCRC16::initCRCTable()
{
    // 对应C#代码的静态构造函数逻辑
    // for (ushort i = 0; i < 256; i++) { ... }
    for (int i = 0; i < 256; i++) {
        quint16 crc = static_cast<quint16>(i);

        // 对应C#代码：for (byte j = 0; j < 8; j++) { ... }
        for (int j = 0; j < 8; j++) {
            // 对应C#代码：if ((crc & 1) != 0)
            if ((crc & 1) != 0) {
                // 对应C#代码：crc = (ushort)((crc >> 1) ^ 0xA001);
                crc = static_cast<quint16>((crc >> 1) ^ 0xA001);
            } else {
                // 对应C#代码：crc >>= 1;
                crc >>= 1;
            }
        }

        crcTable[i] = crc;
    }

    // 验证表格生成正确（可选）
    /*
    qDebug() << "CRC Table initialized:";
    for (int i = 0; i < 10; i++) {
        qDebug() << QString("crcTable[%1] = 0x%2")
                    .arg(i)
                    .arg(crcTable[i], 4, 16, QChar('0'));
    }
    */
}

quint16 ModbusCRC16::calculateWithTable(const quint8* data, int length)
{
    // 确保表格已初始化
    if (!tableInitialized) {
        initCRCTable();
        tableInitialized = true;
    }

    // 对应C#代码：
    // ushort crc = 0xFFFF;
    // int len = data.Length < length ? data.Length : length;
    quint16 crc = 0xFFFF;

    // 对应C#代码：for (int i = 0; i < len; i++)
    for (int i = 0; i < length; i++) {
        // 对应C#代码：crc = (ushort)((crc >> 8) ^ crcTable[(crc ^ data[i]) & 0xFF]);
        crc = static_cast<quint16>((crc >> 8) ^ crcTable[(crc ^ data[i]) & 0xFF]);
    }

    return crc;
}

quint16 ModbusCRC16::calculateWithTable(const QByteArray& data, int length)
{
    if (data.isEmpty()) {
        return 0xFFFF;
    }

    int len = (length < 0 || length > data.size()) ? data.size() : length;
    return calculateWithTable(reinterpret_cast<const quint8*>(data.constData()), len);
}

quint16 ModbusCRC16::calculate(const quint8* data, int length)
{
    // 对应C#代码：
    // ushort crc = 0xFFFF;
    // const ushort polynomial = 0xA001;
    // int len = data.Length < length ? data.Length : length;
    quint16 crc = 0xFFFF;
    const quint16 polynomial = 0xA001;

    // 对应C#代码：for (int a = 0; a < len; a++)
    for (int a = 0; a < length; a++) {
        // 对应C#代码：crc ^= data[a];
        crc ^= data[a];

        // 对应C#代码：for (int i = 0; i < 8; i++)
        for (int i = 0; i < 8; i++) {
            // 对应C#代码：if ((crc & 0x0001) != 0)
            if ((crc & 0x0001) != 0) {
                // 对应C#代码：crc = (ushort)((crc >> 1) ^ polynomial);
                crc = static_cast<quint16>((crc >> 1) ^ polynomial);
            } else {
                // 对应C#代码：crc >>= 1;
                crc >>= 1;
            }
        }
    }

    return crc;
}

quint16 ModbusCRC16::calculate(const QByteArray& data, int length)
{
    if (data.isEmpty()) {
        return 0xFFFF;
    }

    int len = (length < 0 || length > data.size()) ? data.size() : length;
    return calculate(reinterpret_cast<const quint8*>(data.constData()), len);
}
