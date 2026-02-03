#include "XDSerialCommunication.h"

// 初始化静态成员
SerialCommunication* SerialCommunication::m_instance = nullptr;
QMutex SerialCommunication::m_mutex;

SerialCommunication::SerialCommunication(QObject* parent)
    : QObject(parent)
    , m_serialPort(nullptr)
    , m_isReceiving(false)
{
    // 初始化串口
    m_serialPort = new QSerialPort(this);

    // 设置默认参数：3.3V TTL，921600-8-N-1
    m_serialPort->setBaudRate(921600);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // 连接信号槽
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialCommunication::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            emit errorOccurred(m_serialPort->errorString());
        }
    });

    // 初始化接收定时器
    m_receiveTimer = new QTimer(this);
    m_receiveTimer->setSingleShot(true);
    m_receiveTimer->setInterval(100); // 100ms超时
    connect(m_receiveTimer, &QTimer::timeout, this, &SerialCommunication::processReceivedData);
}

SerialCommunication::~SerialCommunication()
{
    closeSerialPort();
    delete m_serialPort;
}

SerialCommunication* SerialCommunication::instance()
{
    QMutexLocker locker(&m_mutex);
    if (!m_instance) {
        m_instance = new SerialCommunication();
    }
    return m_instance;
}

bool SerialCommunication::openSerialPort(const QString& portName)
{
    QMutexLocker locker(&m_mutex);

    if (m_serialPort->isOpen()) {
        closeSerialPort();
    }

    m_serialPort->setPortName(portName);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        emit connectionStatusChanged(true);
        return true;
    }

    emit errorOccurred(QString("Failed to open port %1: %2")
                      .arg(portName, m_serialPort->errorString()));
    return false;
}

void SerialCommunication::closeSerialPort()
{
    QMutexLocker locker(&m_mutex);

    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit connectionStatusChanged(false);
    }
}

bool SerialCommunication::isOpen() const
{
    return m_serialPort->isOpen();
}

uint16_t SerialCommunication::calculateCRC16(const QByteArray& data)
{
    uint16_t crc = 0xFFFF;

    for (char byte : data) {
        crc ^= (uint8_t)byte;

        for (int i = 0; i < 8; i++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

QByteArray SerialCommunication::buildPacket(uint16_t cmd, uint8_t operate, const QByteArray& data)
{
    QByteArray packet;

    // 基本帧长度 = 固定头部(18字节) + 数据长度
    uint16_t frameLength = 18 + data.size();

    // 添加头部
    packet.append(HEAD0);
    packet.append(HEAD1);

    // 添加长度（小端）
    packet.append((char)(frameLength & 0xFF));
    packet.append((char)((frameLength >> 8) & 0xFF));

    // 添加命令（小端）
    packet.append((char)(cmd & 0xFF));
    packet.append((char)((cmd >> 8) & 0xFF));

    // 添加操作码
    packet.append(operate);

    // 保留字节
    packet.append((char)0x00);

    // 添加TX_UID（小端）
    for (int i = 0; i < 4; i++) {
        packet.append((char)((DEFAULT_TX_UID >> (i * 8)) & 0xFF));
    }

    // 添加RX_UID（小端）
    for (int i = 0; i < 4; i++) {
        packet.append((char)((DEFAULT_RX_UID >> (i * 8)) & 0xFF));
    }

    // 添加数据
    packet.append(data);

    // 计算CRC（不包括CRC本身）
    uint16_t crc = calculateCRC16(packet);

    // 添加CRC（小端）
    packet.append((char)(crc & 0xFF));
    packet.append((char)((crc >> 8) & 0xFF));

    return packet;
}

bool SerialCommunication::sendCommand(uint16_t cmd, uint8_t operate, const QByteArray& data)
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        emit errorOccurred("Serial port is not open");
        return false;
    }

    QByteArray packet = buildPacket(cmd, operate, data);

    qint64 bytesWritten = m_serialPort->write(packet);

    if (bytesWritten == packet.size()) {
        m_serialPort->flush();
        qDebug() << "Command sent:" << QString("0x%1").arg(cmd, 4, 16, QChar('0'));
        return true;
    }

    emit errorOccurred(QString("Failed to write data: %1").arg(m_serialPort->errorString()));
    return false;
}

// 具体命令实现
bool SerialCommunication::readMCUSN()
{
    return sendCommand(CMD_MCU_SN, OPERATE_READ);
}

bool SerialCommunication::readSoftwareVersion()
{
    return sendCommand(CMD_SOFTWARE_VERSION, OPERATE_READ);
}

bool SerialCommunication::readADC()
{
    return sendCommand(CMD_ADC, OPERATE_READ);
}

bool SerialCommunication::readKVWaveform(uint16_t packetSeq, uint16_t packetLen)
{
    QByteArray data;
    // 添加分包序号（小端）
    data.append((char)(packetSeq & 0xFF));
    data.append((char)((packetSeq >> 8) & 0xFF));
    // 添加分包长度（小端）
    data.append((char)(packetLen & 0xFF));
    data.append((char)((packetLen >> 8) & 0xFF));

    return sendCommand(CMD_KV_WAVEFORM, OPERATE_READ, data);
}

bool SerialCommunication::readMAWaveform(uint16_t packetSeq, uint16_t packetLen)
{
    QByteArray data;
    data.append((char)(packetSeq & 0xFF));
    data.append((char)((packetSeq >> 8) & 0xFF));
    data.append((char)(packetLen & 0xFF));
    data.append((char)((packetLen >> 8) & 0xFF));

    return sendCommand(CMD_MA_WAVEFORM, OPERATE_READ, data);
}

bool SerialCommunication::readFIWaveform(uint16_t packetSeq, uint16_t packetLen)
{
    QByteArray data;
    data.append((char)(packetSeq & 0xFF));
    data.append((char)((packetSeq >> 8) & 0xFF));
    data.append((char)(packetLen & 0xFF));
    data.append((char)((packetLen >> 8) & 0xFF));

    return sendCommand(CMD_FI_WAVEFORM, OPERATE_READ, data);
}

bool SerialCommunication::readOverCWaveform(uint16_t packetSeq, uint16_t packetLen)
{
    QByteArray data;
    data.append((char)(packetSeq & 0xFF));
    data.append((char)((packetSeq >> 8) & 0xFF));
    data.append((char)(packetLen & 0xFF));
    data.append((char)((packetLen >> 8) & 0xFF));

    return sendCommand(CMD_OVER_C_WAVEFORM, OPERATE_READ, data);
}

bool SerialCommunication::readErrorInfo()
{
    return sendCommand(CMD_ERROR_INFO, OPERATE_READ);
}

bool SerialCommunication::sendExposureCommand2(bool enable, bool mode)
{
    QByteArray data;
    // 曝光使能（小端）
    data.append((char)(enable ? 0x01 : 0x00));
    data.append((char)0x00);
    // 曝光模式（小端）
    data.append((char)(mode ? 0x01 : 0x00));
    data.append((char)0x00);

    return sendCommand(CMD_EXPOSURE_CMD2, OPERATE_WRITE, data);
}

bool SerialCommunication::setExposureParams(const QByteArray& paramsData)
{
    if (paramsData.size() != 36) { // 4字节 * 9个参数 = 36字节
        emit errorOccurred("Invalid exposure parameters size");
        return false;
    }

    return sendCommand(CMD_EXPOSURE_PARAMS, OPERATE_WRITE, paramsData);
}

bool SerialCommunication::readExposureErrorFlag()
{
    return sendCommand(CMD_EXPOSURE_ERROR_FLAG, OPERATE_READ);
}

bool SerialCommunication::clearExposureErrorFlag()
{
    return sendCommand(CMD_EXPOSURE_ERROR_FLAG, OPERATE_WRITE);
}

void SerialCommunication::handleReadyRead()
{
    m_receivedData.append(m_serialPort->readAll());

    if (!m_receiveTimer->isActive()) {
        m_receiveTimer->start();
    }
}

void SerialCommunication::processReceivedData()
{
    // 查找完整的数据包
    while (m_receivedData.size() >= 20) { // 最小数据包长度
        // 查找帧头
        int startIndex = m_receivedData.indexOf(QByteArray::fromHex("55aa"));
        if (startIndex == -1) {
            m_receivedData.clear();
            return;
        }

        // 移除无效数据
        if (startIndex > 0) {
            m_receivedData.remove(0, startIndex);
        }

        // 检查是否有足够的数据
        if (m_receivedData.size() < 4) {
            return;
        }

        // 获取帧长度
        uint16_t frameLength = (uint8_t)m_receivedData.at(2) |
                              ((uint8_t)m_receivedData.at(3) << 8);

        // 检查是否收到完整的数据包
        if (m_receivedData.size() < frameLength) {
            return; // 等待更多数据
        }

        // 提取完整数据包
        QByteArray packet = m_receivedData.left(frameLength);
        m_receivedData.remove(0, frameLength);

        // 验证数据包
        parseResponse(packet);
    }
}

bool SerialCommunication::parseResponse(const QByteArray& data)
{
    // 验证最小长度
    if (data.size() < 20) {
        emit errorOccurred("Invalid packet length");
        return false;
    }

    // 验证帧头
    if ((uint8_t)data.at(0) != HEAD0 || (uint8_t)data.at(1) != HEAD1) {
        emit errorOccurred("Invalid packet header");
        return false;
    }

    // 验证CRC
    uint16_t receivedCRC = (uint8_t)data.at(data.size() - 2) |
                          ((uint8_t)data.at(data.size() - 1) << 8);
    uint16_t calculatedCRC = calculateCRC16(data.left(data.size() - 2));

    if (receivedCRC != calculatedCRC) {
        emit errorOccurred("CRC check failed");
        return false;
    }

    // 提取命令和操作码
    uint16_t cmd = (uint8_t)data.at(4) | ((uint8_t)data.at(5) << 8);
    uint8_t operate = (uint8_t)data.at(6);

    // 确保是回复包
    if (operate != OPERATE_REPLY) {
        return false;
    }

    // 提取数据部分（从第16字节开始，到CRC前结束）
    QByteArray payload = data.mid(16, data.size() - 18);

    // 根据命令解析数据
    switch (cmd) {
    case CMD_MCU_SN:
        parseMCUSN(payload);
        break;
    case CMD_SOFTWARE_VERSION:
        parseSoftwareVersion(payload);
        break;
    case CMD_ADC:
        parseADC(payload);
        break;
    case CMD_KV_WAVEFORM:
    case CMD_MA_WAVEFORM:
    case CMD_FI_WAVEFORM:
    case CMD_OVER_C_WAVEFORM:
        parseWaveform(cmd, payload);
        break;
    case CMD_ERROR_INFO:
        parseErrorInfo(payload);
        break;
    case CMD_EXPOSURE_CMD2:
        parseExposureStatus(payload);
        break;
    case CMD_EXPOSURE_ERROR_FLAG:
        parseExposureErrorFlag(payload);
        break;
    default:
        qDebug() << "Unknown command received:" << QString("0x%1").arg(cmd, 4, 16, QChar('0'));
        break;
    }

    emit dataReceived(data);
    return true;
}

void SerialCommunication::parseMCUSN(const QByteArray& data)
{
    if (data.size() < 12) {
        emit errorOccurred("Invalid MCU SN data");
        return;
    }

    MCUSNData snData;
    snData.uid2 = (uint8_t)data.at(0) | ((uint8_t)data.at(1) << 8) |
                 ((uint8_t)data.at(2) << 16) | ((uint8_t)data.at(3) << 24);
    snData.uid1 = (uint8_t)data.at(4) | ((uint8_t)data.at(5) << 8) |
                 ((uint8_t)data.at(6) << 16) | ((uint8_t)data.at(7) << 24);
    snData.uid0 = (uint8_t)data.at(8) | ((uint8_t)data.at(9) << 8) |
                 ((uint8_t)data.at(10) << 16) | ((uint8_t)data.at(11) << 24);

    emit mcuSNReceived(snData);
}

void SerialCommunication::parseSoftwareVersion(const QByteArray& data)
{
    // 查找字符串结束符
    int endIndex = data.indexOf('\0');
    if (endIndex == -1) {
        endIndex = data.size();
    }

    QString version = QString::fromLatin1(data.constData(), endIndex);
    emit softwareVersionReceived(version);
}

void SerialCommunication::parseADC(const QByteArray& data)
{
    if (data.size() < 28) { // 14个2字节 + 3个4字节浮点数
        emit errorOccurred("Invalid ADC data");
        return;
    }

    ADCData adcData;

    // 解析ADC值（小端）
    int index = 0;
    adcData.chargeCurrent = (uint8_t)data.at(index) | ((uint8_t)data.at(index + 1) << 8);
    index += 2;
    adcData.ovc2 = (uint8_t)data.at(index) | ((uint8_t)data.at(index + 1) << 8);
    index += 2;
    adcData.batteryVoltage = (uint8_t)data.at(index) | ((uint8_t)data.at(index + 1) << 8);
    index += 2;
    adcData.kvRef = (uint8_t)data.at(index) | ((uint8_t)data.at(index + 1) << 8);
    index += 2;
    adcData.ma = (uint8_t)data.at(index) | ((uint8_t)data.at(index + 1) << 8);
    index += 2;
    adcData.fill = (uint8_t)data.at(index) | ((uint8_t)data.at(index + 1) << 8);
    index += 2;
    adcData.kv = (uint8_t)data.at(index) | ((uint8_t)data.at(index + 1) << 8);
    index += 2;

    // 解析浮点数（小端）
    memcpy(&adcData.mcuTemperature, data.constData() + index, sizeof(float));
    index += sizeof(float);
    memcpy(&adcData.mcuVref, data.constData() + index, sizeof(float));
    index += sizeof(float);
    memcpy(&adcData.deviceTemperature, data.constData() + index, sizeof(float));

    emit adcDataReceived(adcData);
}

void SerialCommunication::parseWaveform(uint16_t cmd, const QByteArray& data)
{
    if (data.size() < 4) {
        emit errorOccurred("Invalid waveform data");
        return;
    }

    WaveformData waveform;
    waveform.packetSeq = (uint8_t)data.at(0) | ((uint8_t)data.at(1) << 8);
    waveform.totalBytes = (uint8_t)data.at(2) | ((uint8_t)data.at(3) << 8);

    // 解析波形数据（每2字节一个ADC值，小端）
    int adcCount = (data.size() - 4) / 2;
    for (int i = 0; i < adcCount; i++) {
        int offset = 4 + i * 2;
        uint16_t adcValue = (uint8_t)data.at(offset) | ((uint8_t)data.at(offset + 1) << 8);
        waveform.adcValues.append(adcValue);
    }

    emit waveformDataReceived(cmd, waveform);
}

void SerialCommunication::parseErrorInfo(const QByteArray& data)
{
    if (data.size() < 90) {
        emit errorOccurred("Invalid error info data");
        return;
    }

    ErrorInfoData errorData;
    int index = 0;

    // 解析4字节整数（小端）
    memcpy(&errorData.fillData0, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.errorCode, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.exposureCount, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.totalExposureCount, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.totalExposureTime, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.noneErrorCount, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.overKVErrorCount, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.lowMAErrorCount1, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.overMAErrorCount, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.lowMAErrorCount2, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.overCurrentErrorCount, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.lowFILIErrorCount, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.overKVSErrorCount, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.keyReleaseErrorCount, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);
    memcpy(&errorData.keyNotReleaseErrorCount, data.constData() + index, sizeof(uint32_t));
    index += sizeof(uint32_t);

    // 解析浮点数
    memcpy(&errorData.lastErrorBatteryVoltage, data.constData() + index, sizeof(float));
    index += sizeof(float);
    memcpy(&errorData.lastErrorDeviceTemperature, data.constData() + index, sizeof(float));
    index += sizeof(float);

    // 解析2字节整数
    memcpy(&errorData.fillData1, data.constData() + index, sizeof(uint16_t));

    emit errorInfoReceived(errorData);
}

void SerialCommunication::parseExposureStatus(const QByteArray& data)
{
    if (data.size() < 11) {
        emit errorOccurred("Invalid exposure status data");
        return;
    }

    ExposureStatusData status;

    // 解析曝光使能和模式（小端）
    status.enable = (uint8_t)data.at(0) & 0x01;
    status.mode = (uint8_t)data.at(2) & 0x01;
    status.lowBattery = (uint8_t)data.at(4) & 0x01;
    status.hardwareStatus = (uint8_t)data.at(5);
    status.shutdownDelay = (uint8_t)data.at(6) | ((uint8_t)data.at(7) << 8);
    status.exposureCooldownTime = (uint8_t)data.at(8) | ((uint8_t)data.at(9) << 8);
    status.exposureStep = (uint8_t)data.at(10);

    emit exposureStatusReceived(status);
}

void SerialCommunication::parseExposureErrorFlag(const QByteArray& data)
{
    if (data.size() < 1) {
        emit errorOccurred("Invalid exposure error flag data");
        return;
    }

    bool errorFlag = (uint8_t)data.at(0) & 0x01;
    emit exposureErrorFlagReceived(errorFlag);
}
