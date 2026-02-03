
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
    "F5F501050000000000247FFF000001020A101A1810E4181B194C067C115C11DC0E820D5417001500142016C016C4");

QByteArray XSensorProtocol::Commands::F6_ENABLE_EXPOSE = QByteArray::fromHex(
    "F6F6010000000000000A0002000000C800001388"); //5
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
    , m_serial(nullptr)
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

    m_baudRate = ConfigFileManager::getInstance()->getValue("serial/SerialBaudRate20").toInt();
    m_readTimeout = ConfigFileManager::getInstance()->getValue("serial/SerialReadTimeout").toInt();
    m_openTimeout = ConfigFileManager::getInstance()->getValue("serial/SPOpenTimeout").toInt();
    m_retryTimes
        = ConfigFileManager::getInstance()->getValue("serial/RetryReadImageByteTimes").toInt();
    m_echoTimeout = ConfigFileManager::getInstance()->getValue("serial/EchoCOMTimeOut").toInt();
    m_normalCmdTimeout
        = ConfigFileManager::getInstance()->getValue("serial/NormalCMDTimeOut").toInt();

    m_rectifyF5Enabled = QVariant(ConfigFileManager::getInstance()->getValue(
                                      "hardware/RectifyF5CmdEnabled"))
                             .toBool();
    m_rectifyAECEnabled = QVariant(ConfigFileManager::getInstance()->getValue(
                                       "hardware/RectifyAECEnabled"))
                              .toBool();
    m_imageLogEnabled
        = QVariant(ConfigFileManager::getInstance()->getValue("hardware/ImageLogEnable")).toBool();

    m_acquireImageDelay
        = ConfigFileManager::getInstance()->getValue("timing/AcquireImageBagDelay").toInt();
    if (m_acquireImageDelay <= 0) {
        m_acquireImageDelay = 50;
    }

    bool portExists = false;
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports) {
        if (info.portName() == portName) {
            portExists = true;
            qDebug() << "Port found:" << portName << "description:" << info.description();
            break;
        }
    }

    if (!portExists) {
        qWarning() << "Port" << portName << "does not exist";
        m_lastError = QString("端口 %1 不存在").arg(portName);
        return false;
    }

    if (m_serial) {
        m_serial->close();
        delete m_serial;
        m_serial = nullptr;
    }

    m_serial = internalGetTransducerCOM(portName);

    if (m_serial && m_serial->isOpen()) {
        qInfo() << "串口初始化成功:" << portName;
        m_deviceInitialized = true;
        return true;
    }

    qWarning() << "串口初始化失败:" << portName;
    return false;
}

QSerialPort* XSensorProtocol::internalGetTransducerCOM(const QString& comPort)
{
    qInfo() << "线程" << QThread::currentThreadId() << "Opening port:" << comPort;

    QSerialPort* sp = new QSerialPort();
    sp->setPortName(comPort);

    sp->setBaudRate(2000000);
    sp->setDataBits(QSerialPort::Data8);
    sp->setParity(QSerialPort::NoParity);
    sp->setStopBits(QSerialPort::OneStop);
    sp->setFlowControl(QSerialPort::NoFlowControl);

    if (sp->open(QIODevice::ReadWrite)) {
        qInfo() << "Port opened successfully at 2000000 baud";
        QThread::msleep(500);
        sp->clear();

        if (echoSerialPort(sp)) {
            return sp;
        } else {
            delete sp;
            return nullptr;
        }
    } else {
        qWarning() << "Failed to open port:" << comPort;
        delete sp;
        return nullptr;
    }
}

void XSensorProtocol::setupSerialPortCallbacks(QSerialPort* serialPort)
{
    if (!serialPort)
        return;

    disconnect(serialPort, nullptr, nullptr, nullptr);

    connect(serialPort,
            QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this,
            [serialPort](QSerialPort::SerialPortError error) {
                if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
                    qDebug() << "Serial error on" << serialPort->portName() << ":" << error;
                }
            });
}

bool XSensorProtocol::echoSerialPort(QSerialPort* serialPort)
{
    if (!serialPort || !serialPort->isOpen()) {
        return false;
    }

    qInfo() << "=== Starting echo test on" << serialPort->portName() << "===";

    QSerialPort* oldSerial = m_serial;
    m_serial = serialPort;

    bool echoSuccess = false;
    bool powerOnSuccess = false;

    try {
        qInfo() << "1. Clearing buffer before starting...";
        m_serial->clear(QSerialPort::Input | QSerialPort::Output);
        QThread::msleep(200);

        QByteArray initialLeftover = m_serial->readAll();
        if (!initialLeftover.isEmpty()) {
            qInfo() << "  Discarded initial leftover:" << initialLeftover.size() << "bytes";
        }

        qInfo() << "2. Powering on device...";
        powerOnSuccess = powerOn();
        if (!powerOnSuccess) {
            qWarning() << "Power on failed: " << m_lastError;
            throw std::runtime_error("Power on failed");
        }

        qInfo() << "3. Waiting for device to stabilize...";
        QThread::msleep(300);

        m_serial->clear(QSerialPort::Input | QSerialPort::Output);
        QThread::msleep(100);

        QByteArray afterPowerOnLeftover = m_serial->readAll();
        if (!afterPowerOnLeftover.isEmpty()) {
            qInfo() << "  Discarded after power-on leftover:" << afterPowerOnLeftover.size()
                    << "bytes";
        }

        qInfo() << "4. Sending FA find device command...";

        for (int attempt = 1; attempt <= 3; attempt++) {
            qInfo() << "  Attempt" << attempt << "/3";

            m_serial->clear(QSerialPort::Input | QSerialPort::Output);
            QThread::msleep(100);
            m_serial->readAll();

            if (!sendCommand(Commands::FA_FIND_DEVICE)) {
                qWarning() << "Failed to send FA command";
                continue;
            }

            qInfo() << "5. Checking for response...";
            QThread::msleep(300);

            QByteArray response = readResponse(2000);
            qDebug() << "Response:" << response.toHex(' ');

            if (response.size() >= 512) {
                QString hexString;
                for (int i = 0; i < qMin(10, response.size()); i++) {
                    hexString
                        += QString("%1 ").arg((quint8) response[i], 2, 16, QChar('0')).toUpper();
                }
                qDebug() << "  First 10 bytes:" << hexString;

                if (response.size() >= 2) {
                    quint8 firstByte = static_cast<quint8>(response[0]);
                    quint8 secondByte = static_cast<quint8>(response[1]);

                    if (firstByte == 0xFA && secondByte == 0xFA) {
                        qInfo() << "  *** FA FA header found ***";

                        m_deviceInfo = parseDeviceInfo(response);

                        if (!m_deviceInfo.version.isEmpty() && m_deviceInfo.version != "0.0") {
                            qInfo() << "  Device found - Version:" << m_deviceInfo.version
                                    << "SN:" << m_deviceInfo.sn;
                            echoSuccess = true;
                            break;
                        }
                    } else {
                        qInfo() << "  Device responded but not with FA FA header";
                        qDebug() << "  Header: 0x" << QString::number(firstByte, 16).toUpper()
                                 << "0x" << QString::number(secondByte, 16).toUpper();

                        qInfo() << "  *** Accepting device (512 bytes response) ***";
                        m_deviceInfo.version = "1.0";
                        m_deviceInfo.sn = "Unknown_" + serialPort->portName();
                        echoSuccess = true;
                        break;
                    }
                }
            } else {
                qWarning() << "  Response too small:" << response.size() << "bytes";
            }

            QThread::msleep(500);
        }

    } catch (const std::exception& e) {
        qCritical() << "Exception during echo test:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception during echo test";
    }

    qInfo() << "7. Powering off device...";
    if (m_poweredOn) {
        powerOff();
    } else if (powerOnSuccess) {
        qWarning() << "Device power state incorrect, forcing power off";
        sendCommand(Commands::F4_POWER_OFF);
        QThread::msleep(100);
    }

    m_serial = oldSerial;

    if (echoSuccess) {
        qInfo() << "*** Echo test successful! ***";
    } else {
        qWarning() << "Echo failed: " << m_lastError;
    }

    return echoSuccess;
}

void XSensorProtocol::closeSerialPort()
{
    qInfo() << "=== 关闭串口 (曝光后) ===";
    qInfo() << "当前状态 - 曝光中:" << m_exposing << "电源开启:" << m_poweredOn
            << "忙碌:" << m_isBusy;

    m_exposing = false;
    m_poweredOn = false;
    m_isBusy = false;
    m_cancelRequested = true;

    QMutexLocker locker(&m_mutex);

    try {
        m_isBusy = true;

        if (m_serial) {
            qWarning() << "Actively close port" << m_serial->portName();

            if (m_serial->isOpen()) {
                m_serial->close();
            }

            m_serial->deleteLater();
            m_serial = nullptr;
        }
    } catch (const std::exception& e) {
        m_isBusy = false;
        qCritical() << "Error in closeSerialPort:" << e.what();
        m_lastError = QString("Close port error: %1").arg(e.what());
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
    m_serial = nullptr;
}

bool XSensorProtocol::checkActive()
{
    return isConnected() && m_poweredOn;
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

bool XSensorProtocol::switchToLowPowerMode()
{
    if (!m_serial || !m_serial->isOpen()) {
        return false;
    }

    qInfo() << "Switching to low power mode...";

    QByteArray lowPowerCmd = buildCommand(Commands::F8_TO_LOWER_POWER, '\x0D', '\x0A');

    qint64 written = m_serial->write(lowPowerCmd);
    if (written != lowPowerCmd.size()) {
        qWarning() << "Failed to write low power mode command";
        return false;
    }

    if (!m_serial->waitForBytesWritten(m_writeTimeout)) {
        qWarning() << "Write timeout for low power mode command";
        return false;
    }

    m_serial->flush();

    QByteArray response = readResponse(m_readTimeout);
    if (response.isEmpty()) {
        qWarning() << "No response to low power mode command";
        return false;
    }

    qDebug() << "Response:" << response.toHex(' ');

    if (response.size() >= 4 && static_cast<quint8>(response[0]) == 0xF8
        && static_cast<quint8>(response[1]) == 0xF8) {
        qInfo() << "Switched to low power mode successfully";
        return true;
    }

    return false;
}

// ==================================================================================
// 🔧 修复关键函数1: enableExposure - 增加等待时间和响应超时
// ==================================================================================
bool XSensorProtocol::enableExposure(bool wait, bool isCalibration)
{
    qInfo() << "[" << (m_serial ? m_serial->portName() : "Unknown")
            << "] 开始使能曝光，等待:" << wait << "，校准:" << isCalibration;

    if (wait) {
        QThread::msleep(1950);
    }

    QByteArray exposureCommand = isCalibration ? Commands::F6_ENABLE_EXPOSE_CALI
                                               : Commands::F6_ENABLE_EXPOSE;

    QByteArray fullCommand = buildCommand(exposureCommand, '\x0D', '\x0A');
    if (fullCommand.isEmpty()) {
        qCritical() << "[" << m_serial->portName() << "] 构建曝光命令失败";
        return false;
    }

    QString hexStr = fullCommand.toHex(' ').toUpper();
    qDebug() << "[TX] 线程" << QThread::currentThreadId() << "发送F6命令:" << hexStr << "到端口"
             << m_serial->portName();

    m_serial->write(fullCommand);

    if (!m_serial->waitForBytesWritten(m_writeTimeout)) {
        qCritical() << "[" << m_serial->portName() << "] 写入超时";
        return false;
    }

    m_serial->flush();

    qInfo() << "[" << m_serial->portName() << "] 等待F6响应...";
    QByteArray response = readResponse(6000);

    qDebug() << "Response:" << response.toHex(' ');
    bool exposureSuccess = false;
    if (response.size() >= 4 && static_cast<quint8>(response[0]) == 0xF6
        && static_cast<quint8>(response[1]) == 0xF6) {
        exposureSuccess = true;
        m_exposing = true;

        emit exposureF6Ready(m_serial->portName());
    } else {

        if (!response.isEmpty()) {
            qWarning() << "[" << m_serial->portName() << "] 响应头: 0x"
                       << QString::number(static_cast<quint8>(response[0]), 16).toUpper() << "0x"
                       << QString::number(static_cast<quint8>(response[1]), 16).toUpper();
        }

        if (m_poweredOn) {
            qWarning() << "[" << m_serial->portName() << "] F6失败，尝试关闭设备电源";
            sendCommand(Commands::F8_TO_LOWER_POWER);
            QThread::msleep(100);
            sendCommand(Commands::F4_POWER_OFF);
            m_poweredOn = false;
        }
    }

    return exposureSuccess;
}

void XSensorProtocol::acquireImage()
{
    if (m_isBusy) {
        m_lastError = "Device is busy";
        emit warningOccurred(m_lastError);
        return;
    }

    if (!m_exposing) {
        m_lastError = "Device not connected or not exposing";
        emit warningOccurred(m_lastError);
        return;
    }

    QMutexLocker locker(&m_mutex);

    m_isBusy = true;
    bool allGot = true;

    try {
        emit statusChanged("Acquiring image...");

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
                    qWarning() << "[" << m_serial->portName() << "] Failed to get package"
                               << (i + 1) << ":" << errorCode;
                    break;
                }
            }

            if (allGot) {
                validRawDataInByte = receivedDataBytes.left(
                    qMin(receivedDataBytes.size(), rawImgTotalBytes));

                qInfo() << "[" << m_serial->portName() << "] ^^^^^^ End Acquiring Image."
                        << (realGetBags - 1) << "bags." << sw.elapsed() << "ms ^^^^^^";
                imgRetrieved = true;
            } else {
                qWarning() << "[" << m_serial->portName()
                           << "] Failed to retrieve image data:" << errorCode;
            }
        } catch (const std::exception& ex) {
            qCritical() << "[" << m_serial->portName() << "] Image acquisition error:" << ex.what();
            allGot = false;
        }

        if (imgRetrieved && !validRawDataInByte.isEmpty()) {
            QByteArray processedImage = processImageData(validRawDataInByte, cols, rows);
            if (!processedImage.isEmpty()) {
                emit imageReady(processedImage);
                emit statusChanged("Image acquired successfully");
                qInfo() << "[" << m_serial->portName() << "] Image processing completed";
            } else {
                allGot = false;
                qWarning() << "[" << m_serial->portName() << "] Image processing failed";
            }
        } else {
            allGot = false;
        }

    } catch (const std::exception& ex) {
        qCritical() << "[" << m_serial->portName() << "] AcquireImage exception:" << ex.what();
        allGot = false;
    }

    if (!allGot) {
        qWarning() << "[" << m_serial->portName() << "] Image acquisition failed, cleaning up...";
        if (m_serial && m_poweredOn) {
            powerOff();
        }
    }

    m_isBusy = false;
    m_exposing = false;

    cleanupAfterImageAcquisition(allGot);
    qInfo() << "[" << m_serial->portName() << "] AcquireImage End - Success:" << allGot;
}

QByteArray XSensorProtocol::retrieveImageByte(QString& errorCode, int& realGetBags, int packageIndex)
{
    if (!m_serial || !m_serial->isOpen()) {
        errorCode = "Device not connected";
        return QByteArray();
    }

    QElapsedTimer timer;
    timer.start();

    if (!sendCommand(Commands::F8_REQUEST_IMAGE)) {
        errorCode = QString("Failed to request package %1").arg(packageIndex + 1);
        qWarning() << "[" << m_serial->portName() << "]" << errorCode;
        return QByteArray();
    }

    m_serial->clear(QSerialPort::Input);
    QThread::msleep(10);

    QByteArray package;
    const int maxPackageSize = 512;
    const int timeoutMs = 5000;

    while (timer.elapsed() < timeoutMs && package.size() < maxPackageSize) {
        if (m_serial->waitForReadyRead(100)) {
            QByteArray data = m_serial->readAll();

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
                        qDebug() << "[" << m_serial->portName() << "] Skipping" << startIndex
                                 << "bytes before F8 F8 header";
                        data = data.mid(startIndex);
                    }
                } else {
                    qWarning() << "[" << m_serial->portName() << "] No F8 F8 header found, skipping"
                               << data.size() << "bytes";
                    continue;
                }
            }

            package.append(data);
            qDebug() << "[" << m_serial->portName() << "] Read" << data.size()
                     << "bytes, total:" << package.size();

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
        errorCode = QString("Incomplete package %1: %2 bytes, expected %3 (timeout: %4ms)")
                        .arg(packageIndex + 1)
                        .arg(package.size())
                        .arg(maxPackageSize)
                        .arg(timer.elapsed());
        qWarning() << "[" << m_serial->portName() << "]" << errorCode;

        if (!package.isEmpty()) {
            qDebug() << "[" << m_serial->portName() << "] Received data (first 32 bytes):"
                     << package.left(qMin(32, package.size())).toHex(' ');
        }

        return QByteArray();
    }

    if (static_cast<quint8>(package[0]) != 0xF8 || static_cast<quint8>(package[1]) != 0xF8) {
        errorCode = QString("Invalid package %1 header: 0x%2 0x%3")
                        .arg(packageIndex + 1)
                        .arg(QString::number(static_cast<quint8>(package[0]), 16).toUpper())
                        .arg(QString::number(static_cast<quint8>(package[1]), 16).toUpper());
        qWarning() << "[" << m_serial->portName() << "]" << errorCode;
        return QByteArray();
    }

    return package;
}

QByteArray XSensorProtocol::processImageData(const QByteArray& rawData, int cols, int rows)
{
    if (rawData.size() < cols * rows * 2) {
        m_lastError = QString("Invalid image data size: %1, expected: %2")
                          .arg(rawData.size())
                          .arg(cols * rows * 2);
        qDebug() << m_lastError;
        return QByteArray();
    }

    QByteArray processedImage;

    processedImage.resize(cols * rows * 2);
    for (int i = 0; i < rawData.size(); i += 2) {
        processedImage[i] = rawData[i + 1];
        processedImage[i + 1] = rawData[i];
    }

    emit statusChanged("Image data processed");
    return processedImage;
}

void XSensorProtocol::setCalibrationBefore() {}

bool XSensorProtocol::stopWorkMode()
{
    stopExposure();
    return true;
}

QString XSensorProtocol::portName() const
{
    if (m_serial) {
        return m_serial->portName();
    }
    return QString();
}

bool XSensorProtocol::isConnected() const
{
    return m_serial && m_serial->isOpen() && m_deviceInitialized;
}

bool XSensorProtocol::isInitialized() const
{
    return m_autoInitialized;
}

QString XSensorProtocol::getLastError() const
{
    return m_lastError;
}

void XSensorProtocol::setBaudRate(int baudRate)
{
    QMutexLocker locker(&m_mutex);
    m_baudRate = baudRate;

    if (m_serial && m_serial->isOpen()) {
        m_serial->setBaudRate(m_baudRate);
    }
}

void XSensorProtocol::setTimeouts(int readTimeout,
                                  int writeTimeout,
                                  int echoTimeout,
                                  int exposureTimeout)
{
    QMutexLocker locker(&m_mutex);
    m_readTimeout = readTimeout;
    m_writeTimeout = writeTimeout;
    m_echoTimeout = echoTimeout;
    m_exposureTimeout = exposureTimeout;
}

bool XSensorProtocol::autoInitialize(const QString& preferredPort, int timeoutMs)
{
    QMutexLocker locker(&m_mutex);

    if (m_isBusy) {
        m_lastError = "Device is busy";
        emit warningOccurred(m_lastError);
        return false;
    }

    m_isBusy = true;
    emit statusChanged("Starting auto initialization...");

    QElapsedTimer timer;
    timer.start();

    bool success = false;

    if (!preferredPort.isEmpty()) {
        emit statusChanged(QString("Trying preferred port: %1").arg(preferredPort));
        if (initSerialPort(preferredPort) && setupWorkMode(true)) {
            success = true;
        }
    }

    if (!success) {
        emit statusChanged("Auto detecting device...");
        QString detectedPort = detectDevicePort(timeoutMs / 2);

        if (!detectedPort.isEmpty()) {
            emit statusChanged(QString("Auto-detected device at: %1").arg(detectedPort));

            if (initSerialPort(detectedPort)) {
                success = setupWorkMode(true);
            }
        }
    }

    if (success) {
        m_autoInitialized = true;
        emit deviceInitialized(true);
        emit statusChanged("Auto initialization completed successfully");
    } else {
        m_lastError = "Auto initialization failed";
        emit deviceInitialized(false);
        emit errorOccurred(m_lastError);
    }

    m_isBusy = false;
    return success;
}

bool XSensorProtocol::echoDevice()
{
    if (!m_serial || !m_serial->isOpen()) {
        m_lastError = "Serial port not open";
        return false;
    }

    emit statusChanged("Echoing device...");

    if (!sendCommand(Commands::F4_POWER_ON_3V3)) {
        m_lastError = "Failed to send power on command";
        return false;
    }
    QThread::msleep(200);

    if (!sendCommand(Commands::FA_FIND_DEVICE)) {
        m_lastError = "Failed to send find device command";
        return false;
    }

    QByteArray response = readResponse(m_echoTimeout);
    if (response.isEmpty() || response.size() < 50) {
        m_lastError = "Invalid or empty device response";
        return false;
    }

    qDebug() << "Response:" << response.toHex(' ');

    m_deviceInfo = parseDeviceInfo(response);
    if (m_deviceInfo.version.isEmpty()) {
        m_lastError = "Failed to parse device info";
        return false;
    }

    qDebug() << "Device found - Version:" << m_deviceInfo.version << "SN:" << m_deviceInfo.sn;

    powerOff();
    emit deviceReady();
    emit statusChanged("Device echo successful");
    return true;
}

bool XSensorProtocol::powerOn()
{
    if (!m_serial || !m_serial->isOpen()) {
        m_lastError = "Serial port not open";
        return false;
    }

    emit statusChanged("Powering on device...");

    m_serial->clear();
    QThread::msleep(100);

    if (!sendCommand(Commands::F4_POWER_ON_3V3)) {
        m_lastError = "Failed to send power on command";
        return false;
    } else {
        QByteArray response = readResponse(m_echoTimeout);
        qDebug() << "Response:" << response.toHex(' ');
    }

    QThread::msleep(300);

    if (!sendCommand(Commands::F4_POWER_ON)) {
        m_lastError = "Failed to send main power on command";
        return false;
    } else {
        QByteArray response = readResponse(m_echoTimeout);
        qDebug() << "Response:" << response.toHex(' ');
    }

    m_poweredOn = true;
    emit statusChanged("Device powered on");

    qInfo() << "Power on successful";
    return true;
}

bool XSensorProtocol::powerOff()
{
    if (!m_serial || !m_serial->isOpen()) {
        qWarning() << "Serial port not open";
        return false;
    }

    qInfo() << "Turning off device (with low power mode first)...";

    bool success = false;

    try {
        if (m_poweredOn) {
            if (sendCommand(Commands::F8_TO_LOWER_POWER)) {
                readResponse(m_readTimeout);
            }

            QThread::msleep(200);
        }

        if (sendCommand(Commands::F4_POWER_OFF)) {
            readResponse(m_readTimeout);
            success = true;
            m_poweredOn = false;
        }

    } catch (...) {
        qCritical() << "Exception during power off";
    }

    if (!success && m_serial) {
        qWarning() << "Power off failed, forcing close...";
        m_poweredOn = false;
    }

    return success;
}

// ==================================================================================
// 🔧 修复关键函数2: setupWorkMode - 无修改，但调用了sendF5Config
// ==================================================================================
bool XSensorProtocol::setupWorkMode(bool callbackF5F5)
{
    qDebug() << "=== setupWorkMode (simplified) ===";

    if (!isConnected()) {
        m_lastError = "Device not connected";
        return false;
    }

    QMutexLocker locker(&m_mutex);

    if (m_isBusy) {
        m_lastError = "Device is busy";
        qWarning() << m_lastError;
        return false;
    }

    m_isBusy = true;

    bool success = false;

    try {
        emit statusChanged("Setting up work mode...");

        m_exposing = false;

        if (m_serial) {
            m_serial->clear(QSerialPort::Input | QSerialPort::Output);
            QThread::msleep(200);
            m_serial->readAll();
        }

        if (m_poweredOn) {
            qDebug() << "Device is already powered on, turning off first...";
            powerOff();
            QThread::msleep(1000);
        }

        qDebug() << "Powering on device...";
        if (!powerOn()) {
            m_lastError = "Failed to power on device";
            qWarning() << m_lastError;
            m_isBusy = false;
            return false;
        }

        QThread::msleep(500);

        success = sendF5Config();

        if (!success) {
            qWarning() << "F5 config failed";
            powerOff();
        } else {
            emit statusChanged("Work mode setup successful");
        }

    } catch (...) {
        qCritical() << "Exception in setupWorkMode";
        if (m_poweredOn) {
            powerOff();
        }
        success = false;
    }

    m_isBusy = false;
    return success;
}

void XSensorProtocol::stopExposure()
{
    QMutexLocker locker(&m_mutex);

    m_isBusy = true;
    emit statusChanged("Stopping exposure...");

    m_exposing = false;

    if (m_poweredOn) {
        sendCommand(Commands::F4_POWER_OFF);
        readResponse(m_readTimeout);

        powerOff();
    }

    m_isBusy = false;

    m_cancelRequested = true;
    emit exposureCompleted();
    emit statusChanged("Exposure stopped");
}

bool XSensorProtocol::sendCommand(const QByteArray& cmd)
{
    if (QThread::currentThread() != this->thread()) {
        qDebug() << "调度命令到正确线程...";

        bool result = false;
        QMetaObject::invokeMethod(
            this,
            [this, cmd, &result]() { result = sendCommand(cmd); },
            Qt::BlockingQueuedConnection);
        return result;
    }

    if (!m_serial || !m_serial->isOpen()) {
        m_lastError = "Serial port not open";
        return false;
    }

    QByteArray fullCommand = buildCommand(cmd, '\x0D', '\x0A');
    if (fullCommand.isEmpty()) {
        m_lastError = "Failed to build command";
        return false;
    }

    QString hexStr = fullCommand.toHex(' ').toUpper();
    qDebug() << "[TX] 线程" << QThread::currentThreadId() << "发送命令:" << hexStr << "到端口"
             << m_serial->portName();

    m_serial->write(fullCommand);

    if (!m_serial->waitForBytesWritten(m_writeTimeout)) {
        m_lastError = "Write timeout";
        qWarning() << m_lastError;
        return false;
    }

    m_serial->flush();
    return true;
}

QByteArray XSensorProtocol::readResponse(int timeout)
{
    if (!m_serial || !m_serial->isOpen()) {
        return QByteArray();
    }

    QByteArray response;
    QElapsedTimer timer;
    timer.start();

    if (m_serial->bytesAvailable() > 0) {
        QByteArray existing = m_serial->readAll();
        response.append(existing);
    }

    while (timer.elapsed() < timeout && response.size() < 1024) {
        if (m_serial->waitForReadyRead(50)) {
            QByteArray newData = m_serial->readAll();
            if (!newData.isEmpty()) {
                response.append(newData);
            }
        }

        QCoreApplication::processEvents();
    }

    return response;
}

void XSensorProtocol::cleanupAfterImageAcquisition(bool success)
{
    QMutexLocker locker(&m_mutex);

    try {
        qInfo() << "[" << (m_serial ? m_serial->portName() : "Unknown")
                << "] Cleaning up after image acquisition, success:" << success;

        m_exposing = false;

        if (m_serial && m_serial->isOpen() && m_poweredOn) {
            qInfo() << "[" << m_serial->portName()
                    << "] Powering off device after image acquisition";

            if (sendCommand(Commands::F8_TO_LOWER_POWER)) {
                QThread::msleep(100);
            }

            if (sendCommand(Commands::F4_POWER_OFF)) {
                m_poweredOn = false;
                qInfo() << "[" << m_serial->portName() << "] Device powered off successfully";
            }
        }

        m_isBusy = false;

        if (success) {
            emit exposureCompleted();
            emit statusChanged("Exposure completed and device powered off");
        } else {
            emit errorOccurred("Image acquisition failed");
            emit statusChanged("Image acquisition failed");
        }

    } catch (const std::exception& ex) {
        qCritical() << "Exception in cleanupAfterImageAcquisition:" << ex.what();
        m_isBusy = false;
        m_exposing = false;
        m_poweredOn = false;
    }
}

XSensorProtocol::DeviceInfo XSensorProtocol::getDeviceInfo() const
{
    return m_deviceInfo;
}

QStringList XSensorProtocol::findAvailablePorts()
{
    QStringList ports;
    QList<QSerialPortInfo> portInfos = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo& info : portInfos) {
        ports.append(info.portName());
    }

    return ports;
}

QString XSensorProtocol::detectDevicePort(int timeoutMs)
{
    QStringList ports = findAvailablePorts();

    if (ports.isEmpty()) {
        emit warningOccurred("No serial ports available");
        return QString();
    }

    emit statusChanged(QString("Found %1 available ports").arg(ports.size()));

    QElapsedTimer timer;
    timer.start();

    for (const QString& portName : ports) {
        if (timer.elapsed() > timeoutMs) {
            break;
        }

        emit statusChanged(QString("Testing port: %1").arg(portName));

        QSerialPort testPort;
        testPort.setPortName(portName);
        testPort.setBaudRate(m_baudRate);
        testPort.setDataBits(QSerialPort::Data8);
        testPort.setParity(QSerialPort::NoParity);
        testPort.setStopBits(QSerialPort::OneStop);
        testPort.setFlowControl(QSerialPort::NoFlowControl);

        if (testPort.open(QIODevice::ReadWrite)) {
            QByteArray testCommand = Commands::FA_FIND_DEVICE;
            QByteArray crc = calculateCRC(testCommand);
            testCommand.append(crc);
            testCommand.append(0x0D);
            testCommand.append(0x0A);

            testPort.write(testCommand);
            testPort.waitForBytesWritten(100);

            if (testPort.waitForReadyRead(300)) {
                QByteArray response = testPort.readAll();

                if (response.size() > 20) {
                    testPort.close();
                    return portName;
                }
            }

            testPort.close();
        }

        QThread::msleep(100);
    }

    return QString();
}

// ==================================================================================
// 🔧 修复关键函数3: sendF5Config - 增加设备稳定等待时间
// ==================================================================================
bool XSensorProtocol::sendF5Config()
{
    // 1. 主配置命令
    if (!sendCommand(Commands::F5_CONFIG)) {
        qWarning() << "Failed to send F5 config command";
        return false;
    }

    QByteArray response = readResponse(m_readTimeout);
    if (response.isEmpty()) {
        qWarning() << "No response to F5 config";
        return false;
    }

    qDebug() << "Response:" << response.toHex(' ');

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
            qWarning() << "Failed to send voltage config command";
            return false;
        }

        QByteArray response = readResponse(m_readTimeout);
        if (response.isEmpty()) {
            qWarning() << "No response to voltage config";
            return false;
        }

        qDebug() << "Response:" << response.toHex(' ');

        // 🔧 修复点4: 增加电压配置后的等待时间
        QThread::msleep(500); // 新增500ms等待
    }

    if (response.size() >= 2 && static_cast<quint8>(response[0]) == 0xF5
        && static_cast<quint8>(response[1]) == 0xF5) {
        qDebug() << "F5 config successful!";
    } else {
        qDebug() << "Response:" << response.toHex(' ');
        return false;
    }

    // 🔧 修复点5: F5成功后增加设备稳定时间
    QThread::msleep(100);

    // 3. 参数校正（如果需要）
    //....

    if (m_serial) {
        m_serial->clear(QSerialPort::Input | QSerialPort::Output);
        QThread::msleep(100);
        m_serial->readAll();
    }

    // 5. 回调 - 开始曝光
    emit exposureF5Ready(m_serial->portName());

    return true;
}

bool XSensorProtocol::sendF8ImageRequest()
{
    emit statusChanged("Requesting image...");

    if (!sendCommand(Commands::F8_REQUEST_IMAGE)) {
        m_lastError = "Failed to send image request";
        return false;
    }

    return true;
}

void XSensorProtocol::rectifyDeviceParam()
{
    qDebug() << "=== rectifyDeviceParam (Fixed) ===";

    QByteArray rectifyParam;
    rectifyParam.append(static_cast<char>(0xF5));
    rectifyParam.append(static_cast<char>(0xF5));
    rectifyParam.append(static_cast<char>(0x05));
    rectifyParam.append(static_cast<char>(0x03));
    rectifyParam.append(static_cast<char>(0x00));
    rectifyParam.append(static_cast<char>(0x00));
    rectifyParam.append(static_cast<char>(0x00));
    rectifyParam.append(static_cast<char>(0x00));
    rectifyParam.append(static_cast<char>(0x00));
    rectifyParam.append(static_cast<char>(0x08));

    for (int i = 0; i < 8; i++) {
        rectifyParam.append(static_cast<char>(0x00));
    }

    qDebug() << "Rectify command:" << rectifyParam.toHex(' ');

    if (!sendCommand(rectifyParam)) {
        m_lastError = "Failed to send rectify param command";
        qWarning() << m_lastError;
        return;
    }

    QByteArray response = readResponse(5000);
    if (response.isEmpty()) {
        m_lastError = "No response to rectify param command";
        qWarning() << m_lastError;
        return;
    }
    qDebug() << "Response:" << response.toHex(' ');

    if (response.size() >= 18) {
        auto getValue = [&response](int index) -> int {
            if (index + 1 < response.size()) {
                return (static_cast<quint8>(response.at(index)) << 8)
                       | static_cast<quint8>(response.at(index + 1));
            }
            return 0;
        };

        int vset = getValue(10);
        int vth1 = getValue(12);
        int vth2 = getValue(14);
        int vref3 = getValue(16);

        auto toVoltage = [](int adValue) -> double { return 2.048 * adValue / 4096.0; };

        qInfo() << QString("RectifyDeviceParam: %1V, %2V, %3V, %4V")
                       .arg(toVoltage(vset), 0, 'f', 3)
                       .arg(toVoltage(vth1), 0, 'f', 3)
                       .arg(toVoltage(vth2), 0, 'f', 3)
                       .arg(toVoltage(vref3), 0, 'f', 3);
    } else {
        qWarning() << "Rectify response too short:" << response.size() << "bytes";
    }
}

XSensorProtocol::DeviceInfo XSensorProtocol::parseDeviceInfo(const QByteArray& data)
{
    DeviceInfo info;

    if (data.size() < 50) {
        qWarning() << "parseDeviceInfo: Data too small:" << data.size() << "bytes";
        return info;
    }

    qInfo() << "=== Parsing device info from" << data.size() << "bytes ===";

    try {
        qInfo() << "Data (first 32 bytes):" << data.left(32).toHex(' ');

        if (static_cast<quint8>(data[0]) != 0xFA || static_cast<quint8>(data[1]) != 0xFA) {
            qWarning() << "Invalid header - expected FA FA, got:"
                       << QString::number(static_cast<quint8>(data[0]), 16)
                       << QString::number(static_cast<quint8>(data[1]), 16);
            return info;
        }

        quint8 verMajor = static_cast<quint8>(data[3]);
        quint8 verMinor = static_cast<quint8>(data[4]);
        info.version = QString("%1.%2").arg(verMajor).arg(verMinor);

        qInfo() << "  Oral board version:" << info.version
                << "(bytes:" << QString::number(verMajor, 16).toUpper()
                << QString::number(verMinor, 16).toUpper() << ")";

        if (data.size() >= 7) {
            quint8 powerVerMajor = static_cast<quint8>(data[5]);
            quint8 powerVerMinor = static_cast<quint8>(data[6]);
            info.powerVersion = QString("%1.%2").arg(powerVerMajor).arg(powerVerMinor);
        }

        if (data.size() >= 22) {
            QByteArray snBytes = data.mid(10, 12);
            info.sn = "";
            for (int i = 0; i < snBytes.size(); i++) {
                quint8 byte = static_cast<quint8>(snBytes[i]);
                if (byte >= 32 && byte <= 126) {
                    info.sn += QChar(byte);
                } else {
                    info.sn += QString("%1").arg(byte, 2, 16, QLatin1Char('0')).toUpper();
                }
            }
            qInfo() << "  Oral SN (hex):" << snBytes.toHex(' ');
            qInfo() << "  Oral SN (parsed):" << info.sn;
        }

        if (data.size() >= 34) {
            QByteArray powerSnBytes = data.mid(22, 12);
            info.powerSN = bytesToHexString(powerSnBytes);
        }

        qInfo() << "  Parsed device info:";
        qInfo() << "    Oral Version:" << info.version;
        qInfo() << "    Power Version:" << info.powerVersion;
        qInfo() << "    Oral SN:" << info.sn;
        qInfo() << "    Power SN:" << info.powerSN;

        if (info.version == "0.0" || (info.sn.isEmpty() && info.powerSN.isEmpty())) {
            qWarning() << "  Warning: Parsed version or SN seems invalid";

            for (int i = 0; i < qMin(50, data.size()); i++) {
                qInfo() << QString("    Byte[%1]: 0x%2")
                               .arg(i, 2, 10, QLatin1Char('0'))
                               .arg(QString::number(static_cast<quint8>(data[i]), 16)
                                        .toUpper()
                                        .rightJustified(2, '0'));
            }
        }

    } catch (const std::exception& e) {
        qCritical() << "Exception in parseDeviceInfo:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception in parseDeviceInfo";
    }

    return info;
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
            qDebug() << "FA response CRC mismatch";
            qDebug() << "Expected:" << calculatedCRC.toHex(' ');
            qDebug() << "Received:" << receivedCRC.toHex(' ');

            return true;
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

QByteArray XSensorProtocol::hexStringToBytes(const QString& hexStr)
{
    QByteArray bytes;
    QString cleanStr = hexStr;
    cleanStr = cleanStr.replace(" ", "").replace("-", "");

    if (cleanStr.length() % 2 != 0) {
        qDebug() << "Hex string length must be even";
        return bytes;
    }

    for (int i = 0; i < cleanStr.length(); i += 2) {
        bool ok;
        quint8 byte = cleanStr.mid(i, 2).toUInt(&ok, 16);
        if (ok) {
            bytes.append(static_cast<char>(byte));
        } else {
            qDebug() << "Invalid hex byte at position" << i << ":" << cleanStr.mid(i, 2);
        }
    }

    return bytes;
}

QString XSensorProtocol::bytesToHexString(const QByteArray& bytes)
{
    QString result;
    for (int i = 0; i < bytes.size(); i++) {
        result += QString("%1").arg(static_cast<quint8>(bytes[i]), 2, 16, QLatin1Char('0')).toUpper();
    }
    return result;
}

quint16 XSensorProtocol::bytesToUInt16(const QByteArray& bytes, bool littleEndian)
{
    if (bytes.size() < 2) {
        qDebug() << "Insufficient bytes for UInt16:" << bytes.size();
        return 0;
    }

    const quint8* data = reinterpret_cast<const quint8*>(bytes.constData());

    if (littleEndian) {
        return (data[1] << 8) | data[0];
    } else {
        return (data[0] << 8) | data[1];
    }
}

quint32 XSensorProtocol::bytesToUInt32(const QByteArray& bytes, bool littleEndian)
{
    if (bytes.size() < 4) {
        qDebug() << "Insufficient bytes for UInt32:" << bytes.size();
        return 0;
    }

    const quint8* data = reinterpret_cast<const quint8*>(bytes.constData());

    if (littleEndian) {
        return (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
    } else {
        return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    }
}

QByteArray XSensorProtocol::cropImage(
    const QByteArray& image, int cols, int rows, int top, int right, int bottom, int left)
{
    int newWidth = cols - left - right;
    int newHeight = rows - top - bottom;

    if (newWidth <= 0 || newHeight <= 0) {
        qDebug() << "Invalid crop dimensions:" << newWidth << "x" << newHeight;
        return image;
    }

    if (image.size() != cols * rows * 2) {
        qDebug() << "Image size mismatch. Expected:" << cols * rows * 2 << "Got:" << image.size();
        return image;
    }

    QByteArray cropped(newWidth * newHeight * 2, 0);

    const char* src = image.constData();
    char* dst = cropped.data();
    int bytesPerPixel = 2;

    for (int y = top; y < rows - bottom; y++) {
        int srcPos = (y * cols + left) * bytesPerPixel;
        int dstPos = ((y - top) * newWidth) * bytesPerPixel;

        memcpy(dst + dstPos, src + srcPos, newWidth * bytesPerPixel);
    }

    qDebug() << "Image cropped from" << cols << "x" << rows << "to" << newWidth << "x" << newHeight;
    return cropped;
}

void XSensorProtocol::onSerialReadyRead()
{
    if (m_serial && m_serial->isOpen()) {
        QByteArray data = m_serial->readAll();
        emit statusChanged(QString("Data received: %1 bytes").arg(data.size()));
    }
}

void XSensorProtocol::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        QString errorStr
            = QString("Serial port error: %1 - %2").arg(error).arg(m_serial->errorString());
        m_lastError = errorStr;
        qDebug() << errorStr;
        emit errorOccurred(errorStr);

        if (error == QSerialPort::ResourceError || error == QSerialPort::PermissionError
            || error == QSerialPort::DeviceNotFoundError) {
            closeSerialPort();
        }
    }
}
