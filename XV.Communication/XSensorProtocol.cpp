#include "XSensorProtocol.h"
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QSerialPortInfo>
#include <QThread>
#include <QTimer>

#include "XV.Tool/XDConfigFileManager.h"

// 静态命令数据初始化（保持不变）
QByteArray XSensorProtocol::Commands::F4_POWER_ON_3V3 = QByteArray::fromHex(
    "F4F40400000000000000"); //dui
QByteArray XSensorProtocol::Commands::F4_POWER_ON = QByteArray::fromHex(
    "F4F40600000000000000"); //dui
QByteArray XSensorProtocol::Commands::F4_POWER_OFF = QByteArray::fromHex(
    "F4F40100000000000000"); //dui
QByteArray XSensorProtocol::Commands::FA_FIND_DEVICE = QByteArray::fromHex(
    "FAFA0200000000000000"); //dui
QByteArray XSensorProtocol::Commands::F5_VOLTAGE_CONFIG = QByteArray::fromHex(
    "F5F50500000000000000");

QByteArray XSensorProtocol::Commands::F5_CONFIG = QByteArray::fromHex(
    "F5F501050000000000267FFF000001020A101A1810E0181B194C067C115C11DC0E820F200D5417001500142016C016"
    "C4");
QByteArray XSensorProtocol::Commands::F5_READ_CONFIG = QByteArray::fromHex(
    "F5F50100000000000026FFFF800081028A109A1890E4981B994C867C915C91DC8E828F208D5497009500942096C096"
    "C4");
QByteArray XSensorProtocol::Commands::F5_EXTERNAL_BIAL = QByteArray::fromHex(
    "F5F5050200000000000805DC057704B04E20");
QByteArray XSensorProtocol::Commands::F5_READ_EXTERNAL_BIAL = QByteArray::fromHex(
    "F5F50503000000000000");

QByteArray XSensorProtocol::Commands::F6_ENABLE_EXPOSE = QByteArray::fromHex(
    "F6F6010000001388000A0002000000642CB88028"); //5
QByteArray XSensorProtocol::Commands::F6_ENABLE_EXPOSE_02 = QByteArray::fromHex(
    "F6F6010000000000000A000200DB0F4907500000"); //30
QByteArray XSensorProtocol::Commands::F6_ENABLE_EXPOSE_CALI = QByteArray::fromHex(
    "F6F6010000001388000A0002000000642CB88028"); // 20.5
QByteArray XSensorProtocol::Commands::F6_ENABLE_EXPOSE_1MIN = QByteArray::fromHex(
    "F6F6010000000000000A0002000000C800007530"); //20.5

QByteArray XSensorProtocol::Commands::F8_REQUEST_IMAGE = QByteArray::fromHex(
    "F8F80100000000000000");
QByteArray XSensorProtocol::Commands::F8_TO_LOWER_POWER = QByteArray::fromHex(
    "F8F80200000000000000");

XSensorProtocol::XSensorProtocol(QObject* parent)
    : QObject(parent)
    , m_serialPort(nullptr)
    , m_deviceInitialized(false)
    , m_exposing(false)
    , m_poweredOn(false)
    , m_autoInitialized(false)
    , m_baudRate(2000000)
    , m_readTimeout(500)
    , m_writeTimeout(500)
    , m_echoTimeout(500)
    , m_exposureTimeout(30000)
    , m_isBusy(false)
    , m_cancelRequested(false)
{}

XSensorProtocol::~XSensorProtocol()
{
    closeSerialPort();
}

bool XSensorProtocol::initSerialPort(const QString& portName)
{
    QMutexLocker locker(&m_mutex);
    resetDeviceState();

    m_serialPort = new QSerialPort();

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(2000000);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        QThread::msleep(500);
        m_serialPort->clear();

        if (!echoSerialPort()) {
            qWarning() << tr("Echo test failed for port:") << portName;
            // 清理资源
            m_serialPort->close();
            delete m_serialPort;
            m_serialPort = nullptr;
            return false;
        }
    } else {
        qWarning() << tr("Failed to open port:") << portName;
        delete m_serialPort;
        m_serialPort = nullptr;
        return false;
    }

    return true;
}

bool XSensorProtocol::echoSerialPort()
{
    if (!m_serialPort || !m_serialPort->isOpen())
        return false;

    m_serialPort->clear();

    QThread::msleep(200);

    if (!powerOn()) {
        return false;
    }

    QThread::msleep(500);

    if (!getVersion())
        return false;

    m_deviceInitialized = true;

    powerOff();

    return true;
}

void XSensorProtocol::closeSerialPort()
{
    qInfo() << tr("=== Closing sensor serial port ===");

    m_exposing = false;
    m_poweredOn = false;
    m_isBusy = false;
    m_cancelRequested = true;

    try {
        m_isBusy = true;

        if (m_serialPort) {
            qWarning() << tr("Actively close port") << m_serialPort->portName();

            if (m_serialPort->isOpen()) {
                m_serialPort->close();
            }

            m_serialPort->deleteLater();
            m_serialPort = nullptr;
        }
    } catch (const std::exception& e) {
        m_isBusy = false;
        qCritical() << tr("Error in closeSerialPort:") << e.what();
        m_lastError = QString(tr("Close port error: %1")).arg(e.what());
        emit errorOccurred(m_lastError);
    }

    m_isBusy = false;
    resetDeviceState();
}

void XSensorProtocol::resetDeviceState()
{
    m_deviceInitialized = false;
    m_exposing = false;
    m_poweredOn = false;
    m_autoInitialized = false;
    m_isBusy = false;
    m_serialPort = nullptr;
}

QByteArray XSensorProtocol::buildCommand(const QByteArray& cmdData, char endChar1, char endChar2)
{
    if (cmdData.isEmpty()) {
        return QByteArray();
    }

    QByteArray fullCommand = cmdData;
    QByteArray crc = calculateCRC(cmdData);

    fullCommand.append(crc);
    fullCommand.append(endChar1);
    fullCommand.append(endChar2);

    return fullCommand;
}

bool XSensorProtocol::sendExposureF6(bool wait, bool isCalibration)
{
    if (wait) {
        QThread::msleep(1950);
    }
    QByteArray exposureCommand = isCalibration ? Commands::F6_ENABLE_EXPOSE_CALI
                                               : Commands::F6_ENABLE_EXPOSE;

    QByteArray response;
    if (sendCommand(exposureCommand)) {
        response = readResponse(21000);
    }

    bool exposureSuccess = false;
    if (response.size() >= 4 && static_cast<quint8>(response[0]) == 0xF6
        && static_cast<quint8>(response[1]) == 0xF6) {
        exposureSuccess = true;
        m_exposing = true;

        emit exposureF6Ready(m_serialPort->portName()); //kaishijishu
    } else {
        if (!response.isEmpty()) {
            qWarning() << "[" << m_serialPort->portName() << tr("] Response header: 0x")
                       << QString::number(static_cast<quint8>(response[0]), 16).toUpper() << "0x"
                       << QString::number(static_cast<quint8>(response[1]), 16).toUpper();
        }
        exposureSuccess = false;
    }

    if (!exposureSuccess && m_poweredOn) {
        powerOff();
    }

    return exposureSuccess;
}

void XSensorProtocol::acquireImage()
{
    if (m_isBusy) {
        m_lastError = tr("Device is busy");
        emit statusChanged(m_lastError);
        return;
    }

    if (!m_exposing) {
        m_lastError = tr("Device not connected or not exposing");
        emit statusChanged(m_lastError);
        return;
    }

    QMutexLocker locker(&m_mutex);

    m_isBusy = true;
    bool allGot = true;

    try {
        emit statusChanged(tr("Acquiring image..."));

        int cols = 352, rows = 425, bitsAllocated = 16;
        int rawImgTotalBytes = cols * rows * bitsAllocated / 8; // 299200 bytes

        // 协议参数
        int totalPackageSize = 512;                                         // 每包总大小
        int headerSize = 10;                                                // 帧头大小
        int crcSize = 2;                                                    // CRC校验大小
        int validBytesPerPackage = totalPackageSize - headerSize - crcSize; // 500字节

        int totalPackages = (rawImgTotalBytes + validBytesPerPackage - 1)
                            / validBytesPerPackage; // 599包

        // qDebug() << "[" << m_serialPort->portName() << "] Image size:" << rawImgTotalBytes
        //          << "bytes";
        // qDebug() << "[" << m_serialPort->portName() << "] Package structure:"
        //          << "Header=" << headerSize << "bytes,"
        //          << "Data=" << validBytesPerPackage << "bytes,"
        //          << "CRC=" << crcSize << "bytes";
        // qDebug() << "[" << m_serialPort->portName() << "] Total packages:" << totalPackages;

        // 分配接收缓冲区
        QByteArray receivedDataBytes(rawImgTotalBytes, 0);
        bool imgRetrieved = false;
        QString errorCode = "";

        try {
            QElapsedTimer sw;
            sw.start();

            int realGetBags = 1;
            int currentOffset = 0; // 当前写入偏移量

            for (int i = 0; i < totalPackages; i++) {
                QByteArray rd = retrieveImageByte(errorCode, realGetBags, i);

                if (!rd.isEmpty() && rd.size() >= totalPackageSize) {
                    // 正确提取有效数据：去掉前10字节帧头，去掉最后2字节CRC，取中间的500字节
                    // rd[10] 到 rd[509] 是有效数据（索引从0开始）
                    int dataStartIndex = headerSize;               // 10
                    int dataEndIndex = totalPackageSize - crcSize; // 510
                    int dataSize = dataEndIndex - dataStartIndex;  // 500字节

                    // 确保数据大小正确
                    if (rd.size() >= dataEndIndex) {
                        QByteArray validData = rd.mid(dataStartIndex, dataSize);

                        // 计算实际可复制的数据大小（最后一包可能不足500字节）
                        int copySize = qMin(validData.size(), rawImgTotalBytes - currentOffset);

                        if (copySize > 0) {
                            // 按顺序写入缓冲区
                            memcpy(receivedDataBytes.data() + currentOffset,
                                   validData.constData(),
                                   copySize);

                            currentOffset += copySize;
                            realGetBags++;

                            // 更新进度
                            // int progress = ((i + 1) * 100) / totalPackages;
                            // emit statusChanged(tr("Acquiring image... %1%").arg(progress));
                        }
                    } else {
                        qWarning() << "[" << m_serialPort->portName() << "] Package" << (i + 1)
                                   << "size too small:" << rd.size();
                        allGot = false;
                        break;
                    }
                } else {
                    qWarning() << "[" << m_serialPort->portName() << tr("] Failed to get package")
                               << (i + 1) << ":" << errorCode;
                    allGot = false;
                    break;
                }
            }

            if (allGot && currentOffset == rawImgTotalBytes) {
                imgRetrieved = true;
                qDebug() << "[" << m_serialPort->portName() << "] Image acquisition completed in"
                         << sw.elapsed() << "ms"
                         << "total bytes:" << currentOffset;

                // 可选：验证CRC
                // 如果要验证CRC，需要在接收时保存每个包的CRC值

            } else {
                qWarning() << "[" << m_serialPort->portName()
                           << "] Image acquisition incomplete:" << currentOffset << "/"
                           << rawImgTotalBytes << "bytes";
                allGot = false;
            }

        } catch (const std::exception& ex) {
            qCritical() << "[" << m_serialPort->portName() << tr("] Image acquisition error:")
                        << ex.what();
            allGot = false;
        }

        if (imgRetrieved && !receivedDataBytes.isEmpty()) {
            QByteArray processedImage = processImageData(receivedDataBytes, cols, rows);

            if (!processedImage.isEmpty()) {
                emit imageReady(processedImage);
                emit statusChanged(tr("Image acquired successfully"));
                qDebug() << "[" << m_serialPort->portName()
                         << "] Image processing successful, output size:" << processedImage.size();
            } else {
                allGot = false;
                qWarning() << "[" << m_serialPort->portName() << tr("] Image processing failed");
            }
        } else {
            allGot = false;
        }

    } catch (const std::exception& ex) {
        qCritical() << "[" << m_serialPort->portName() << tr("] AcquireImage exception:")
                    << ex.what();
        allGot = false;
    }

    if (!allGot) {
        qWarning() << "[" << m_serialPort->portName()
                   << tr("] Image acquisition failed, cleaning up...");
        if (m_serialPort && m_poweredOn) {
            powerOff();
        }
    } else {
        qDebug() << "[" << m_serialPort->portName() << "] Image acquisition successful";
    }

    m_isBusy = false;
    m_exposing = false;

    cleanupAfterImageAcquisition(allGot);
}

QByteArray XSensorProtocol::retrieveImageByte(QString& errorCode, int& realGetBags, int packageIndex)
{
    QElapsedTimer timer;
    timer.start();

    m_serialPort->clear(QSerialPort::Input);
    QThread::msleep(10);

    if (!sendCommand(Commands::F8_REQUEST_IMAGE)) {
        errorCode = QString(tr("Failed to request package %1")).arg(packageIndex + 1);
        qWarning() << "[" << m_serialPort->portName() << "]" << errorCode;
        return QByteArray();
    }

    QByteArray package;
    const int maxPackageSize = 512;
    const int timeoutMs = 5000;

    while (timer.elapsed() < timeoutMs && package.size() < maxPackageSize) {
        if (m_serialPort->waitForReadyRead(100)) {
            QByteArray data = m_serialPort->readAll();

            if (data.isEmpty()) {
                continue;
            }
            qDebug() << "resp_raw:" << data.toHex(' ').toUpper();

            if (package.isEmpty() && data.size() >= 2) {
                int startIndex = -1;
                for (int i = 0; i <= data.size() - 2; i++) {
                    if (static_cast<quint8>(data[i]) == 0xF8
                        && static_cast<quint8>(data[i + 1]) == 0xF8) {
                        startIndex = i;
                        break;
                    }
                }

                if (startIndex >= 0) {
                    if (startIndex > 0) {
                        qDebug() << "[" << m_serialPort->portName() << tr("] Skipping")
                                 << startIndex << tr("bytes before F8 F8 header");
                        data = data.mid(startIndex);
                    }
                } else {
                    qWarning() << "[" << m_serialPort->portName()
                               << tr("] No F8 F8 header found, skipping") << data.size()
                               << tr("bytes");
                    continue;
                }
            }

            qDebug() << "resp_pro:" << data.toHex(' ').toUpper();

            package.append(data);

            if (package.size() >= maxPackageSize) {
                break;
            }
        }

        if (timer.elapsed() % 200 == 0) {
            QCoreApplication::processEvents();
        }
    }

    if (package.size() > maxPackageSize) {
        package = package.left(maxPackageSize);
    }

    if (package.size() < maxPackageSize) {
        errorCode = QString(tr("Incomplete package %1: %2 bytes, expected %3 (timeout: %4ms)"))
                        .arg(packageIndex + 1)
                        .arg(package.size())
                        .arg(maxPackageSize)
                        .arg(timer.elapsed());
        qWarning() << "[" << m_serialPort->portName() << "]" << errorCode;

        if (!package.isEmpty()) {
            qDebug() << "[" << m_serialPort->portName() << tr("] Received data (first 32 bytes):")
                     << package.left(qMin(32, package.size())).toHex(' ');
        }

        return QByteArray();
    }

    if (static_cast<quint8>(package[0]) != 0xF8 || static_cast<quint8>(package[1]) != 0xF8) {
        errorCode = QString(tr("Invalid package %1 header: 0x%2 0x%3"))
                        .arg(packageIndex + 1)
                        .arg(QString::number(static_cast<quint8>(package[0]), 16).toUpper())
                        .arg(QString::number(static_cast<quint8>(package[1]), 16).toUpper());
        qWarning() << "[" << m_serialPort->portName() << "]" << errorCode;
        return QByteArray();
    }

    return package;
}

QByteArray XSensorProtocol::processImageData(const QByteArray& rawData, int cols, int rows)
{
    int expectedSize = cols * rows * 2;
    if (rawData.size() < expectedSize) {
        return QByteArray();
    }

    // 第一步：按照Python方式交换字节
    QByteArray swappedData;
    swappedData.reserve(expectedSize);

    const quint8* src = reinterpret_cast<const quint8*>(rawData.constData());

    for (int i = 0; i < expectedSize; i++) {
        if (i % 2 == 0) {
            swappedData.append(static_cast<char>(src[i + 1]));
            swappedData.append(static_cast<char>(src[i]));
        }
    }

    // 第二步：裁剪图像
    const int crop = 4;
    const int newW = cols - 2 * crop;
    const int newH = rows - 2 * crop;

    if (newW <= 0 || newH <= 0) {
        return QByteArray();
    }

    QByteArray result(newW * newH * 2, 0);
    const quint8* swappedSrc = reinterpret_cast<const quint8*>(swappedData.constData());
    quint8* dst = reinterpret_cast<quint8*>(result.data());

    for (int y = crop; y < rows - crop; y++) {
        const quint8* rowStart = swappedSrc + y * cols * 2;
        int dstOffset = (y - crop) * newW * 2;
        memcpy(dst + dstOffset, rowStart + crop * 2, newW * 2);
    }

    return result;
}

void XSensorProtocol::setCalibrationBefore() {}

bool XSensorProtocol::stopWorkMode()
{
    stopExposure();
    return true;
}

QString XSensorProtocol::portName() const
{
    if (m_serialPort) {
        return m_serialPort->portName();
    }
    return QString();
}

bool XSensorProtocol::isConnected() const
{
    return m_serialPort && m_serialPort->isOpen() && m_deviceInitialized;
}

QString XSensorProtocol::getLastError() const
{
    return m_lastError;
}

bool XSensorProtocol::getVersion()
{
    m_serialPort->clear();
    QThread::msleep(100);

    if (sendCommand(Commands::FA_FIND_DEVICE)) {
        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            return false;
        } else {
            if (!parseDeviceInfo(response))
                return false;
        }
    } else {
        return false;
    }
    return true;
}

QString XSensorProtocol::getDeviceInfoVersion()
{
    return m_deviceInfo.version;
}

QString XSensorProtocol::getDeviceInfoSN()
{
    return m_deviceInfo.sn;
}

XSensorProtocol::DeviceInfo XSensorProtocol::getDeviceInfo()
{
    return m_deviceInfo;
}

bool XSensorProtocol::powerOn()
{
    if (sendCommand(Commands::F4_POWER_OFF)) {
        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            return false;
        }
    } else {
        return false;
    }

    QThread::msleep(500);

    if (sendCommand(Commands::F4_POWER_ON_3V3)) {
        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            return false;
        }
    } else {
        return false;
    }

    QThread::msleep(500);

    if (sendCommand(Commands::F4_POWER_ON)) {
        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            return false;
        }
    } else {
        return false;
    }

    m_poweredOn = true;
    return true;
}

bool XSensorProtocol::powerOff()
{
    if (m_poweredOn) {
        if (sendCommand(Commands::F8_TO_LOWER_POWER)) {
            QByteArray response = readResponse(m_readTimeout);
            if (response.isEmpty()) {
                return false;
            }
        } else {
            return false;
        }

        QThread::msleep(500);
    }

    if (sendCommand(Commands::F4_POWER_OFF)) {
        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            return false;
        }
    } else {
        return false;
    }

    m_poweredOn = false;
    return true;
}

bool XSensorProtocol::setupWorkMode(bool b)
{
    if (!isConnected()) {
        m_lastError = tr("Device not connected");
        return false;
    }

    if (m_isBusy) {
        m_lastError = tr("Device is busy");
        qWarning() << m_lastError;
        return false;
    }

    m_isBusy = true;

    try {
        emit statusChanged(tr("Setting up work mode..."));

        m_exposing = true;

        if (m_serialPort) {
            m_serialPort->clear(QSerialPort::Input | QSerialPort::Output);
            QThread::msleep(200);
            m_serialPort->readAll();
        }

        if (!powerOn()) {
            m_lastError = tr("Failed to power on device");
            qWarning() << m_lastError;
            m_isBusy = false;
            m_exposing = false;
            return false;
        }

        if (!sendF5Config()) {
            qWarning() << tr("F5 config failed");
            powerOff();
        } else {
            emit statusChanged(tr("Work mode setup successful"));
        }

    } catch (...) {
        qCritical() << tr("Exception in setupWorkMode");
        if (m_poweredOn) {
            powerOff();
        }
        m_isBusy = false;
        return false;
    }

    m_isBusy = false;
    return true;
}

void XSensorProtocol::stopExposure()
{
    if (m_poweredOn) {
        powerOff();
    }

    m_exposing = false;
    m_cancelRequested = true;
    emit exposureCompleted();
}

bool XSensorProtocol::sendCommand(const QByteArray& cmd)
{
    QByteArray fullCommand = buildCommand(cmd, '\x0D', '\x0A');
    if (fullCommand.isEmpty()) {
        m_lastError = tr("Failed to build command");
        return false;
    }
    QString hexStr = fullCommand.toHex(' ').toUpper();

    static bool flag = true;

    if (Commands::F8_REQUEST_IMAGE != cmd)
        qDebug() << "req:" << hexStr << "to" << m_serialPort->portName();
    else {
        if (flag) {
            flag = false;
            qDebug() << "req:" << hexStr << "to" << m_serialPort->portName();
        }
    }
    m_serialPort->write(fullCommand);

    if (!m_serialPort->waitForBytesWritten(m_writeTimeout)) {
        m_lastError = tr("Write timeout");
        qWarning() << m_lastError;
        return false;
    }

    m_serialPort->flush();
    return true;
}

QByteArray XSensorProtocol::readResponse(int timeout)
{
    QByteArray response;
    QElapsedTimer timer;
    timer.start();

    if (m_serialPort->bytesAvailable() > 0) {
        QByteArray existing = m_serialPort->readAll();
        response.append(existing);
    }

    while (timer.elapsed() < timeout && response.size() < 1024) {
        if (m_serialPort->waitForReadyRead(50)) {
            QByteArray newData = m_serialPort->readAll();
            if (!newData.isEmpty()) {
                response.append(newData);
            }
        }

        QCoreApplication::processEvents();
    }

    if (!response.isEmpty() && validateCRC(response)) {
        // qDebug() << "resp:" << packet.toHex(' ').toUpper();
        return response;
    }

    return QByteArray();
}

void XSensorProtocol::cleanupAfterImageAcquisition(bool success)
{
    try {
        m_exposing = false;
        m_isBusy = false;

        if (m_poweredOn) {
            powerOff();
        }

        if (success) {
            emit exposureCompleted();
        } else {
            emit exposureCompleted();
            emit statusChanged(tr("Image acquisition failed"));
        }
    } catch (const std::exception& ex) {
        qCritical() << tr("Exception in cleanupAfterImageAcquisition:") << ex.what();
        m_isBusy = false;
        m_exposing = false;
        m_poweredOn = false;
    }
}

bool XSensorProtocol::sendF5Config()
{
    if (sendCommand(Commands::F5_CONFIG)) {
        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            return false;
        } else {
            if (response.size() >= 2 && static_cast<quint8>(response[0]) == 0xF5
                && static_cast<quint8>(response[1]) == 0xF5) {
                qDebug() << tr("F5 config successful!");
                qDebug() << "resp" << response.toHex(' ');
            } else {
                qDebug() << "resp" << response.toHex(' ');
                return false;
            }
        }
    } else {
        return false;
    }

    if (sendCommand(Commands::F5_READ_CONFIG)) {
        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            return false;
        } else {
            if (response.size() >= 2 && static_cast<quint8>(response[0]) == 0xF5
                && static_cast<quint8>(response[1]) == 0xF5) {
                qDebug() << tr("F5 config echo successful!");
                qDebug() << "resp" << response.toHex(' ');
            } else {
                qDebug() << "resp" << response.toHex(' ');
                return false;
            }
        }
    } else {
        return false;
    }

    if (sendCommand(Commands::F5_EXTERNAL_BIAL)) {
        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            return false;
        } else {
            if (response.size() >= 2 && static_cast<quint8>(response[0]) == 0xF5
                && static_cast<quint8>(response[1]) == 0xF5) {
                qDebug() << tr("F5 exernal bial successful!");
                qDebug() << "resp" << response.toHex(' ');
            } else {
                qDebug() << "resp" << response.toHex(' ');
                return false;
            }
        }
    } else {
        return false;
    }

    if (sendCommand(Commands::F5_READ_EXTERNAL_BIAL)) {
        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            return false;
        } else {
            if (response.size() >= 2 && static_cast<quint8>(response[0]) == 0xF5
                && static_cast<quint8>(response[1]) == 0xF5) {
                qDebug() << tr("F5 read exernal bial successful!");
                qDebug() << "resp" << response.toHex(' ');
            } else {
                qDebug() << "resp" << response.toHex(' ');
                return false;
            }
        }
    } else {
        return false;
    }
    // 2. 电压配置
    // if (true) {
    //     int vset = (int) (0.75 * 2000);
    //     int vth1 = (int) (0.70 * 2000);
    //     int vth2 = (int) (0.60 * 2000);
    //     int vref3 = (int) (3.0 * 2000);

    //     QByteArray voltageCmd;
    //     voltageCmd.append(static_cast<char>(0xF5));
    //     voltageCmd.append(static_cast<char>(0xF5));
    //     voltageCmd.append(static_cast<char>(0x05));
    //     voltageCmd.append(static_cast<char>(0x02));
    //     voltageCmd.append(static_cast<char>(0x00));
    //     voltageCmd.append(static_cast<char>(0x00));
    //     voltageCmd.append(static_cast<char>(0x00));
    //     voltageCmd.append(static_cast<char>(0x00));
    //     voltageCmd.append(static_cast<char>(0x00));
    //     voltageCmd.append(static_cast<char>(0x08));

    //     voltageCmd.append(static_cast<char>(vset & 0xFF));
    //     voltageCmd.append(static_cast<char>((vset >> 8) & 0xFF));
    //     voltageCmd.append(static_cast<char>(vth1 & 0xFF));
    //     voltageCmd.append(static_cast<char>((vth1 >> 8) & 0xFF));
    //     voltageCmd.append(static_cast<char>(vth2 & 0xFF));
    //     voltageCmd.append(static_cast<char>((vth2 >> 8) & 0xFF));
    //     voltageCmd.append(static_cast<char>(vref3 & 0xFF));
    //     voltageCmd.append(static_cast<char>((vref3 >> 8) & 0xFF));

    //     if (!sendCommand(voltageCmd)) {
    //         qWarning() << tr("Failed to send voltage config command");
    //         return false;
    //     }

    //     QByteArray response = readResponse(m_readTimeout);
    //     if (response.isEmpty()) {
    //         qWarning() << tr("No response to voltage config");
    //         return false;
    //     }

    //     QThread::msleep(500);
    // }

    QThread::msleep(100);

    // 3. 参数校正（如果需要）
    //....

    if (m_serialPort) {
        m_serialPort->clear(QSerialPort::Input | QSerialPort::Output);
        QThread::msleep(100);
        m_serialPort->readAll();
    }

    // 5. 回调 - 开始曝光
    emit exposureF5Ready(m_serialPort->portName());

    return true;
}

bool XSensorProtocol::sendF8ImageRequest()
{
    if (sendCommand(Commands::F8_REQUEST_IMAGE)) {
        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

bool XSensorProtocol::parseDeviceInfo(const QByteArray& data)
{
    if (data.size() < 50 || static_cast<quint8>(data[0]) != 0xFA
        || static_cast<quint8>(data[1]) != 0xFA) {
        return false;
    }

    // 版本号
    m_deviceInfo.version
        = QString("%1.%2").arg(static_cast<quint8>(data[3])).arg(static_cast<quint8>(data[4]));

    // 电源版本号
    if (data.size() >= 7) {
        m_deviceInfo.powerVersion
            = QString("%1.%2").arg(static_cast<quint8>(data[5])).arg(static_cast<quint8>(data[6]));
    }

    // 序列号
    if (data.size() >= 22) {
        m_deviceInfo.sn = bytesToHexString(data.mid(10, 12));
    }

    // 电源序列号
    if (data.size() >= 34) {
        m_deviceInfo.powerSN = bytesToHexString(data.mid(22, 12));
    }

    // 打印设备信息
    qDebug() << "=== Device Info ===";
    qDebug() << "Version:" << m_deviceInfo.version;
    qDebug() << "Power Version:" << m_deviceInfo.powerVersion;
    qDebug() << "SN:" << m_deviceInfo.sn;
    qDebug() << "Power SN:" << m_deviceInfo.powerSN;
    qDebug() << "===================";

    return !m_deviceInfo.version.isEmpty() && m_deviceInfo.version != "0.0";
}

bool XSensorProtocol::validateCRC(const QByteArray& data)
{
    if (data.size() < 4) {
        return false;
    }

    if (static_cast<quint8>(data[0]) == 0xFA && static_cast<quint8>(data[1]) == 0xFA) {
        if (data.size() < 2) {
            return true;
        }

        QByteArray dataWithoutCRC = data.left(data.size() - 2);
        QByteArray receivedCRC = data.right(2);

        QByteArray calculatedCRC = calculateCRC(dataWithoutCRC);

        if (calculatedCRC == receivedCRC) {
            return true;
        } else {
            qDebug() << tr("Expected:") << calculatedCRC.toHex(' ');
            qDebug() << tr("Received:") << receivedCRC.toHex(' ');

            return false;
        }
    }

    if (data.size() < 2) {
        return false;
    }

    QByteArray dataWithoutCRC = data.left(data.size() - 2);
    QByteArray calculatedCRC = calculateCRC(dataWithoutCRC);
    QByteArray receivedCRC = data.right(2);

    return calculatedCRC == receivedCRC;
}

QByteArray XSensorProtocol::calculateCRC(const QByteArray& data)
{
    return SensorCRC16::calculate(data);
}

QString XSensorProtocol::bytesToHexString(const QByteArray& bytes)
{
    QString result;
    for (int i = 0; i < bytes.size(); i++) {
        result += QString("%1").arg(static_cast<quint8>(bytes[i]), 2, 16, QLatin1Char('0')).toUpper();
    }
    return result;
}
