#include "SensorCRC16.h"
#include <QtCore/QDebug>

QByteArray SensorCRC16::calculate(const QByteArray& data)
{
    if (data.isEmpty()) {
        return QByteArray(2, 0);
    }

    // 步骤1：计算所有字节的累加和（对应C#的pSrcBytes.ToList().ForEach）
    quint64 sum = 0;
    for (char byte : data) {
        sum += static_cast<quint8>(byte);
    }

    // 取低16位
    quint16 crcValue = static_cast<quint16>(sum & 0xFFFF);

    // 步骤2：交换高低字节（对应C#的(crcValue << 8) & 0xFF00 | (crcValue >> 8) & 0x00FF）
    quint16 swapped = ((crcValue << 8) & 0xFF00) | ((crcValue >> 8) & 0x00FF);

    // 步骤3：转换为大端序（对应C#的.Reverse().ToArray()）
    QByteArray result(2, 0);
    result[0] = static_cast<char>((swapped >> 8) & 0xFF); // 高位在前
    result[1] = static_cast<char>(swapped & 0xFF);        // 低位在后

    return result;
}

QString SensorCRC16::calculateHexStr(const QByteArray& data)
{
    QByteArray crcBytes = calculate(data);

    // 格式化为十六进制字符串，类似C#的"   " + BitConverter.ToString(crcBytes).Replace("-", " ")
    QString hexStr = QString("%1 %2")
                         .arg(static_cast<quint8>(crcBytes[0]), 2, 16, QLatin1Char('0'))
                         .arg(static_cast<quint8>(crcBytes[1]), 2, 16, QLatin1Char('0'))
                         .toUpper();

    return "   " + hexStr;
}
