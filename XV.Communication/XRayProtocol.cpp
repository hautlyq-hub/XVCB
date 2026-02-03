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
    "55AA12001D000000FFFFFFFF0000000015CE");

const QByteArray XRayProtocol::CMD_ERROR_CLEAR = QByteArray::fromHex(
    "55AA12001D000100FFFFFFFF00000000440B");

const QByteArray XRayProtocol::CMD_EXPOSURE_QUERY = QByteArray::fromHex(
    "55AA16001B000000FFFFFFFF0000000000000000A760");

const QByteArray XRayProtocol::CMD_EXPOSURE_START = QByteArray::fromHex(
    "55AA16001B000100FFFFFFFF0000000001000100A7CD");

const QByteArray XRayProtocol::CMD_EXPOSURE_STOP = QByteArray::fromHex(
    "55AA16001B000100FFFFFFFF0000000001000000A65D");

XRayProtocol::XRayProtocol(QObject *parent)
    : QObject(parent)
    , m_serialPort(nullptr)
    , m_deviceStatus(DEVICE_IDLE)
    , m_lastError(XRAY_SUCCESS)
    , m_coolingRemaining(0)
    , m_expTimeMs(30000)
{
    m_exposureTimer = new QTimer(this);
    m_exposureTimer->setSingleShot(false);

    // m_statusTimer = new QTimer(this);
    // connect(m_statusTimer, &QTimer::timeout, this, &XRayProtocol::updateDeviceStatus);
    // m_statusTimer->start(1000); // 每秒更新一次状态

    qDebug() << "XRayProtocol 构造函数，线程:" << QThread::currentThreadId()
             << "对象线程:" << this->thread();
}

XRayProtocol::~XRayProtocol()
{
    closeSerialPort();
    delete m_exposureTimer;
}

bool XRayProtocol::initSerialPort(const QString &portName, int baudRate)
{
    if (QThread::currentThread() != this->thread()) {
        qCritical() << "❌ 跨线程访问XRayProtocol串口！";
        qCritical() << "对象在线程:" << this->thread();
        qCritical() << "当前线程:" << QThread::currentThread();

        // 自动调度到正确线程
        bool result = false;
        QMetaObject::invokeMethod(
            this,
            [this, portName, baudRate, &result]() { result = initSerialPort(portName, baudRate); },
            Qt::BlockingQueuedConnection);
        return result;
    }

    QString fullPortName = portName;
    if (!fullPortName.startsWith("/dev/")) {
        fullPortName = "/dev/" + fullPortName;
    }

    // 检查端口文件是否存在
    QFileInfo portFile(fullPortName);
    if (!portFile.exists()) {
        qWarning() << "端口文件不存在:" << fullPortName;
        return false;
    }

    // 清理旧的串口
    if (m_serialPort) {
        delete m_serialPort;
        m_serialPort = nullptr;
    }

    m_serialPort = new QSerialPort();

    m_serialPort->setPortName(fullPortName);
    m_serialPort->setBaudRate(921600);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        qWarning() << "无法打开串口:" << fullPortName;
        delete m_serialPort;
        m_serialPort = nullptr;
        return false;
    }

    // 测试连接
    QString version = getVersion();
    if (version.isEmpty()) {
        qWarning() << "无法获取设备版本号";
        delete m_serialPort;
        m_serialPort = nullptr;
        return false;
    }

    // 发送连接成功信号
    QMetaObject::invokeMethod(
        this,
        [this]() {
            m_deviceStatus = DEVICE_READY;
            emit deviceConnected();
            emit deviceStatusChanged(m_deviceStatus);
        },
        Qt::QueuedConnection);

    return true;
}

void XRayProtocol::closeSerialPort()
{
    qInfo() << "=== 关闭X光串口 (曝光后) ===";
    qInfo() << "设备状态:" << m_deviceStatus << "曝光定时器激活:" << m_exposureTimer->isActive();

    if (m_serialPort) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }

        delete m_serialPort;
        m_serialPort = nullptr;
    }

    m_deviceStatus = DEVICE_IDLE;

    QTimer::singleShot(0, this, [this]() {
        emit deviceDisconnected();
        emit deviceStatusChanged(m_deviceStatus);
    });
}

bool XRayProtocol::isConnected() const
{
    return m_serialPort && m_serialPort->isOpen();
}

QString XRayProtocol::getVersion()
{
    QByteArray response = executeCommand(0x0C, QByteArray(), 500);
    return parseVersion(response);
}

QString XRayProtocol::getSerialNumber()
{
    QByteArray response = executeCommand(0x0A, QByteArray(), 500);
    return parseSerialNumber(response);
}

ADCValues XRayProtocol::getADCValues()
{
    QByteArray response = executeCommand(0x0D, QByteArray(), 500);
    return parseADCValues(response);
}

bool XRayProtocol::setExposureParams(const ExposureParams &params)
{
    if (!isConnected()) {
        qWarning() << "Device not connected";
        return false;
    }

    // 构建参数数据包
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << params.ma_min;
    stream << params.ma_max;
    stream << params.ma_val;
    stream << params.kvs_min;
    stream << params.kvs_max;
    stream << params.kv_min;
    stream << params.kv_max;
    stream << params.kv_val;
    stream << params.exp_time_ms;

    m_expTimeMs = params.exp_time_ms;
    QByteArray response = executeCommand(0x1C, data, 1000);

    if (response.isEmpty()) {
        return false;
    }

    // 解析响应，验证参数设置成功
    if (response.size() >= 44) {
        // 可以解析返回的参数进行验证
        m_currentParams = params;
        return true;
    }

    return false;
}

// 提取曝光状态检查方法
bool XRayProtocol::checkExposureStatus(ExposureStatus &status)
{
    // 使用正确的查询命令
    QByteArray queryResponse = executeCommand(0x1B, QByteArray(), 1000);

    if (queryResponse.isEmpty() || queryResponse.size() < 29) {
        qWarning() << "曝光状态查询失败或无响应";
        return false;
    }

    // 调试输出收到的完整响应
    qDebug() << "曝光查询响应HEX:" << queryResponse.toHex(' ').toUpper();

    // 解析状态 - 从第16字节开始
    QDataStream stream(queryResponse.mid(16));
    stream.setByteOrder(QDataStream::LittleEndian);

    stream >> status.exposureEnable;
    stream >> status.exposureMode;
    stream >> status.lowBattery;
    stream >> status.hardwareCheck;
    stream >> status.delayShutdown;
    stream >> status.cooldownTime;
    stream >> status.exposureStep;

    qDebug() << "解析曝光状态:";
    qDebug() << "  曝光使能:" << status.exposureEnable;
    qDebug() << "  曝光模式:" << status.exposureMode;
    qDebug() << "  低电量:" << status.lowBattery;
    qDebug() << "  硬件检测:" << status.hardwareCheck;
    qDebug() << "  延迟关机:" << status.delayShutdown << "ms";
    qDebug() << "  冷却时间:" << status.cooldownTime << "s";
    qDebug() << "  曝光步骤:" << status.exposureStep;

    // 检查异常状态
    if (status.exposureEnable == 0xFFFF) {
        qWarning() << "⚠️ 设备返回异常状态 (曝光使能=65535)，需要重置设备";
        return false;
    }

    return true;
}

// 提取故障清除方法
bool XRayProtocol::clearFaults()
{
    qDebug() << "清除设备故障...";

    // 1. 发送清除错误命令 (0x1D, OPERATE=0x01)
    QByteArray clearData = QByteArray::fromHex("01000000");
    QByteArray response = executeCommand(0x1D, clearData, 500);

    if (response.isEmpty()) {
        qWarning() << "清除故障失败";
        return false;
    }

    qDebug() << "故障清除成功";

    // 2. 等待设备复位
    QThread::msleep(300);

    // 3. 发送停止曝光命令，确保设备处于停止状态
    QByteArray stopData;
    stopData.append(static_cast<char>(0x01)); // 使能=1
    stopData.append(static_cast<char>(0x00));
    stopData.append(static_cast<char>(0x00)); // 模式=0（停止）
    stopData.append(static_cast<char>(0x00));

    executeCommand(0x1B, stopData, 300);

    // 4. 再次清除错误（双重保险）
    executeCommand(0x1D, clearData, 300);

    QThread::msleep(200);

    return true;
}

// 提取使能曝光方法
bool XRayProtocol::enableExposure()
{
    qDebug() << "使能设备曝光...";

    QByteArray enableData;
    enableData.append(static_cast<char>(0x01)); // 曝光使能低字节
    enableData.append(static_cast<char>(0x00)); // 曝光使能高字节
    enableData.append(static_cast<char>(0x00)); // 曝光模式低字节
    enableData.append(static_cast<char>(0x00)); // 曝光模式高字节

    QByteArray enableResponse = executeCommand(0x1B, enableData, 1000);

    if (enableResponse.isEmpty()) {
        qWarning() << "曝光使能命令失败";
        return false;
    }

    qDebug() << "曝光使能成功";
    QThread::msleep(200); // 等待设备稳定

    return true;
}

// 提取发送开始曝光命令方法
bool XRayProtocol::sendStartExposureCommand(quint8 &exposureStep)
{
    qDebug() << "发送开始曝光命令...";

    QByteArray startData;
    startData.append(static_cast<char>(0x01)); // 曝光使能=1
    startData.append(static_cast<char>(0x00));
    startData.append(static_cast<char>(0x01)); // 曝光模式=1（开始）
    startData.append(static_cast<char>(0x00));

    QByteArray startResponse = executeCommand(0x1B, startData, 1000);

    if (startResponse.isEmpty() || startResponse.size() < 29) {
        qWarning() << "开始曝光命令无响应或响应不完整";
        return false;
    }

    exposureStep = static_cast<quint8>(startResponse[24]);
    qDebug() << "曝光处理步骤:" << exposureStep;

    return true;
}

bool XRayProtocol::startExposure()
{
    QMutexLocker locker(&m_serialMutex);
    qDebug() << "=== 开始曝光流程 ===";

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

    // 启动定时器
    if (m_exposureTimer) {
        m_exposureTimer->disconnect();
        connect(m_exposureTimer, &QTimer::timeout, this, &XRayProtocol::onExposureTimeout);
        m_exposureTimer->start(100);
    }

    return true;
}

bool XRayProtocol::stopExposure()
{
    QMutexLocker locker(&m_serialMutex);

    if (!isConnected()) {
        return true;
    }

    if (m_deviceStatus != DEVICE_EXPOSING) {
        return true;
    }

    // 发送预定义的曝光停止命令

    QByteArray response = executeCommand(0x1B, QByteArray::fromHex("01000000"), 1000);

    m_exposureTimer->stop();
    m_deviceStatus = DEVICE_READY;

    emit exposureStopped();
    emit deviceStatusChanged(m_deviceStatus);

    return true;
}

bool XRayProtocol::resetDevice()
{
    qDebug() << "=== 重置X光设备 ===";

    // 1. 发送停止曝光命令
    QByteArray stopData;
    QDataStream stopStream(&stopData, QIODevice::WriteOnly);
    stopStream.setByteOrder(QDataStream::LittleEndian);
    stopStream << (quint16) 1; // 使能=1
    stopStream << (quint16) 0; // 模式=0

    executeCommand(0x1B, stopData, 500);
    QThread::msleep(200);

    // 2. 清除错误标志
    if (!clearFaults()) {
        qWarning() << "清除错误标志失败";
        return false;
    }

    // 3. 等待设备稳定
    QThread::msleep(500);

    // 4. 验证设备状态
    ExposureStatus status;
    if (checkExposureStatus(status)) {
        if (status.exposureEnable != 0xFFFF && status.exposureStep != 9) {
            qDebug() << "✅ 设备重置成功";
            return true;
        }
    }

    qWarning() << "设备重置失败";
    return false;
}
QString XRayProtocol::portName() const
{
    return m_portName;
}

bool XRayProtocol::checkExposureReady()
{
    QByteArray response = executeCommand(0x1B, QByteArray(), 1000);

    if (response.isEmpty()) {
        qWarning() << "曝光查询无响应";
        return false;
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

        // 使用修改后的executeCommand，它会自动使用波形模板
        QByteArray response = executeCommand(waveformType, requestData, 2000);

        if (response.isEmpty()) {
            break;
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
                qWarning() << "Packet index mismatch";
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
        qWarning() << "数据包太小，无法验证CRC";
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

QByteArray XRayProtocol::executeCommand(quint16 cmdCode, const QByteArray &data, int responseTimeout)
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        qWarning() << "串口未打开";
        return QByteArray();
    }

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
            qWarning() << "曝光命令数据格式错误";
            return QByteArray();
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
            qWarning() << "曝光参数命令数据大小错误";
            return QByteArray();
        }
        break;
    default:
        // 其他命令使用通用构建
        command = buildCommand(cmdCode, 0x00, data);
        break;
    }

    if (command.isEmpty()) {
        qWarning() << "构建命令失败";
        return QByteArray();
    }

    // 发送命令
    qint64 bytesWritten = m_serialPort->write(command);
    if (bytesWritten == -1 || bytesWritten != command.size()) {
        qWarning() << "写入失败";
        return QByteArray();
    }

    if (!m_serialPort->waitForBytesWritten(100)) {
        qWarning() << "写入超时";
        return QByteArray();
    }

    QElapsedTimer timer;
    timer.start();
    QByteArray responseBuffer;

    while (timer.elapsed() < responseTimeout) {
        if (m_serialPort->waitForReadyRead(10)) {
            responseBuffer.append(m_serialPort->readAll());

            // 调试输出接收到的数据
            if (!responseBuffer.isEmpty()) {
                qDebug() << "接收到数据，当前缓冲区大小:" << responseBuffer.size();
            }

            // 尝试提取完整包
            while (true) {
                QByteArray completePacket = extractCompletePacket(responseBuffer);
                if (completePacket.isEmpty()) {
                    break; // 没有完整包，继续等待
                }

                if (verifyCRC(completePacket)) {
                    // 验证命令码
                    if (completePacket.size() >= 6) {
                        quint16 respCmdCode = static_cast<quint8>(completePacket[4])
                                              | (static_cast<quint8>(completePacket[5]) << 8);

                        if (respCmdCode == cmdCode) {
                            if (cmdCode == 0x1B) {
                                handleExposureResponse(completePacket);
                            }
                            return completePacket;
                        } else {
                            qDebug() << "收到其他命令的响应，命令码: 0x"
                                     << QString::number(respCmdCode, 16).toUpper() << "，期望: 0x"
                                     << QString::number(cmdCode, 16).toUpper();
                        }
                    }
                } else {
                    qWarning() << "CRC校验失败的命令响应";
                }
            }
        }
    }

    // 超时后，记录缓冲区中的数据
    if (!responseBuffer.isEmpty()) {
        qWarning() << "超时时缓冲区中的数据大小:" << responseBuffer.size()
                   << "，内容:" << responseBuffer.toHex(' ').toUpper();
    }

    return QByteArray();
}

QByteArray XRayProtocol::getWaveformCommandTemplate(quint16 waveformType, const QByteArray &data)
{
    qDebug() << "获取波形命令模板，类型: 0x" << QString::number(waveformType, 16).toUpper()
             << "数据大小:" << data.size() << "字节";

    // 验证数据格式：分包序号(2字节) + 分包长度(2字节)
    if (data.size() < 4) {
        qWarning() << "波形命令数据格式错误，需要至少4字节(分包序号+分包长度)";
        return QByteArray();
    }

    // 使用标准命令构建函数
    // 根据协议，波形命令是读操作，OPERATE=0x00
    QByteArray command = buildCommand(waveformType, 0x00, data);

    if (command.isEmpty()) {
        qWarning() << "构建波形命令失败";
    } else {
        qDebug() << "波形命令构建完成，长度:" << command.size() << "字节";
    }

    return command;
}

QByteArray XRayProtocol::extractCompletePacket(QByteArray &buffer)
{
    // 寻找包头 (0x55 0xAA)
    int packetStart = -1;
    for (int i = 0; i <= buffer.size() - 2; i++) {
        if (static_cast<quint8>(buffer[i]) == 0x55 && static_cast<quint8>(buffer[i + 1]) == 0xAA) {
            packetStart = i;
            break;
        }
    }

    // 没有找到包头
    if (packetStart == -1) {
        // 保留最近的数据（最多200字节），防止协议粘包
        if (buffer.size() > 200) {
            buffer = buffer.right(200);
        }
        return QByteArray();
    }

    // 移除包头之前的数据
    if (packetStart > 0) {
        buffer.remove(0, packetStart);
        qDebug() << "移除无效包头前数据:" << packetStart << "字节";
    }

    // 检查是否有足够的数据获取包长度（至少需要4字节：包头2 + 长度2）
    if (buffer.size() < 4) {
        return QByteArray(); // 数据不足，等待更多数据
    }

    // 解析包长度（小端序）
    // 协议格式：HEAD0 HEAD1 LEN_L LEN_H ...
    quint16 packetLength = static_cast<quint8>(buffer[2]) | (static_cast<quint8>(buffer[3]) << 8);

    // 验证包长度合理性
    const quint16 MIN_PACKET_SIZE = 16; // 最小包：2包头+2长度+2命令+2操作+4TX+4RX+2CRC
    const quint16 MAX_PACKET_SIZE = 1024;

    if (packetLength < MIN_PACKET_SIZE || packetLength > MAX_PACKET_SIZE) {
        qWarning() << "包长度无效:" << packetLength << "，丢弃第一个字节并重试";
        // 丢弃第一个字节（可能是损坏的数据），然后重试
        buffer.remove(0, 1);
        return extractCompletePacket(buffer);
    }

    // 检查是否收到完整包
    if (buffer.size() >= packetLength) {
        // 提取完整包
        QByteArray packet = buffer.left(packetLength);

        // 验证CRC（可选）
        if (verifyCRC(packet)) {
            // 从缓冲区移除已处理的数据
            buffer.remove(0, packetLength);
            qDebug() << "提取完整包，长度:" << packetLength << "，缓冲区剩余:" << buffer.size()
                     << "字节";
            return packet;
        } else {
            qWarning() << "CRC校验失败，丢弃包";
            // CRC失败，丢弃整个包
            buffer.remove(0, packetLength);
            return QByteArray();
        }
    }

    // 包不完整
    qDebug() << "包不完整，需要" << packetLength << "字节，当前" << buffer.size() << "字节";
    return QByteArray();
}

void XRayProtocol::handleExposureResponse(const QByteArray &response)
{
    if (response.size() < 29) {
        qWarning() << "Invalid exposure response size:" << response.size();
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

    // 更新设备状态
    if (exp_en == 1) {
        m_deviceStatus = DEVICE_EXPOSING;
        m_coolingRemaining = 0;
    } else {
        if (exp_cooling_time > 0) {
            m_deviceStatus = DEVICE_COOLING;
            m_coolingRemaining = exp_cooling_time;
        } else {
            m_deviceStatus = DEVICE_READY;
            m_coolingRemaining = 0;
        }
    }

    // 检查错误
    if (low_battery_flag == 1) {
        m_lastError = XRAY_ERROR_LOW_BATTERY;
        emit errorOccurred(XRAY_ERROR_LOW_BATTERY, "低电量");
    }

    if (hard_check_result == 1 || hard_check_result == 2) {
        m_lastError = XRAY_ERROR_HARDWARE_CHECK;
        emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK, "硬件检测异常");
    }

    if (exp_step == 9) {
        m_lastError = XRAY_ERROR_EXPOSURE_TIMEOUT;
        emit exposureError(XRAY_ERROR_EXPOSURE_TIMEOUT);
    }

    // emit deviceStatusChanged(m_deviceStatus);
}

QString XRayProtocol::parseSerialNumber(const QByteArray &data)
{
    if (data.isEmpty() || data.size() < 28) {
        qDebug() << "Invalid data for serial number parsing, size:" << data.size();
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

        qDebug() << "原始序列号HEX:" << serialNumber;

        // 如果需要格式化为更可读的格式
        if (serialNumber.length() >= 24) {
            // 格式化为: XXXX-XXXX-XXXX
            QString formatted = QString("%1-%2-%3")
                                    .arg(serialNumber.mid(0, 8))
                                    .arg(serialNumber.mid(8, 8))
                                    .arg(serialNumber.mid(16, 8));
            qDebug() << "格式化序列号:" << formatted;
            return formatted;
        }

        return serialNumber;
    }

    qDebug() << "Failed to parse serial number from data";
    return QString();
}

void XRayProtocol::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
        qWarning() << "Serial port error:" << error << m_serialPort->errorString();

        // 根据错误类型处理
        switch (error) {
        case QSerialPort::PermissionError:
            emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK, "串口访问被拒绝");
            break;
        case QSerialPort::DeviceNotFoundError:
            emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK, "设备未找到");
            break;
        case QSerialPort::ResourceError:
            emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK, "设备被移除");
            closeSerialPort();
            break;
        default:
            emit errorOccurred(XRAY_ERROR_HARDWARE_CHECK, "串口通信错误");
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
        executeCommand(0x1B, m_exposureStopData, 500);
        m_deviceStatus = DEVICE_READY;
        emit exposureStopped();
        emit deviceStatusChanged(m_deviceStatus);
    }
    // else if (elapsed >= 1800 && !m_over18SecSent) {
    //     m_over18SecSent = true; // 标记已发送
    //     // emit exposureWarning("曝光时间已超过1.8秒");
    //     //buchuguang
    //     executeCommand(0x1B, m_exposureStartData, 100);
    // }
    else {
        // 正常曝光中，继续发送曝光命令
        executeCommand(0x1B, m_exposureStartData, 100);
    }
}

void XRayProtocol::updateDeviceStatus()
{
    if (!isConnected()) {
        return;
    }

    // 定期查询设备状态
    QByteArray response = executeCommand(0x1B, QByteArray(), 2000);

    if (!response.isEmpty()) {
        handleExposureResponse(response);
    }

    // 更新温度等信息
    ADCValues adcValues = getADCValues();
    emit adcValuesUpdated(adcValues);
    emit temperatureUpdated(adcValues.device_temp, adcValues.mcu_temp);
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
        qDebug() << "ADC值解析:";
        qDebug() << "  充电电流:" << values.charge_current;
        qDebug() << "  电池分压:" << values.battery_div;
        qDebug() << "  KV参考:" << values.kv_ref;
        qDebug() << "  mA:" << values.ma;
        qDebug() << "  灯丝电流:" << values.fill;
        qDebug() << "  KV:" << values.kv;
        qDebug() << "  MCU温度:" << values.mcu_temp << "°C";
        qDebug() << "  MCU参考电压:" << values.mcu_vref << "V";
        qDebug() << "  设备温度:" << values.device_temp << "°C";
    } else {
        qWarning() << "ADC数据包太小:" << data.size();
    }

    return values;
}

QString XRayProtocol::parseVersion(const QByteArray &data)
{
    if (data.size() < 17) {
        qWarning() << "版本数据包太小:" << data.size();
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

    qDebug() << "版本号解析:" << version;

    return version;
}

void XRayProtocol::logCommunication(const QString &direction, const QByteArray &data)
{
#ifdef QT_DEBUG
    QString hexString = data.toHex(' ').toUpper();
    qDebug() << QString("[%1] %2").arg(direction).arg(hexString);
#endif
}

// 错误处理
bool XRayProtocol::clearErrorFlags()
{
    // 使用预定义的错误清除命令
    QByteArray response = executeCommand(0x1D, QByteArray::fromHex("01000000"), 500);
    return !response.isEmpty();
}

XRayErrorCode XRayProtocol::getLastError() const
{
    return m_lastError;
}

QString XRayProtocol::getErrorString(XRayErrorCode error) const
{
    static QMap<XRayErrorCode, QString> errorStrings = {{XRAY_SUCCESS, "成功"},
                                                        {XRAY_ERROR_OVER_KV, "KV过高"},
                                                        {XRAY_ERROR_LOW_MA, "mA过低"},
                                                        {XRAY_ERROR_OVER_MA, "mA过高"},
                                                        {XRAY_ERROR_OVER_CURRENT, "过流"},
                                                        {XRAY_ERROR_LOW_FIL_I, "灯丝电流过低"},
                                                        {XRAY_ERROR_OVER_KV_S, "KV设置过高"},
                                                        {XRAY_ERROR_KEY_RELEASE, "按键释放错误"},
                                                        {XRAY_ERROR_KEY_NO_RELEASE,
                                                         "按键不释放错误"},
                                                        {XRAY_ERROR_LOW_BATTERY, "低电量"},
                                                        {XRAY_ERROR_HARDWARE_CHECK, "硬件检测异常"},
                                                        {XRAY_ERROR_COOLING, "冷却中"},
                                                        {XRAY_ERROR_EXPOSURE_TIMEOUT, "曝光超时"}};

    return errorStrings.value(error, "未知错误");
}

// 设备状态查询
XRayDeviceStatus XRayProtocol::getDeviceStatus() const
{
    return m_deviceStatus;
}

int XRayProtocol::getCoolingRemainingTime() const
{
    return m_coolingRemaining;
}
