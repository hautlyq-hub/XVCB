#ifndef MODBUSCRC16_H
#define MODBUSCRC16_H

#include <QtCore/QByteArray>
#include <QtCore/QVector>

class ModbusCRC16
{
public:
    // 静态构造函数初始化表格（对应C#的静态构造函数）
    static void init();
    
    // 对应C#的CalculateWithTable方法
    static quint16 calculateWithTable(const quint8* data, int length);
    static quint16 calculateWithTable(const QByteArray& data, int length = -1);
    
    // 对应C#的Calculate方法
    static quint16 calculate(const quint8* data, int length);
    static quint16 calculate(const QByteArray& data, int length = -1);
    
private:
    static QVector<quint16> crcTable;
    static bool tableInitialized;
    
    // 私有构造函数防止实例化
    ModbusCRC16() = delete;
    ~ModbusCRC16() = delete;
    
    // 初始化CRC表格（对应C#的静态构造函数逻辑）
    static void initCRCTable();
};

#endif // MODBUSCRC16_H