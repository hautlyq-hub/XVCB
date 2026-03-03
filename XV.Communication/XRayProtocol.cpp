// XRayProtocol.cpp
#include "XRayProtocol.h"
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QProcess>
#include <QThread>
#include <QtMath>

const QByteArray XRayProtocol::CMD_VERSION = QByteArray::fromHex(
    "55AA12000C000000FFFFFFFF0000000029F2");

const QByteArray XRayProtocol::CMD_SERIAL_NUMBER = QByteArray::fromHex(
    "55AA12000A000000FFFFFFFF0000000021FA");

const QByteArray XRayProtocol::CMD_ADC_VALUES = QByteArray::fromHex(
    "55AA12000D000000FFFFFFFF00000000D431");

const QByteArray XRayProtocol::CMD_ERROR_QUERY = QByteArray::fromHex(
    "55AA12001D000000FFFFFFFF0000000015CE"); //DUI

const QByteArray XRayProtocol::CMD_ERROR_CLEAR = QByteArray::fromHex(
    "55AA12001D000100FFFFFFFF00000000440B");

const QByteArray XRayProtocol::CMD_EXPOSURE_QUERY = QByteArray::fromHex(
    "55AA16001B000000FFFFFFFF0000000000000000A760");

const QByteArray XRayProtocol::CMD_EXPOSURE_START = QByteArray::fromHex(
    "55AA16001B000100FFFFFFFF0000000001000100A7CD"); //DUI

const QByteArray XRayProtocol::CMD_EXPOSURE_STOP = QByteArray::fromHex(
    "55AA16001B000100FFFFFFFF0000000001000000A65D");

XRayProtocol::XRayProtocol(QObject *parent)
    : QObject(parent)
    , m_serialPort(nullptr)
    , m_lastError(XRAY_SUCCESS)
    , m_coolingRemaining(0)
    , m_expTimeMs(200)
    , m_respTimeout(1000)
{
    m_exposureTimer = new QTimer(this);
    m_exposureTimer->setSingleShot(false);

    // m_statusTimer = new QTimer(this);
    // connect(m_statusTimer, &QTimer::timeout, this, &XRayProtocol::updateDeviceStatus);
    // m_statusTimer->start(1000); // 每秒更新一次状态
}

XRayProtocol::~XRayProtocol()
{
    closeSerialPort();
    delete m_exposureTimer;
}

bool XRayProtocol::initSerialPort(const QString &portName, int baudRate)
{
    // 1. 检查串口设备文件是否存在（Linux下串口设备文件）
#ifdef Q_OS_LINUX
    if (!QFile::exists(portName)) {
        emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK,
                           tr("Serial port") + portName + tr("does not exist"));
        return false;
    }
#endif

    // 2. 检查是否有权限访问
    QFile testFile(portName);
    if (!testFile.open(QIODevice::ReadWrite)) {
        emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK,
                           tr("Cannot access serial port") + portName + tr("Error:"));
        return false;
    }
    testFile.close();

    m_serialPort = new QSerialPort();

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        QThread::msleep(500);
        m_serialPort->clear();

        QString version = getVersion();
        if (version.isEmpty()) {
            qWarning() << tr("Unable to get device version");
            delete m_serialPort;
            m_serialPort = nullptr;
            return false;
        }
    } else {
        qWarning() << tr("Failed to open port:") << portName;
        delete m_serialPort;
        return false;
    }

    // 发送连接成功信号
    QMetaObject::invokeMethod(
        this,
        [this]() {
            emit deviceConnected();
        },
        Qt::QueuedConnection);

    return true;
}

void XRayProtocol::closeSerialPort()
{
    qInfo() << tr("=== Closing X-ray serial port ===");

    if (m_serialPort) {
        qWarning() << tr("Actively close port") << m_serialPort->portName();

        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }

        delete m_serialPort;
        m_serialPort = nullptr;
    }

    QTimer::singleShot(0, this, [this]() {
        emit deviceDisconnected();
    });
}

bool XRayProtocol::isConnected() const
{
    return m_serialPort && m_serialPort->isOpen();
}

bool XRayProtocol::startExposure()
{

    // 构建开始曝光数据
    m_exposureStartData.clear();
    QDataStream startStream(&m_exposureStartData, QIODevice::WriteOnly);
    startStream.setByteOrder(QDataStream::LittleEndian);
    startStream << (quint16) 1 << (quint16) 1;

    // 构建停止曝光数据
    m_exposureStopData.clear();
    QDataStream stopStream(&m_exposureStopData, QIODevice::WriteOnly);
    stopStream.setByteOrder(QDataStream::LittleEndian);
    stopStream << (quint16) 1 << (quint16) 0;

    // 更新状态
    emit exposureStarted();

    // 记录开始时间
    m_over18SecSent = false;
    m_exposureStartTime = QDateTime::currentDateTime();
    qDebug() << m_exposureStartTime.toString("yyyyMMdd_HHmmss.zzz");
    // 启动定时器
    if (m_exposureTimer) {
        m_exposureTimer->disconnect();
        connect(m_exposureTimer, &QTimer::timeout, this, &XRayProtocol::onExposureTimeout);
        m_exposureTimer->start(50);
    }

    return true;
}

bool XRayProtocol::stopExposure()
{
    QByteArray response;
    if (sendCommand(0x1B, QByteArray::fromHex("01000000"))) {
        response = readResponse(m_respTimeout);
        if (response.isEmpty()) {
            return false;
        }
    }

    m_exposureTimer->stop();

    emit exposureStopped();

    return true;
}

QString XRayProtocol::portName() const
{
    return m_serialPort->portName();
}

bool XRayProtocol::checkExposureReady()
{
    QByteArray response;
    if (sendCommand(0x1B, QByteArray())) {
        response = readResponse(m_respTimeout);
        if (response.isEmpty()) {
            return false;
        }
    }

    // 解析状态数据
    if (response.size() >= 29) {
        QDataStream stream(response.mid(16));
        stream.setByteOrder(QDataStream::LittleEndian);

        quint16 exp_en;
        quint16 exp_mode;
        quint8 low_battery_flag;
        quint8 hard_check_result;
        quint16 temp_late_time;
        quint16 exp_cooling_time;
        quint8 exp_step;

        stream >> exp_en;
        stream >> exp_mode;
        stream >> low_battery_flag;
        stream >> hard_check_result;
        stream >> temp_late_time;
        stream >> exp_cooling_time;
        stream >> exp_step;

        // 检查设备状态
        bool isReady = (exp_en == 0) && (exp_mode == 0) && (low_battery_flag == 0)
                       && (hard_check_result == 0) && (temp_late_time == 0)
                       && (exp_cooling_time == 0) && (exp_step != 9);

        return isReady;
    }

    return false;
}

QByteArray XRayProtocol::getKVWaveform()
{
    // 获取KV波形数据（分包获取）
    return getWaveformData(0x0F);
}

QByteArray XRayProtocol::getWaveformData(quint16 waveformType)
{
    QByteArray waveform;
    quint16 packetIndex = 0;
    const quint16 packetSize = 64;

    while (true) {
        // 构建波形请求包
        QByteArray requestData;
        QDataStream stream(&requestData, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << packetIndex;
        stream << packetSize;

        QByteArray response;
        if (sendCommand(waveformType, requestData)) {
            response = readResponse(m_respTimeout);
        }

        // 解析响应
        if (response.size() >= 20) {
            QDataStream respStream(response.mid(16));
            respStream.setByteOrder(QDataStream::LittleEndian);

            quint16 respIndex;
            quint16 respLength;
            respStream >> respIndex;
            respStream >> respLength;

            if (respIndex != packetIndex) {
                qWarning() << tr("Packet index mismatch");
                break;
            }

            if (respLength == 0 && packetIndex != 0) {
                break; // 数据接收完成
            }

            // 提取波形数据
            int dataStart = 20; // 命令头 + index + length
            QByteArray packetData = response.mid(dataStart, respLength);
            waveform.append(packetData);

            packetIndex++;
        } else {
            break;
        }
    }

    return waveform;
}

bool XRayProtocol::verifyCRC(const QByteArray &data)
{
    if (data.size() < 4) { // 至少要有包头和CRC
        qWarning() << tr("Packet too small, cannot verify CRC");
        return false;
    }

    // 提取接收到的CRC（小端序）
    quint16 receivedCrc = static_cast<quint8>(data[data.size() - 2])
                          | (static_cast<quint8>(data[data.size() - 1]) << 8);

    // 计算数据的CRC（不包括最后2个字节）
    QByteArray dataWithoutCrc = data.left(data.size() - 2);

    ModbusCRC16::init();
    quint16 calculatedCrc = ModbusCRC16::calculateWithTable(dataWithoutCrc);

    return receivedCrc == calculatedCrc;
}

QByteArray XRayProtocol::buildCommand(quint16 cmdCode, quint8 operate, const QByteArray &data)
{
    QByteArray command;

    // HEAD0, HEAD1 (0x55, 0xAA)
    command.append(static_cast<char>(0x55));
    command.append(static_cast<char>(0xAA));

    // 先留出长度字段的位置
    int lenPos = command.size();             // 记住长度字段的位置
    command.append(static_cast<char>(0x00)); // LEN_L 占位
    command.append(static_cast<char>(0x00)); // LEN_H 占位

    // CMD (小端)
    command.append(static_cast<char>(cmdCode & 0xFF));        // CMD_L
    command.append(static_cast<char>((cmdCode >> 8) & 0xFF)); // CMD_H

    // OPERATE
    command.append(static_cast<char>(operate));

    // RESERVE
    command.append(static_cast<char>(0x00));

    // TX_UID: 0xFFFFFFFF
    command.append(static_cast<char>(0xFF));
    command.append(static_cast<char>(0xFF));
    command.append(static_cast<char>(0xFF));
    command.append(static_cast<char>(0xFF));

    // RX_UID: 0x00000000
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));
    command.append(static_cast<char>(0x00));

    // DATA
    if (!data.isEmpty()) {
        command.append(data);
    }

    // 现在计算总长度（包括CRC的2字节）
    quint16 totalLength = command.size() + 2; // 当前长度 + CRC的2字节

    // 回填长度字段（小端）
    command[lenPos] = static_cast<char>(totalLength & 0xFF);            // LEN_L
    command[lenPos + 1] = static_cast<char>((totalLength >> 8) & 0xFF); // LEN_H

    // 计算CRC（对整帧数据除了CRC部分）
    quint16 crc = ModbusCRC16::calculate(command);

    // 添加CRC（小端）
    command.append(static_cast<char>(crc & 0xFF));        // CRC_L
    command.append(static_cast<char>((crc >> 8) & 0xFF)); // CRC_H

    return command;
}

bool XRayProtocol::sendCommand(quint16 cmdCode, const QByteArray &data)
{
    // 清空接收缓冲区
    m_serialPort->clear(QSerialPort::Input);
    QThread::msleep(10);

    QByteArray command;

    // 使用已验证的命令模板
    switch (cmdCode) {
    case 0x0C: // 获取版本
        command = CMD_VERSION;
        break;
    case 0x0A: // 获取序列号
        command = CMD_SERIAL_NUMBER;
        break;
    case 0x0D: // 获取ADC值
        command = CMD_ADC_VALUES;
        break;
    case 0x18: // 报错查询
        command = CMD_ERROR_QUERY;

        break;
    case 0x1D: // 曝光错误标志处理
        if (data == QByteArray::fromHex("01000000")) {
            command = CMD_ERROR_CLEAR; // 清除错误
        } else {
            // 查询错误标志 - 使用构建命令
            command = buildCommand(cmdCode, 0x00, QByteArray());
        }
        break;
    case 0x1B: // 曝光指令
    {
        if (data.isEmpty()) {
            // 查询命令 - 使用已验证的查询格式
            command = CMD_EXPOSURE_QUERY;
        } else if (data.size() == 4) {
            // 提取参数
            quint16 exposureEnable = static_cast<quint8>(data[0])
                                     | (static_cast<quint8>(data[1]) << 8);
            quint16 exposureMode = static_cast<quint8>(data[2])
                                   | (static_cast<quint8>(data[3]) << 8);

            if (exposureEnable == 1 && exposureMode == 1) {
                // 开始曝光
                command = CMD_EXPOSURE_START;
            } else if (exposureEnable == 1 && exposureMode == 0) {
                // 停止曝光
                command = CMD_EXPOSURE_STOP;
            } else {
                // 其他参数使用构建命令
                command = buildCommand(cmdCode, 0x01, data);
            }
        } else {
            qWarning() << tr("Exposure command data format error");
            return false;
        }
        break;
    }
    case 0x1C: // 曝光参数
        if (data.isEmpty()) {
            // 查询参数
            command = buildCommand(cmdCode, 0x00, QByteArray());
        } else if (data.size() == 36) {
            // 设置参数
            command = buildCommand(cmdCode, 0x01, data);
        } else {
            qWarning() << tr("Exposure parameter command data size error");
            return false;
        }
        break;
    default:
        // 其他命令使用通用构建
        command = buildCommand(cmdCode, 0x00, data);
        break;
    }

    if (command.isEmpty()) {
        qWarning() << tr("Failed to build command");
        return false;
    }

    // 清空接收缓冲区
    m_serialPort->clear(QSerialPort::Input);
    QThread::msleep(10);

    // 发送命令
    QString hexStr = command.toHex(' ').toUpper();
    qDebug() << "req:" << hexStr << tr("to port") << m_serialPort->portName();

    m_serialPort->write(command);

    if (!m_serialPort->waitForBytesWritten(100)) {
        qWarning() << tr("Write timeout");
        return false;
    }

    m_serialPort->flush();
    return true;
}

QByteArray XRayProtocol::readResponse(int timeout)
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        qWarning() << tr("readResponse: Serial port not open");
        return QByteArray();
    }

    QElapsedTimer timer;
    timer.start();
    QByteArray buffer;

    while (timer.elapsed() < timeout) {
        if (m_serialPort->waitForReadyRead(10)) {
            buffer.append(m_serialPort->readAll());

            QByteArray packet = extractCompletePacket(buffer);
            if (!packet.isEmpty() && verifyCRC(packet)) {
                // qDebug() << "resp:" << packet.toHex(' ').toUpper();
                return packet;
            }
        }
    }

    return QByteArray();
}

QByteArray XRayProtocol::extractCompletePacket(QByteArray &buffer)
{
    if (buffer.size() < 4) {
        return QByteArray();
    }

    const quint8 *data = reinterpret_cast<const quint8 *>(buffer.constData());
    int bufferSize = buffer.size();
    int packetStart = -1;

    for (int i = 0; i <= bufferSize - 2; ++i) {
        if (data[i] == 0x55 && data[i + 1] == 0xAA) {
            packetStart = i;
            break;
        }
    }

    if (packetStart == -1) {
        if (bufferSize > 256) {
            buffer = buffer.right(128);
        }
        return QByteArray();
    }

    // 移除包头之前的数据
    if (packetStart > 0) {
        buffer.remove(0, packetStart);
    }

    data = reinterpret_cast<const quint8 *>(buffer.constData());
    bufferSize = buffer.size();

    if (bufferSize < 4) {
        return QByteArray();
    }

    quint16 packetLength = data[2] | (data[3] << 8);

    const quint16 MIN_PACKET_SIZE = 16;
    const quint16 MAX_PACKET_SIZE = 1024;

    if (packetLength < MIN_PACKET_SIZE || packetLength > MAX_PACKET_SIZE) {
        if (bufferSize > 1) {
            buffer.remove(0, 1);
        } else {
            buffer.clear();
        }
        return QByteArray();
    }

    if (bufferSize >= packetLength) {
        QByteArray packet = buffer.left(packetLength);

        if (verifyCRC(packet)) {
            buffer.remove(0, packetLength);
            return packet;
        } else {
            buffer.remove(0, packetLength);
            return QByteArray();
        }
    }

    return QByteArray();
}

void XRayProtocol::handleExposureResponse(const QByteArray &response)
{
    if (response.size() < 29) {
        qWarning() << tr("Invalid exposure response size:") << response.size();
        return;
    }

    QDataStream stream(response.mid(16));
    stream.setByteOrder(QDataStream::LittleEndian);

    quint16 exp_en;
    quint16 exp_mode;
    quint8 low_battery_flag;
    quint8 hard_check_result;
    quint16 temp_late_time;
    quint16 exp_cooling_time;
    quint8 exp_step;

    stream >> exp_en;
    stream >> exp_mode;
    stream >> low_battery_flag;
    stream >> hard_check_result;
    stream >> temp_late_time;
    stream >> exp_cooling_time;
    stream >> exp_step;

    // 检查错误
    if (low_battery_flag == 1) {
        m_lastError = XRAY_ERROR_LOW_BATTERY;
        emit errorOccurred(XRAY_ERROR_LOW_BATTERY, tr("Low battery"));
    }

    if (hard_check_result == 1 || hard_check_result == 2) {
        m_lastError = XRAY_ERROR_HARDWARE_CHECK;
        emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK, tr("Hardware check abnormal"));
    }

    if (exp_step == 9) {
        m_lastError = XRAY_ERROR_EXPOSURE_TIMEOUT;
        emit exposureError(XRAY_ERROR_EXPOSURE_TIMEOUT);
    }

}

QString XRayProtocol::parseSerialNumber(const QByteArray &data)
{
    if (data.isEmpty() || data.size() < 28) {
        qDebug() << tr("Invalid data for serial number parsing, size:") << data.size();
        return QString();
    }

    // 根据协议，序列号数据在特定位置
    // 示例: HEX RX: 55 AA 1E 00 0A 00 03 00 32 00 57 00 FF FF FF FF 38 30 30 20 01 50 56 58 32 00 57 00 95 95
    // MCU UID2: 0x20303038
    // MCU UID1: 0x58565001
    // MCU UID0: 0x00570032

    QString serialNumber;

    // 提取UID数据（12字节）
    int startIndex = 16; // 从第16字节开始
    if (data.size() >= startIndex + 12) {
        QByteArray uidData = data.mid(startIndex, 12);

        // 将二进制数据转换为十六进制字符串
        serialNumber = uidData.toHex().toUpper();

        qDebug() << tr("Raw serial number HEX:") << serialNumber;

        // 如果需要格式化为更可读的格式
        if (serialNumber.length() >= 24) {
            // 格式化为: XXXX-XXXX-XXXX
            QString formatted = QString("%1-%2-%3")
                                    .arg(serialNumber.mid(0, 8))
                                    .arg(serialNumber.mid(8, 8))
                                    .arg(serialNumber.mid(16, 8));
            qDebug() << tr("Formatted serial number:") << formatted;
            return formatted;
        }

        return serialNumber;
    }

    qDebug() << tr("Failed to parse serial number from data");
    return QString();
}

void XRayProtocol::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
        qWarning() << tr("Serial port error:") << error << m_serialPort->errorString();

        // 根据错误类型处理
        switch (error) {
        case QSerialPort::PermissionError:
            emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK, tr("Serial port access denied"));
            break;
        case QSerialPort::DeviceNotFoundError:
            emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK, tr("Device not found"));
            break;
        case QSerialPort::ResourceError:
            emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK, tr("Device removed"));
            closeSerialPort();
            break;
        default:
            emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK, tr("Serial port communication error"));
            break;
        }
    }
}

void XRayProtocol::onExposureTimeout()
{
    QMutexLocker locker(&m_serialMutex);

    // 检查是否达到设定曝光时间
    qint64 elapsed = m_exposureStartTime.msecsTo(QDateTime::currentDateTime());

    if (elapsed >= m_expTimeMs + 2000) {
        m_exposureTimer->stop();
        if (sendCommand(0x1B, m_exposureStopData)) {
            QByteArray resp = readResponse(m_respTimeout);
        }
        emit exposureStopped();
    }
    // else if (elapsed >= 1800 && !m_over18SecSent) {
    //     m_over18SecSent = true; // 标记已发送
    //     // emit exposureWarning("曝光时间已超过1.8秒");
    //     //buchuguang
    //     sendCommand(0x1B, m_exposureStartData);
    // }
    else {
        // 正常曝光中，继续发送曝光命令
        if (sendCommand(0x1B, m_exposureStartData)) {
            QByteArray resp = readResponse(m_respTimeout);
        }
    }
}

void XRayProtocol::updateDeviceStatus()
{
    QByteArray response;
    if (sendCommand(0x1B, QByteArray())) {
        response = readResponse(m_respTimeout);
    }

    if (!response.isEmpty()) {
        handleExposureResponse(response);
    }

    // 更新温度等信息
    // ADCValues adcValues = getADCValues();
    // emit adcValuesUpdated(adcValues);
    // emit temperatureUpdated(adcValues.device_temp, adcValues.mcu_temp);
}

// 数据解析方法实现
ADCValues XRayProtocol::parseADCValues(const QByteArray &data)
{
    ADCValues values;

    if (data.size() >= 44) { // 根据协议，ADC数据包至少44字节
        QDataStream stream(data.mid(16));
        stream.setByteOrder(QDataStream::LittleEndian);

        stream >> values.charge_current;
        stream >> values.ovc2;
        stream >> values.battery_div;
        stream >> values.kv_ref;
        stream >> values.ma;
        stream >> values.fill;
        stream >> values.kv;
        stream >> values.mcu_temp;
        stream >> values.mcu_vref;
        stream >> values.device_temp;

        // 添加详细输出
        qDebug() << tr("ADC values parsed:");
        qDebug() << tr("  Charge current:") << values.charge_current;
        qDebug() << tr("  Battery divider:") << values.battery_div;
        qDebug() << tr("  KV reference:") << values.kv_ref;
        qDebug() << tr("  mA:") << values.ma;
        qDebug() << tr("  Filament current:") << values.fill;
        qDebug() << tr("  KV:") << values.kv;
        qDebug() << tr("  MCU temperature:") << values.mcu_temp << "°C";
        qDebug() << tr("  MCU reference voltage:") << values.mcu_vref << "V";
        qDebug() << tr("  Device temperature:") << values.device_temp << "°C";
    } else {
        qWarning() << tr("ADC data packet too small:") << data.size();
    }

    return values;
}

QString XRayProtocol::parseVersion(const QByteArray &data)
{
    if (data.size() < 10) {
        qWarning() << tr("Version data packet too small:") << data.size();
        return QString();
    }

    // 版本字符串从第17字节开始，以0x00结束
    int start = 16;
    int end = data.indexOf('\0', start);

    if (end == -1) {
        end = data.size();
    }

    QByteArray versionData = data.mid(start, end - start);
    QString version = QString::fromLatin1(versionData);

    return version;
}

ExposureParams XRayProtocol::parseExposureParams(const QByteArray &data)
{
    ExposureParams params;

    if (data.size() < 54) {
        qWarning() << tr("Exposure parameter response too small:") << data.size();
        return params;
    }

    if (static_cast<quint8>(data[0]) != 0x55 || static_cast<quint8>(data[1]) != 0xAA) {
        qWarning() << tr("Invalid exposure parameter response header");
        return params;
    }

    quint16 cmd = static_cast<quint8>(data[4]) | (static_cast<quint8>(data[5]) << 8);
    if (cmd != 0x1C) {
        qWarning() << tr("Invalid command code in response:") << QString::number(cmd, 16);
        return params;
    }

    quint8 operate = static_cast<quint8>(data[6]);
    if (operate != 0x03) {
        qWarning() << tr("Invalid operate code in response: expected 0x03, got")
                   << QString::number(operate, 16);
    }

    const char *ptr = data.constData() + 16;

    memcpy(&params.ma_min, ptr, 4);
    ptr += 4;
    memcpy(&params.ma_max, ptr, 4);
    ptr += 4;
    memcpy(&params.ma_val, ptr, 4);
    ptr += 4;
    memcpy(&params.kvs_min, ptr, 4);
    ptr += 4;
    memcpy(&params.kvs_max, ptr, 4);
    ptr += 4;
    memcpy(&params.kv_min, ptr, 4);
    ptr += 4;
    memcpy(&params.kv_max, ptr, 4);
    ptr += 4;
    memcpy(&params.kv_val, ptr, 4);
    ptr += 4;
    memcpy(&params.exp_time_ms, ptr, 4);

    qDebug() << tr("Exposure parameters parsed:");
    qDebug() << tr("  OPERATE:") << QString::number(operate, 16);
    qDebug() << tr("  MA min:") << params.ma_min;
    qDebug() << tr("  MA max:") << params.ma_max;
    qDebug() << tr("  MA val:") << params.ma_val;
    qDebug() << tr("  KVS min:") << params.kvs_min;
    qDebug() << tr("  KVS max:") << params.kvs_max;
    qDebug() << tr("  KV min:") << params.kv_min;
    qDebug() << tr("  KV max:") << params.kv_max;
    qDebug() << tr("  KV val:") << params.kv_val;
    qDebug() << tr("  Exposure time:") << params.exp_time_ms << "ms";

    return params;
}

QString XRayProtocol::getVersion()
{
    QByteArray response;
    if (sendCommand(0x0C, QByteArray())) {
        response = readResponse(m_respTimeout);
    }
    return parseVersion(response);
}

QString XRayProtocol::getSerialNumber()
{
    QByteArray response;
    if (sendCommand(0x0A, QByteArray())) {
        response = readResponse(m_respTimeout);
    }
    return parseSerialNumber(response);
}

ADCValues XRayProtocol::getADCValues()
{
    QByteArray response;
    if (sendCommand(0x0D, QByteArray())) {
        response = readResponse(m_respTimeout);
    }
    return parseADCValues(response);
}

bool XRayProtocol::setExposureParams(const ExposureParams &params)
{
    // 构建参数数据包 (9个参数 × 4字节 = 36字节)
    QByteArray data;
    data.resize(36);
    char *ptr = data.data();

    // 使用memcpy确保数据正确拷贝
    memcpy(ptr, &params.ma_min, 4);
    ptr += 4;
    memcpy(ptr, &params.ma_max, 4);
    ptr += 4;
    memcpy(ptr, &params.ma_val, 4);
    ptr += 4;
    memcpy(ptr, &params.kvs_min, 4);
    ptr += 4;
    memcpy(ptr, &params.kvs_max, 4);
    ptr += 4;
    memcpy(ptr, &params.kv_min, 4);
    ptr += 4;
    memcpy(ptr, &params.kv_max, 4);
    ptr += 4;
    memcpy(ptr, &params.kv_val, 4);
    ptr += 4;
    memcpy(ptr, &params.exp_time_ms, 4);

    // 保存设置的曝光时间
    m_expTimeMs = params.exp_time_ms;

    // 发送命令
    QByteArray response;
    if (sendCommand(0x1C, data)) {
        response = readResponse(m_respTimeout);
    }

    if (response.isEmpty()) {
        qWarning() << tr("No response to exposure parameter setting");
        return false;
    }

    // 解析响应数据
    ExposureParams responseParams = parseExposureParams(response);

    // 验证参数
    bool success = true;
    const float epsilon = 0.001f;

    if (qAbs(responseParams.ma_min - params.ma_min) > epsilon) {
        qWarning() << tr("MA min mismatch: set") << params.ma_min << tr("got")
                   << responseParams.ma_min;
        success = false;
    }

    if (qAbs(responseParams.ma_max - params.ma_max) > epsilon) {
        qWarning() << tr("MA max mismatch: set") << params.ma_max << tr("got")
                   << responseParams.ma_max;
        success = false;
    }

    if (qAbs(responseParams.ma_val - params.ma_val) > epsilon) {
        qWarning() << tr("MA val mismatch: set") << params.ma_val << tr("got")
                   << responseParams.ma_val;
        success = false;
    }

    if (qAbs(responseParams.kvs_min - params.kvs_min) > epsilon) {
        qWarning() << tr("KVS min mismatch: set") << params.kvs_min << tr("got")
                   << responseParams.kvs_min;
        success = false;
    }

    if (qAbs(responseParams.kvs_max - params.kvs_max) > epsilon) {
        qWarning() << tr("KVS max mismatch: set") << params.kvs_max << tr("got")
                   << responseParams.kvs_max;
        success = false;
    }

    if (qAbs(responseParams.kv_min - params.kv_min) > epsilon) {
        qWarning() << tr("KV min mismatch: set") << params.kv_min << tr("got")
                   << responseParams.kv_min;
        success = false;
    }

    if (qAbs(responseParams.kv_max - params.kv_max) > epsilon) {
        qWarning() << tr("KV max mismatch: set") << params.kv_max << tr("got")
                   << responseParams.kv_max;
        success = false;
    }

    if (qAbs(responseParams.kv_val - params.kv_val) > epsilon) {
        qWarning() << tr("KV val mismatch: set") << params.kv_val << tr("got")
                   << responseParams.kv_val;
        success = false;
    }

    if (responseParams.exp_time_ms != params.exp_time_ms) {
        qWarning() << tr("Exposure time mismatch: set") << params.exp_time_ms << tr("got")
                   << responseParams.exp_time_ms;
        success = false;
    }

    if (success) {
        qDebug() << tr("Exposure parameters set and verified successfully");
        m_currentParams = responseParams;
    } else {
        qWarning() << tr("Exposure parameter verification failed");
    }

    return success;
}

// 提取故障清除方法
bool XRayProtocol::clearFaults()
{
    QByteArray response;
    if (sendCommand(0x1D, QByteArray::fromHex("01000000"))) {
        response = readResponse(m_respTimeout);
    }

    if (response.isEmpty()) {
        return false;
    }

    qDebug() << tr("Fault cleared successfully");

    return true;
}

// 错误处理
bool XRayProtocol::clearErrorFlags()
{
    QByteArray response;
    if (sendCommand(0x1D, QByteArray::fromHex("01000000"))) {
        response = readResponse(m_respTimeout);
    }
    return !response.isEmpty();
}

XRayErrorCode XRayProtocol::getLastError() const
{
    return m_lastError;
}

QString XRayProtocol::getErrorString(XRayErrorCode error) const
{
    static QMap<XRayErrorCode, QString> errorStrings
        = {{XRAY_SUCCESS, tr("Success")},
           {XRAY_ERROR_OVER_KV, tr("KV too high")},
           {XRAY_ERROR_LOW_MA, tr("mA too low")},
           {XRAY_ERROR_OVER_MA, tr("mA too high")},
           {XRAY_ERROR_OVER_CURRENT, tr("Over current")},
           {XRAY_ERROR_LOW_FIL_I, tr("Filament current too low")},
           {XRAY_ERROR_OVER_KV_S, tr("KV setting too high")},
           {XRAY_ERROR_KEY_RELEASE, tr("Key release error")},
           {XRAY_ERROR_KEY_NO_RELEASE, tr("Key not release error")},
           {XRAY_ERROR_LOW_BATTERY, tr("Low battery")},
           {XRAY_ERROR_HARDWARE_CHECK, tr("Hardware check abnormal")},
           {XRAY_ERROR_COOLING, tr("Cooling")},
           {XRAY_ERROR_EXPOSURE_TIMEOUT, tr("Exposure timeout")}};

    return errorStrings.value(error, tr("Unknown error"));
}
