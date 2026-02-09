#include "SensorCRC16.h"
#include <QtCore/QDebug>

QByteArray SensorCRC16::calculate(const QByteArray& data)
{
    if (data.isEmpty()) {
        return QByteArray(2, 0);
    }

    // 计算累加和
    quint64 sum = 0;
    for (char byte : data) {
        sum += static_cast<quint8>(byte);
    }

    // 取低16位
    quint16 crcValue = static_cast<quint16>(sum & 0xFFFF);

    // 交换高低字节 - 单片机也是这么做的
    //quint16 swapped = ((crcValue << 8) & 0xFF00) | ((crcValue >> 8) & 0x00FF);

    // ⚠️ 关键修改：直接返回内存表示，不要再转大端序
    QByteArray result;

    result.append(static_cast<char>(crcValue & 0xFF));        // 低字节在前
    result.append(static_cast<char>((crcValue >> 8) & 0xFF)); // 高字节在后

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
