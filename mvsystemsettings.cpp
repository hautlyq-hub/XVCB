// mvsystemsettings.cpp
#include "mvsystemsettings.h"
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QMutex>
#include <QPushButton>
#include <QRadioButton>
#include <QSerialPortInfo>
#include <QThread>
#include <QTimer>
#include <QtConcurrent>
#include "ui_mvsystemsettings.h"
#include <algorithm>

mvsystemsettings::mvsystemsettings(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::mvsystemsettings)

    , mSaveSettingsTimer(nullptr)
    , mHWImageDataX(nullptr)
    , mHWImageDataY(nullptr)
    , mIsExposing(false)
    , mRBtnXChecked(true)
{
    ui->setupUi(this);

    initializeOrientationButtons();

    Init();

    InitLanguage();

    initData();

    enableUIComponents(true);
}

mvsystemsettings::~mvsystemsettings()
{
    if (mManager) {
        mManager->unInitSerialPort();
        delete mManager;
        mManager = nullptr;
    }

    if (mInfoTimer) {
        mInfoTimer->stop();
        delete mInfoTimer;
        mInfoTimer = nullptr; // 添加nullptr重置
    }

    if (mAutoExposureTimer) {
        mAutoExposureTimer->stop();
        delete mAutoExposureTimer;
        mAutoExposureTimer = nullptr; // 添加nullptr重置
    }

    if (mSaveSettingsTimer) {
        mSaveSettingsTimer->stop();
        delete mSaveSettingsTimer;
        mSaveSettingsTimer = nullptr; // 添加nullptr重置
    }

    if (mPortMonitorTimer) {
        mPortMonitorTimer->stop();
        delete mPortMonitorTimer;
        mPortMonitorTimer = nullptr; // 添加nullptr重置
    }

    // 清理内存
    qDeleteAll(mToothPosition);
    delete mHWImageDataX;
    delete mHWImageDataY;

    delete ui;
}

void mvsystemsettings::initData()
{
    // 初始化校准数据
    CleanupEnv();

    for (int i = 1; i <= 8; i++) {
        CorrectRelationView* view = new CorrectRelationView();
        view->ViewId = i;
        view->Expose = false;
        view->ImgFileName = "";
        view->XaxisOrYaxis = (i <= 4) ? 0 : 1; // 前4个是X轴，后4个是Y轴
        mToothPosition.append(view);
    }
}

// 在初始化函数中
void mvsystemsettings::initializeOrientationButtons()
{
    // 创建互斥组（如果需要完全互斥）
    QButtonGroup* sensorOriGroup = new QButtonGroup(this);
    sensorOriGroup->setExclusive(true);

    // 将按钮添加到组中
    sensorOriGroup->addButton(ui->mRBtnSensor1);
    sensorOriGroup->addButton(ui->mRBtnSensor2);

    QButtonGroup* sensorOriGroup1 = new QButtonGroup(this);
    sensorOriGroup1->setExclusive(true);

    // 将按钮添加到组中
    sensorOriGroup1->addButton(ui->mRBtnSensorOri1LR);
    sensorOriGroup1->addButton(ui->mRBtnSensorOri1UpDown);

    QButtonGroup* sensorOriGroup2 = new QButtonGroup(this);
    sensorOriGroup2->setExclusive(true);

    // 将按钮添加到组中
    sensorOriGroup2->addButton(ui->mRBtnSensorOri2LR);
    sensorOriGroup2->addButton(ui->mRBtnSensorOri2UpDown);

    connect(ui->mRBtnSensor1, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            qDebug() << "主传感器: 1, 次传感器: 2";
            onComboBoxChanged(); // 保存配置
        }
    });

    connect(ui->mRBtnSensor2, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            qDebug() << "主传感器: 2, 次传感器: 1";
            onComboBoxChanged(); // 保存配置
        }
    });

    // 连接信号
    connect(ui->mRBtnSensorOri1LR, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->mRBtnSensorOri2UpDown->setChecked(true);
            qDebug() << "Sensor 1: Left-Right, Sensor 2: Up-Down";
            onComboBoxChanged(); // 自动保存配置
        }
    });

    // 设置传感器2左右方向
    connect(ui->mRBtnSensorOri2LR, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->mRBtnSensorOri1UpDown->setChecked(true);
            qDebug() << "Sensor 2: Left-Right, Sensor 1: Up-Down";
            onComboBoxChanged(); // 自动保存配置
        }
    });

    // 设置传感器1上下方向
    connect(ui->mRBtnSensorOri1UpDown, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->mRBtnSensorOri2LR->setChecked(true);
            qDebug() << "Sensor 1: Up-Down, Sensor 2: Left-Right";
            onComboBoxChanged(); // 自动保存配置
        }
    });

    // 设置传感器2上下方向
    connect(ui->mRBtnSensorOri2UpDown, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            ui->mRBtnSensorOri1LR->setChecked(true);
            qDebug() << "Sensor 2: Up-Down, Sensor 1: Left-Right";
            onComboBoxChanged(); // 自动保存配置
        }
    });
}

void mvsystemsettings::Init()
{
    // 初始化计时器 - 保持原有命名
    mInfoTimer = new QTimer(this);
    mInfoTimer->setSingleShot(true);
    connect(mInfoTimer, &QTimer::timeout, this, &mvsystemsettings::onInfoTimerTimeout);

    mAutoExposureTimer = new QTimer(this);
    mAutoExposureTimer->setSingleShot(true);
    connect(mAutoExposureTimer, &QTimer::timeout, this, &mvsystemsettings::onAutoExposureTimeout);

    // 自动保存定时器
    mSaveSettingsTimer = new QTimer(this);
    mSaveSettingsTimer->setSingleShot(true);
    connect(mSaveSettingsTimer, &QTimer::timeout, this, &mvsystemsettings::savePortSettings);

    // 串口监控定时器
    mPortMonitorTimer = new QTimer(this);
    mPortMonitorTimer->setInterval(3000);
    connect(mPortMonitorTimer, &QTimer::timeout, this, &mvsystemsettings::checkPortChanges);

    // 设置默认选中
    ui->mRBtnX->setChecked(true);
    mRBtnXChecked = true;

    // 连接信号槽
    connect(ui->mBtnEnableExp,
            &QPushButton::clicked,
            this,
            &mvsystemsettings::onBtnEnableExpClicked);
    connect(ui->mBtnGenerate, &QPushButton::clicked, this, &mvsystemsettings::onBtnGenerateClicked);
    connect(ui->mRBtnX, &QRadioButton::toggled, this, &mvsystemsettings::onRBtnXtoggled);
    connect(ui->mRBtnY, &QRadioButton::toggled, this, &mvsystemsettings::onRBtnYtoggled);

    // 初始化状态显示
    ui->mLblStatusInfo->setText("");
    updateDeviceState(ExposureState::Idle);

    mPreCalFolder = DataLocations::getRootConfigPath() + "/Cal/Pre";

    // 扫描并填充串口列表
    refreshComPorts();

    // 从配置文件加载已保存的端口
    loadSelectedPorts();

    // 启动定时器
    mPortMonitorTimer->start();

    // 创建当前目录
    mCurrentDirX = QDateTime::currentDateTime().toString("QC_X_yyyyMMddHHmmss");
    mCurrentDirY = QDateTime::currentDateTime().toString("QC_Y_yyyyMMddHHmmss");

    mManager = new XProtocolManager(this);
    connect(mManager, &XProtocolManager::info, this, &mvsystemsettings::onInfoReceived);
    connect(mManager, &XProtocolManager::warning, this, &mvsystemsettings::onWarningReceived);
    connect(mManager, &XProtocolManager::errorOccurred, this, &mvsystemsettings::onErrorReceived);
    connect(mManager,
            &XProtocolManager::notification,
            this,
            &mvsystemsettings::onNotificationReceived);
    connect(mManager, &XProtocolManager::exposureError, this, &mvsystemsettings::onExposureError);

    // 连接图像接收信号
    connect(mManager, &XProtocolManager::imagesReady, this, &mvsystemsettings::onImagesReady);
    connect(mManager,
            &XProtocolManager::exposureProcess,
            this,
            &mvsystemsettings::onExposureProcess);

    connect(ui->pushButtonConnect,
            &QPushButton::clicked,
            this,
            &mvsystemsettings::onReconnecttoggled);

    QTimer::singleShot(3000, this, [this]() { mManager->initSerialPort(); });
}

void mvsystemsettings::onReconnecttoggled()
{
    ui->pushButtonConnect->setEnabled(false);

    if (mManager) {
        mManager->unInitSerialPort();

        QThread::msleep(2000);

        QTimer::singleShot(100, this, [this]() {
            bool ret = mManager->initSerialPort();
            if (ret) {
                resetUI();
            } else {
                updateInfoPanel("串口初始化失败", Error);
                updateDeviceState(ExposureState::Fault);
            }
            QTimer::singleShot(2000, this, [this]() { ui->pushButtonConnect->setEnabled(true); });
        });
    }
}

void mvsystemsettings::InitLanguage() {}

void mvsystemsettings::resetUI()
{
    mIsExposing = false;

    updateInfoPanel("", Normal);
    updateDeviceState(ExposureState::Idle);

    ui->mBtnEnableExp->setChecked(false);
    ui->mBtnEnableExp->setText(tr("Exposure"));

    enableUIComponents(true);
}

void mvsystemsettings::enableUIComponents(bool enabled)
{
    // 曝光按钮始终启用（用于切换）
    ui->mBtnEnableExp->setEnabled(enabled);

    // 启用校准按钮
    ui->mBtnGenerate->setEnabled(enabled);

    // 启用轴选择
    // ui->mRBtnX->setEnabled(enabled);
    // ui->mRBtnY->setEnabled(enabled);

    // 启用串口选择
    ui->mCbComSensor1->setEnabled(enabled);
    ui->mCbComSensor2->setEnabled(enabled);
    ui->mCbComXray1->setEnabled(enabled);
    ui->mCbComXray2->setEnabled(enabled);

    // 启用方向选择
    // ui->mRBtnSensorOri1LR->setEnabled(enabled);
    // ui->mRBtnSensorOri1UpDown->setEnabled(enabled);
    // ui->mRBtnSensorOri2LR->setEnabled(enabled);
    // ui->mRBtnSensorOri2UpDown->setEnabled(enabled);
}

// 按钮点击处理
void mvsystemsettings::onBtnEnableExpClicked(bool check)
{
    if (mIsExposing) {
        StopExposure();
    } else {
        StartExposure();
    }
}

void mvsystemsettings::onBtnGenerateClicked()
{
    ui->mBtnGenerate->setEnabled(false);

    try {
        // if (!mHWImageDataX || !mHWImageDataY) {
        //     updateInfoPanel("No raw images found", Warning);
        //     ui->mBtnGenerate->setEnabled(true);
        //     return;
        // }

        // 在后台线程中生成校准文件
        QFuture<void> future = QtConcurrent::run([this]() { GenerateCalibrationFiles(); });

        QFutureWatcher<void>* watcher = new QFutureWatcher<void>();
        connect(watcher, &QFutureWatcher<void>::finished, [this, watcher]() {
            updateInfoPanel("Calibration files generated successfully", Warning);
            ui->mBtnGenerate->setEnabled(true);
            watcher->deleteLater();
        });
        watcher->setFuture(future);

    } catch (...) {
        updateInfoPanel("Failed to generate calibration files", Error);
        ui->mBtnGenerate->setEnabled(true);
    }
}

void mvsystemsettings::onRBtnXtoggled(bool checked)
{
    if (checked) {
        mRBtnXChecked = true;
        // 显示当前使用的设备
        updateInfoPanel("当前使用: X轴 (X光机1)", Normal);
        // 高亮X光机1下拉框
        ui->mCbComXray1->setStyleSheet("border: 2px solid blue;");
        ui->mCbComXray2->setStyleSheet("");
    }
}

void mvsystemsettings::onRBtnYtoggled(bool checked)
{
    if (checked) {
        mRBtnXChecked = false;
        // 显示当前使用的设备
        updateInfoPanel("当前使用: Y轴 (X光机2)", Normal);
        // 高亮X光机2下拉框
        ui->mCbComXray1->setStyleSheet("");
        ui->mCbComXray2->setStyleSheet("border: 2px solid blue;");
    }
}
// 曝光开始
void mvsystemsettings::StartExposure()
{
    mIsExposing = true;
    ui->mBtnEnableExp->setText(tr("Stop"));

    updateInfoPanel("正在设置工作模式...", Normal);
    updateDeviceState(ExposureState::SettingUp);

    // 状态检查
    if (!canStartExposure()) {
        resetUI();
        updateInfoPanel("Lack of exposure conditions", Error);
        updateDeviceState(ExposureState::Fault);
        return;
    }

    int xy = mRBtnXChecked ? 1 : 2;

    // 调用硬件设置
    bool success = mManager->setupWorkMode(true, xy); // 总是需要校准前设置

    if (!success) {
        resetUI();
        updateInfoPanel("设置工作模式失败，请重试", Error);
        updateDeviceState(ExposureState::Fault);
        return;
    }

}

void mvsystemsettings::StopExposure()
{
    if (mAutoExposureTimer && mAutoExposureTimer->isActive()) {
        mAutoExposureTimer->stop();
    }

    if (mManager) {
        mManager->stopExposure();
    }

    resetUI();
}

void mvsystemsettings::AutoNextExposure()
{
    if (mIsExposing) {
        qDebug() << "当前正在曝光中，跳过自动曝光";
        return;
    }

    // 检查是否有未曝光的牙齿位置
    CorrectRelationView* correctRelationView = nullptr;
    for (auto view : mToothPosition) {
        if (!view->Expose) {
            correctRelationView = view;
            break;
        }
    }

    if (correctRelationView) {
        // 延迟1秒后自动曝光
        qDebug() << "找到未曝光位置，将在1秒后启动自动曝光，位置ID:" << correctRelationView->ViewId;
        mAutoExposureTimer->start(1000);
    } else {
        // 所有位置都已完成
        updateInfoPanel("所有位置曝光完成", Normal);
        qDebug() << "所有位置已完成曝光";
    }
}

bool mvsystemsettings::canStartExposure() const
{
    // 检查硬件管理器
    if (!mManager) {
        return false;
    }

    // 检查硬件状态
    if (!mManager->isHardwareReady()) {
        return false;
    }

    // 检查是否有未完成的位置
    bool hasUnExposed = false;
    for (auto view : mToothPosition) {
        if (!view->Expose) {
            hasUnExposed = true;
            break;
        }
    }

    return hasUnExposed;
}

void mvsystemsettings::AcceptImage(const QVector<HWImageData>& raws)
{
    int isx = mRBtnXChecked ? 0 : 1; // 对应原代码中的 isx

    for (const HWImageData& raw : raws) {
        if (raw.orientation % 2 == isx) {
            QString _file;
            if (raw.orientation % 2 == 0) { // X轴
                _file = mCurrentDirX;
            } else { // Y轴
                _file = mCurrentDirY;
            }

            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");

            QString fileName = QDir::toNativeSeparators(mPreCalFolder + "/" + _file + "/"
                                                        + QString("image_%1.raw").arg(timestamp));

            qDebug() << "保存图像到:" << fileName;

            QFileInfo fileInfo(fileName);
            if (!QDir().mkpath(fileInfo.path())) {
                qWarning() << "创建目录失败:" << fileInfo.path();
                return;
            }

            // 如果有原始图像数据，直接保存
            if (!raw.imageData.isEmpty()) {
                QFile file(fileName);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(raw.imageData);
                    file.close();
                    qDebug() << "原始数据已保存到:" << fileName;

                    // 更新数据结构中的文件路径
                    HWImageData& mutableData = const_cast<HWImageData&>(raw);
                    mutableData.filePath = fileName;
                } else {
                    qWarning() << "无法写入文件:" << fileName;
                }
            }
            // 如果有文件路径，复制文件
            else if (!raw.filePath.isEmpty() && QFile::exists(raw.filePath)) {
                if (QFile::copy(raw.filePath, fileName)) {
                    qDebug() << "文件复制成功:" << fileName;

                    // 删除源文件（可选）
                    QFile::remove(raw.filePath);
                } else {
                    qWarning() << "文件复制失败";
                }
            } else {
                qWarning() << "没有可保存的图像数据";
            }
        }
    }

    try {
        int isx = mRBtnXChecked ? 0 : 1;

        for (const HWImageData& raw : raws) {
            if (raw.orientation % 2 == isx) {
                ShowImage(raw, isx);
            }
        }
    } catch (...) {
    }

    onExposureFinished();
}

void mvsystemsettings::ShowImage(const HWImageData& imageData, int xOy)
{
    // 找到对应的牙齿位置
    CorrectRelationView* correctRelationView = nullptr;
    for (auto view : mToothPosition) {
        if (!view->Expose && view->XaxisOrYaxis == xOy) {
            correctRelationView = view;
            break;
        }
    }

    if (correctRelationView) {
        correctRelationView->Expose = true;
        correctRelationView->ImgFileName = imageData.filePath;

        // 根据 ViewId (1-8) 映射到对应的 QLabel
        int viewId = correctRelationView->ViewId;
        if (viewId >= 1 && viewId <= 8) {
            InteractiveImageLabel* label = nullptr;
            switch (viewId) {
            case 1:
                label = ui->mImagePnl1;
                break;
            case 2:
                label = ui->mImagePnl2;
                break;
            case 3:
                label = ui->mImagePnl3;
                break;
            case 4:
                label = ui->mImagePnl4;
                break;
            case 5:
                label = ui->mImagePnl5;
                break;
            case 6:
                label = ui->mImagePnl6;
                break;
            case 7:
                label = ui->mImagePnl7;
                break;
            case 8:
                label = ui->mImagePnl8;
                break;
            }

            if (label) {
                QPixmap pixmap = QPixmap::fromImage(convertRawToImage(imageData.imageData));
                if (!pixmap.isNull()) {
                    label->setImage(pixmap.toImage());
                    label->zoomFit();
                }
            }
        }
    }
}

QImage mvsystemsettings::convertRawToImage(const QByteArray& rawData)
{
    // 假设是 288x288 16位灰度图像
    int width = 288;
    int height = 288;

    // 创建 8位灰度图像用于显示
    QImage image(width, height, QImage::Format_Grayscale8);

    // 简单的 16位转8位（取高8位）
    const unsigned short* src = (const unsigned short*) rawData.constData();
    uchar* dst = image.bits();

    for (int i = 0; i < width * height; i++) {
        dst[i] = src[i] >> 8; // 取高8位
    }

    return image;
}

void mvsystemsettings::CleanupEnv()
{
    if (this->isVisible()) {
        for (auto view : mToothPosition) {
            view->Expose = false;
            view->ImgFileName = "";
            // 恢复默认图标显示
            // view->ImagePanel->setPixmap(...);
        }
    }

    if (mHWImageDataX) {
        delete mHWImageDataX;
        mHWImageDataX = nullptr;
    }

    if (mHWImageDataY) {
        delete mHWImageDataY;
        mHWImageDataY = nullptr;
    }
}

void mvsystemsettings::GenerateCalibrationFiles()
{
    QString _file;
    for (int i = 0; i < 2; i++) {
        if (i == 0)
            _file = mCurrentDirX;
        else
            _file = mCurrentDirY;

        QDir directory(mPreCalFolder + "/" + _file);
        directory.setFilter(QDir::Files);
        QFileInfoList fileList = directory.entryInfoList();
        if (fileList.size() == 0) {
            qDebug() << ("No valid data. Calibration suspended.");
            ui->mLblStatusInfo->setText(tr("No valid data. Calibration suspended."));
            return;
        }

        QString version = mManager->getSensorVersion(QString::number(i));
        if (!version.isEmpty()) {
            mOralMajor = version.left(1).toInt();
        } else {
            mOralMajor = 0; // 默认值
        }

        QString serial = mManager->getSensorSerialNumber(QString::number(i));
        if (!serial.isEmpty()) {
            mOralAddr = serial;
        } else {
            mOralAddr = "Unknown"; // 默认值
        }

        auto algorithmic = XPectAlgorithmic::Instance();
        QString targetFile = DataLocations::getRootConfigPath() + "/Cal/"
                             + QString::number(mOralMajor) + ".0/" + mOralAddr;
        qDebug() << targetFile;
        QDir dir;
        dir.mkpath(targetFile);
        targetFile += "/Calibration.dat";
        if (mImageWidth == 0 && mImageHeight == 0) {
            if (mOralMajor == 7 || mOralMajor == 10) {
                mImageWidth = 288;
                mImageHeight = 288;
            } else if (mOralMajor == 3 || mOralMajor == 6 || mOralMajor == 9) {
                mImageWidth = 344;
                mImageHeight = 417;
            } else if (mOralMajor == 8) {
                mImageWidth = 288;
                mImageHeight = 423;
            }
        }
        qDebug() << (QString("GenCalibrationFile,width:%1, height:%2")
                         .arg(mImageWidth)
                         .arg(mImageHeight)
                         .toStdString());
        int res = algorithmic->GenerateCorrectionFile(mPreCalFolder + "/" + _file,
                                                      targetFile,
                                                      mImageWidth,
                                                      mImageHeight,
                                                      mImageBit);

        if (res == 0) {
            qDebug() << ("End generating calibration file Successful");
            ui->mLblStatusInfo->setText(tr("Calibration completed."));
        } else {
            qDebug() << ("End generating calibration file Failed");
            ui->mLblStatusInfo->setText(tr("Calibration failed.."));
        }
    }
}

void mvsystemsettings::SaveRawImage(const HWImageData& data)
{

}

// 状态信息显示
void mvsystemsettings::updateInfoPanel(const QString& message, int type)
{
    if (message.isEmpty()) {
        ui->mLblStatusInfo->setText("");
        return;
    }

    QString fullMessage = message;

    // 限制消息长度，避免显示过长
    if (fullMessage.length() > 100) {
        fullMessage = fullMessage.left(97) + "...";
    }

    QString color;
    QFont font = ui->mLblStatusInfo->font();

    switch (type) {
    case Normal:
        color = "gray";
        font.setBold(false);
        break;
    case Warning:
        color = "orange";
        font.setBold(true);
        break;
    case Error:
        color = "red";
        font.setBold(true);
        break;
    }

    QString style
        = QString("color: %1; font-weight: %2;").arg(color).arg(font.bold() ? "bold" : "normal");

    ui->mLblStatusInfo->setStyleSheet(style);
    ui->mLblStatusInfo->setFont(font);
    ui->mLblStatusInfo->setText(fullMessage);

    qDebug() << fullMessage;
    // 如果是普通信息，15秒后自动清除
    if (type == Normal) {
        mInfoTimer->start(15000);
    } else if (type == Warning) {
        mInfoTimer->start(10000);
    }
}

void mvsystemsettings::updateDeviceState(ExposureState state)
{
    QString text;
    QString style;
    m_exposureState = state;

    switch (state) {
    case ExposureState::Idle:
        text = "空闲";
        style = "color: gray; font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::SettingUp:
        text = "准备中";
        style = "color: orange;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Exposing:
        text = "曝光中";
        style = "color: yellow;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Acquiring:
        text = "采集中";
        style = "color: blue;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Processing:
        text = "处理中";
        style = "color: purple;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Completed:
        text = "完成";
        style = "color: green;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Fault:
        text = "故障";
        style = "color: red;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Timeout:
        text = "超时";
        style = "color: red;font-weight: bold; font-size: 25px;";
        break;
    }

    ui->mLblState->setStyleSheet(style);
    ui->mLblState->setText(text);
}

// 计时器槽函数
void mvsystemsettings::onInfoTimerTimeout()
{
    ui->mLblStatusInfo->setText("");
}

void mvsystemsettings::onAutoExposureTimeout()
{
    StartExposure();
}

void mvsystemsettings::onImageReceived(const QVector<HWImageData>& images)
{
    AcceptImage(images);
}

void mvsystemsettings::onInfoReceived(const QString& message)
{
    updateInfoPanel(message, Normal);
}

void mvsystemsettings::onWarningReceived(const QString& message)
{
    updateInfoPanel("警告: " + message, Warning);
}

void mvsystemsettings::onErrorReceived(const QString& message)
{
    updateInfoPanel("错误: " + message, Error);

    // 更新状态但不阻塞
    updateDeviceState(ExposureState::Fault);

    // 延迟显示错误对话框，避免立即阻塞
    QTimer::singleShot(100, [this, message]() {
        if (this->isVisible()) {
        }
    });
}

void mvsystemsettings::onNotificationReceived(const QString& message)
{
    // 通知信息，显示较短时间
    updateInfoPanel("通知: " + message, Normal);

    // 5秒后自动清除通知
    QTimer::singleShot(5000, [this]() {
        if (ui->mLblStatusInfo->text().contains("通知:")) {
            updateInfoPanel("", Normal);
        }
    });
}

void mvsystemsettings::onExposureFinished()
{
    qDebug() << "曝光完成，重置UI并准备下一次曝光";

    // 重置UI状态
    resetUI();

    QTimer::singleShot(300, this, [this]() {
        // 确保UI已重置后再检查
        if (!mIsExposing) {
            AutoNextExposure();
        }
    });
}

void mvsystemsettings::onExposureError(const QString& error)
{
    updateInfoPanel("曝光错误: " + error, Error);
    updateDeviceState(ExposureState::Fault);
}

void mvsystemsettings::onExposureProcess(ExposureState state)
{
    updateInfoPanel("", Normal);
    updateDeviceState(state);
}

void mvsystemsettings::onImagesReady(const QVector<HWImageData>& images)
{
    updateInfoPanel(QString("收到 %1 张图像").arg(images.size()), Normal);
    updateDeviceState(ExposureState::Processing);

    // 延迟处理，确保状态已更新
    QTimer::singleShot(100, [this, images]() { onImageReceived(images); });
}

void mvsystemsettings::refreshComPorts()
{
    // 暂时断开信号连接，避免刷新时触发保存
    bool saveTimerWasActive = false;
    if (mSaveSettingsTimer && mSaveSettingsTimer->isActive()) {
        mSaveSettingsTimer->stop();
        saveTimerWasActive = true;
    }

    // 保存当前选择
    QString currentSelections[4];
    currentSelections[0] = ui->mCbComSensor1->currentData().toString();
    currentSelections[1] = ui->mCbComSensor2->currentData().toString();
    currentSelections[2] = ui->mCbComXray1->currentData().toString();
    currentSelections[3] = ui->mCbComXray2->currentData().toString();

    // 阻塞信号，避免触发保存
    ui->mCbComSensor1->blockSignals(true);
    ui->mCbComSensor2->blockSignals(true);
    ui->mCbComXray1->blockSignals(true);
    ui->mCbComXray2->blockSignals(true);

    // 清空所有下拉框
    ui->mCbComSensor1->clear();
    ui->mCbComSensor2->clear();
    ui->mCbComXray1->clear();
    ui->mCbComXray2->clear();

    // 添加一个空选项
    ui->mCbComSensor1->addItem("", "");
    ui->mCbComSensor2->addItem("", "");
    ui->mCbComXray1->addItem("", "");
    ui->mCbComXray2->addItem("", "");

    // 获取所有可用串口
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    // 按照端口号排序（COM1, COM2, COM3...）
    std::sort(ports.begin(), ports.end(), [](const QSerialPortInfo& a, const QSerialPortInfo& b) {
        return a.portName() < b.portName();
    });

    // 添加到所有下拉框
    for (const QSerialPortInfo& port : ports) {
        QString portName = port.portName();
        qDebug() << "==============" + portName;
        QString displayText = "/dev/" + portName;

        // 添加项，存储短格式端口名
        ui->mCbComSensor1->addItem(displayText, displayText);
        ui->mCbComSensor2->addItem(displayText, displayText);
        ui->mCbComXray1->addItem(displayText, displayText);
        ui->mCbComXray2->addItem(displayText, displayText);
    }

    // 恢复之前的选择
    selectPortInComboBox(ui->mCbComSensor1, currentSelections[0]);
    selectPortInComboBox(ui->mCbComSensor2, currentSelections[1]);
    selectPortInComboBox(ui->mCbComXray1, currentSelections[2]);
    selectPortInComboBox(ui->mCbComXray2, currentSelections[3]);

    // 恢复信号连接
    ui->mCbComSensor1->blockSignals(false);
    ui->mCbComSensor2->blockSignals(false);
    ui->mCbComXray1->blockSignals(false);
    ui->mCbComXray2->blockSignals(false);

    // 连接ComboBox的变化信号
    connect(ui->mCbComSensor1,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &mvsystemsettings::onComboBoxChanged);
    connect(ui->mCbComSensor2,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &mvsystemsettings::onComboBoxChanged);
    connect(ui->mCbComXray1,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &mvsystemsettings::onComboBoxChanged);
    connect(ui->mCbComXray2,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &mvsystemsettings::onComboBoxChanged);

    // 如果之前保存定时器在运行，重新启动它
    if (saveTimerWasActive) {
        onComboBoxChanged();
    }

    qDebug() << "串口列表已刷新，找到" << ports.size() << "个串口";
}

void mvsystemsettings::loadSelectedPorts()
{
    // 从配置文件读取保存的端口并设置选中
    QString sensor1Port = ConfigFileManager::getInstance()->getValue("sensor1/value");
    QString sensor2Port = ConfigFileManager::getInstance()->getValue("sensor2/value");
    QString xray1Port = ConfigFileManager::getInstance()->getValue("xray1/value");
    QString xray2Port = ConfigFileManager::getInstance()->getValue("xray2/value");

    //确保保存的端口名有前缀
    auto ensurePrefix = [](const QString& port) -> QString {
        if (port.isEmpty())
            return port;
        if (!port.startsWith("/dev/")) {
            return "/dev/" + port;
        }
        return port;
    };

    sensor1Port = ensurePrefix(sensor1Port);
    sensor2Port = ensurePrefix(sensor2Port);
    xray1Port = ensurePrefix(xray1Port);
    xray2Port = ensurePrefix(xray2Port);

    // 在对应下拉框中查找并选中
    selectPortInComboBox(ui->mCbComSensor1, sensor1Port);
    selectPortInComboBox(ui->mCbComSensor2, sensor2Port);
    selectPortInComboBox(ui->mCbComXray1, xray1Port);
    selectPortInComboBox(ui->mCbComXray2, xray2Port);

    // 设置方向
    int sensor1Orientation
        = ConfigFileManager::getInstance()->getValue("sensor1/orientation").toInt();
    int sensor2Orientation
        = ConfigFileManager::getInstance()->getValue("sensor2/orientation").toInt();

    if (sensor1Orientation == 0) {
        ui->mRBtnSensorOri1LR->setChecked(true);
    } else {
        ui->mRBtnSensorOri1UpDown->setChecked(true);
    }

    if (sensor2Orientation == 0) {
        ui->mRBtnSensorOri2LR->setChecked(true);
    } else {
        ui->mRBtnSensorOri2UpDown->setChecked(true);
    }

    qDebug() << "从配置文件加载端口设置完成";
}

void mvsystemsettings::selectPortInComboBox(QComboBox* comboBox, const QString& portName)
{
    QString shortPortName = portName;

    // 转换为短格式
    if (shortPortName.startsWith("/dev/")) {
        shortPortName = shortPortName.mid(5);
    }

    if (shortPortName.isEmpty()) {
        comboBox->setCurrentIndex(0);
        return;
    }

    // 在下拉项中查找（考虑两种格式）
    for (int i = 0; i < comboBox->count(); i++) {
        QString itemData = comboBox->itemData(i).toString();
        QString shortItemData = itemData;

        if (shortItemData.startsWith("/dev/")) {
            shortItemData = shortItemData.mid(5);
        }

        // 匹配短格式
        if (shortItemData == shortPortName) {
            comboBox->setCurrentIndex(i);
            return;
        }
    }

    // 如果没找到，添加短格式
    comboBox->addItem(shortPortName, shortPortName);
    comboBox->setCurrentIndex(comboBox->count() - 1);
}

void mvsystemsettings::onComboBoxChanged()
{
    // 使用定时器实现防抖，避免频繁保存
    if (mSaveSettingsTimer && mSaveSettingsTimer->isActive()) {
        mSaveSettingsTimer->stop();
    }

    // 延迟500ms后保存，用户快速连续操作时只会保存最后一次
    mSaveSettingsTimer->start(500);
}

void mvsystemsettings::savePortSettings()
{
    try {
        // 获取当前选择的端口
        QString sensor1Port = ui->mCbComSensor1->currentData().toString();
        QString sensor2Port = ui->mCbComSensor2->currentData().toString();
        QString xray1Port = ui->mCbComXray1->currentData().toString();
        QString xray2Port = ui->mCbComXray2->currentData().toString();

        // 获取方向设置
        int sensor1Orientation = ui->mRBtnSensorOri1LR->isChecked() ? 0 : 1;
        int sensor2Orientation = ui->mRBtnSensorOri2LR->isChecked() ? 0 : 1;

        ConfigFileManager::getInstance()->setValue("sensor1/description",
                                                   ui->mRBtnSensor1->isChecked() ? "true" : "false");
        ConfigFileManager::getInstance()->setValue("sensor2/description",
                                                   ui->mRBtnSensor2->isChecked() ? "true" : "false");

        // 保存到配置文件
        ConfigFileManager::getInstance()->setValue("sensor1/value", sensor1Port);
        ConfigFileManager::getInstance()->setValue("sensor2/value", sensor2Port);
        ConfigFileManager::getInstance()->setValue("xray1/value", xray1Port);
        ConfigFileManager::getInstance()->setValue("xray2/value", xray2Port);

        ConfigFileManager::getInstance()->setValue("sensor1/orientation",
                                                   QString::number(sensor1Orientation));
        ConfigFileManager::getInstance()->setValue("sensor2/orientation",
                                                   QString::number(sensor2Orientation));
        if (true) {
            // 给用户一个视觉反馈
            QString feedback = QString("配置已自动保存");

            updateInfoPanel(feedback, Normal);

        } else {
            updateInfoPanel("配置保存失败", Error);
        }

    } catch (const std::exception& e) {
        updateInfoPanel(QString("保存错误: %1").arg(e.what()), Error);
    }
}

void mvsystemsettings::checkPortChanges()
{
    // 获取当前端口列表
    static QSet<QString> lastPorts;
    QSet<QString> currentPorts;

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const auto& port : ports) {
        currentPorts.insert(port.portName());
    }

    // 如果端口发生变化
    if (lastPorts != currentPorts) {
        lastPorts = currentPorts;

        // 只在界面可见时更新
        if (this->isVisible()) {
            refreshComPorts();
            qDebug() << "检测到串口变化，已更新列表";
        }
    }
}
