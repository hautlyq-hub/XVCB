#include "XSensorProtocol.h"
#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QSerialPortInfo>
#include <QThread>
#include <QTimer>

#include "XV.Tool/XDConfigFileManager.h"

// 静态命令数据初始化（保持不变）
QByteArray XSensorProtocol::Commands::F4_POWER_ON_3V3 = QByteArray::fromHex("F4F40400000000000000");
QByteArray XSensorProtocol::Commands::F4_POWER_ON = QByteArray::fromHex("F4F40600000000000000");
QByteArray XSensorProtocol::Commands::F4_POWER_OFF = QByteArray::fromHex("F4F40100000000000000");
QByteArray XSensorProtocol::Commands::FA_FIND_DEVICE = QByteArray::fromHex("FAFA0200000000000000");
QByteArray XSensorProtocol::Commands::F5_VOLTAGE_CONFIG = QByteArray::fromHex(
    "F5F50500000000000000");
QByteArray XSensorProtocol::Commands::F5_CONFIG = QByteArray::fromHex(
    "F5F501050000000000267FFF000001020A101A1810E0181B194C067C115C11DC0E820F200D5417001500142016C016"
    "C4");

QByteArray XSensorProtocol::Commands::F6_ENABLE_EXPOSE = QByteArray::fromHex(
    "F6F6010000000000000A0002000000C800007530"); //5
QByteArray XSensorProtocol::Commands::F6_ENABLE_EXPOSE_02 = QByteArray::fromHex(
    "F6F6010000000000000A000200DB0F4907500000"); //30
QByteArray XSensorProtocol::Commands::F6_ENABLE_EXPOSE_CALI = QByteArray::fromHex(
    "F6F6010000000000000A0002000000C800007530"); // 20.5
QByteArray XSensorProtocol::Commands::F6_ENABLE_EXPOSE_1MIN = QByteArray::fromHex(
    "F6F6010000000000000A0002091131B407500000"); //20.5

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
    , m_readTimeout(100)
    , m_writeTimeout(100)
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

    if (sendCommand(Commands::F4_POWER_OFF)) {
        readResponse(m_readTimeout);
    } else {
        qDebug() << "F4_POWER_OFF";
        return false;
    }

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
        int rawImgTotalBytes = cols * rows * bitsAllocated / 8;
        int validBytesInAPkg = 500;

        int callTimes = (int) ceil((double) rawImgTotalBytes / (double) validBytesInAPkg);

        QByteArray receivedDataBytes(callTimes * validBytesInAPkg, 0);
        QByteArray validRawDataInByte(rawImgTotalBytes, 0);
        bool imgRetrieved = false;
        QString errorCode = "";

        try {
            int tempIndex = 0;
            QElapsedTimer sw;
            sw.start();

            int realGetBags = 1;

            for (int i = 0; i < callTimes; i++) {
                QByteArray rd = retrieveImageByte(errorCode, realGetBags, i);

                if (!rd.isEmpty()) {
                    int bagNum = (static_cast<quint8>(rd[8]) << 8) | static_cast<quint8>(rd[9]);

                    if (bagNum == realGetBags) {
                        for (int index = 10; index < validBytesInAPkg + 10; index++) {
                            if (tempIndex < receivedDataBytes.size()) {
                                receivedDataBytes[tempIndex] = rd[index];
                                ++tempIndex;
                            }
                        }
                        realGetBags++;

                        if ((i + 1) % 10 == 0 || i == callTimes - 1) {
                        }
                    } else {
                        allGot = false;
                        break;
                    }
                } else {
                    allGot = false;
                    qWarning() << "[" << m_serialPort->portName() << tr("] Failed to get package")
                               << (i + 1) << ":" << errorCode;
                    break;
                }
            }

            if (allGot) {
                validRawDataInByte = receivedDataBytes.left(
                    qMin(receivedDataBytes.size(), rawImgTotalBytes));

                imgRetrieved = true;
            } else {
                qWarning() << "[" << m_serialPort->portName()
                           << tr("] Failed to retrieve image data:") << errorCode;
            }
        } catch (const std::exception& ex) {
            qCritical() << "[" << m_serialPort->portName() << tr("] Image acquisition error:")
                        << ex.what();
            allGot = false;
        }

        if (imgRetrieved && !validRawDataInByte.isEmpty()) {
            QByteArray processedImage = processImageData(validRawDataInByte, cols, rows);
            if (!processedImage.isEmpty()) {
                emit imageReady(processedImage);
                emit statusChanged(tr("Image acquired successfully"));
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
    }

    m_isBusy = false;
    m_exposing = false;

    cleanupAfterImageAcquisition(allGot);
}

QByteArray XSensorProtocol::retrieveImageByte(QString& errorCode, int& realGetBags, int packageIndex)
{
    QElapsedTimer timer;
    timer.start();

    if (!sendCommand(Commands::F8_REQUEST_IMAGE)) {
        errorCode = QString(tr("Failed to request package %1")).arg(packageIndex + 1);
        qWarning() << "[" << m_serialPort->portName() << "]" << errorCode;
        return QByteArray();
    }

    m_serialPort->clear(QSerialPort::Input);
    QThread::msleep(10);

    QByteArray package;
    const int maxPackageSize = 512;
    const int timeoutMs = 5000;

    while (timer.elapsed() < timeoutMs && package.size() < maxPackageSize) {
        if (m_serialPort->waitForReadyRead(100)) {
            QByteArray data = m_serialPort->readAll();

            if (data.isEmpty()) {
                continue;
            }

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

            package.append(data);
            // qDebug() << "[" << m_serialPort->portName() << tr("] Read") << data.size()
            //          << tr("bytes, total:") << package.size();

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
    const int crop = 4;
    const int newW = cols - 2 * crop;
    const int newH = rows - 2 * crop;

    if (rawData.size() < cols * rows * 2 || newW <= 0 || newH <= 0) {
        return QByteArray();
    }

    QByteArray result(newW * newH * 2, 0);
    const quint8* src = reinterpret_cast<const quint8*>(rawData.constData());
    quint8* dst = reinterpret_cast<quint8*>(result.data());

    for (int y = crop; y < rows - crop; y++) {
        const quint8* rowStart = src + y * cols * 2;
        for (int x = crop; x < cols - crop; x++) {
            const quint8* pixel = rowStart + x * 2;
            *dst++ = pixel[1];
            *dst++ = pixel[0];
        }
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

    if (!sendCommand(Commands::FA_FIND_DEVICE))
        return false;

    QByteArray response = readResponse(1000);
    if (response.size() < 16)
        return false;

    if (!parseDeviceInfo(response))
        return false;

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
    if (sendCommand(Commands::F4_POWER_ON_3V3)) {
        QByteArray response = readResponse(1000);
    } else {
        return false;
    }

    QThread::msleep(300);

    if (sendCommand(Commands::F4_POWER_ON)) {
        QByteArray response = readResponse(1000);
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
            readResponse(m_readTimeout);
        } else {
            return false;
        }

        QThread::msleep(200);
    }

    if (sendCommand(Commands::F4_POWER_OFF)) {
        readResponse(m_readTimeout);
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

        if (m_poweredOn) {
            qDebug() << tr("Device is already powered on, turning off first...");
            powerOff();
            QThread::msleep(1000);
        }

        if (!powerOn()) {
            m_lastError = tr("Failed to power on device");
            qWarning() << m_lastError;
            m_isBusy = false;
            m_exposing = false;
            return false;
        }

        QThread::msleep(500);

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
    if (Commands::F8_REQUEST_IMAGE != cmd)
        qDebug() << "req:" << hexStr << "to" << m_serialPort->portName();

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

    // qDebug() << "resp:" << response.toHex(' ').toUpper();

    return response;
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
    // 1. 主配置命令
    if (!sendCommand(Commands::F5_CONFIG)) {
        qWarning() << tr("Failed to send F5 config command");
        return false;
    }

    QByteArray response = readResponse(m_readTimeout);
    if (response.isEmpty()) {
        qWarning() << tr("No response to F5 config");
        return false;
    }

    // 2. 电压配置
    if (true) {
        int vset = (int) (0.75 * 2000);
        int vth1 = (int) (0.70 * 2000);
        int vth2 = (int) (0.60 * 2000);
        int vref3 = (int) (3.0 * 2000);

        QByteArray voltageCmd;
        voltageCmd.append(static_cast<char>(0xF5));
        voltageCmd.append(static_cast<char>(0xF5));
        voltageCmd.append(static_cast<char>(0x05));
        voltageCmd.append(static_cast<char>(0x02));
        voltageCmd.append(static_cast<char>(0x00));
        voltageCmd.append(static_cast<char>(0x00));
        voltageCmd.append(static_cast<char>(0x00));
        voltageCmd.append(static_cast<char>(0x00));
        voltageCmd.append(static_cast<char>(0x00));
        voltageCmd.append(static_cast<char>(0x08));

        voltageCmd.append(static_cast<char>(vset & 0xFF));
        voltageCmd.append(static_cast<char>((vset >> 8) & 0xFF));
        voltageCmd.append(static_cast<char>(vth1 & 0xFF));
        voltageCmd.append(static_cast<char>((vth1 >> 8) & 0xFF));
        voltageCmd.append(static_cast<char>(vth2 & 0xFF));
        voltageCmd.append(static_cast<char>((vth2 >> 8) & 0xFF));
        voltageCmd.append(static_cast<char>(vref3 & 0xFF));
        voltageCmd.append(static_cast<char>((vref3 >> 8) & 0xFF));

        if (!sendCommand(voltageCmd)) {
            qWarning() << tr("Failed to send voltage config command");
            return false;
        }

        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            qWarning() << tr("No response to voltage config");
            return false;
        }

        QThread::msleep(500);
    }

    if (response.size() >= 2 && static_cast<quint8>(response[0]) == 0xF5
        && static_cast<quint8>(response[1]) == 0xF5) {
        qDebug() << tr("F5 config successful!");
    } else {
        qDebug() << "resp" << response.toHex(' ');
        return false;
    }

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
    } else {
        m_lastError = tr("Failed to send image request");
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
