#include "CalibrationDialog.h"
#include "ui_CalibrationDialog.h"

#include <QDateTime>
#include <QDir>
#include <QPainter>
#include <QPointer>
#include "XV.Control/SignalWaiter.h"

#include "XV.Tool/WidgetHelper.h"

CalibrationDialog::CalibrationDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CalibrationDialog)
{
    ui->setupUi(this);
    qDebug() << ("CalibrationDialog init");
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    raise();

    ui->labelWarn->setText(tr("Please use 1mm aluminum plate for calibration"));
    ui->labelStatusInfo->setText("");
    ui->labelImageShow_1->setText("1");
    ui->labelImageShow_2->setText("2");
    ui->labelImageShow_3->setText("3");
    ui->labelImageShow_4->setText("4");
    ui->labelImageShow_5->setText("5");
    ui->labelImageShow_6->setText("6");
    ui->labelImageShow_7->setText("7");
    ui->labelImageShow_8->setText("8");

    connect(ui->pushButtonCancel, &QPushButton::clicked, this, [this]() {
        ui->pushButtonCancel->setVisible(false);
        slotStatus(0);
        mEnableExposure = false;
        mEnableContinue = false;
    });
    connect(ui->pushButtonEnable, &QPushButton::clicked, this, &CalibrationDialog::SetupWork);

    ui->pushButtonCancel->setVisible(false);
    ui->pushButtonGenCaliFile->setAutoDefault(true);
    ui->pushButtonEnable->setAutoDefault(true);
    ui->pushButtonEnable->setDefault(true);
    ui->pushButtonCancel->setAutoDefault(false);
    InitDetectorConnection();

    connect(ui->pushButtonGenCaliFile,
            &QPushButton::clicked,
            this,
            &CalibrationDialog::GenCalibrationFile);
    connect(this,
            &CalibrationDialog::singleGenCalibrationFile,
            this,
            &CalibrationDialog::GenCalibrationFile);

    mPreCalFolder = qApp->applicationDirPath() + "/Cal/pre"
                    + QDateTime::currentDateTime().toString("yyyyMMddTHHmmss") + "/";
    // mPreCalFolder = "/home/pi/amoswork/XVDentalQt/XVDental/build/Desktop-Debug/Cal/pre20250808T092631/";
    QDir dir;
    dir.mkpath(mPreCalFolder);

    ui->pushButtonKVAdd->setAutoDefault(false);
    ui->pushButtonKVDes->setAutoDefault(false);
    ui->pushButtonSecAdd->setAutoDefault(false);
    ui->pushButtonSecDes->setAutoDefault(false);

    QList<qreal> list = GetExposureTimeParam();
    if (list.size() > mSecIndex) {
        ui->lineEditSec->setText(QString::number(list.at(mSecIndex)) + "s");
    } else {
        ui->lineEditSec->setText(QString::number(list.at(0)) + "s");
    }
    ui->lineEditKV->setText(QString::number(mKVValue) + "kV");

    connect(ui->pushButtonKVAdd, &QPushButton::clicked, this, [this]() {
        if (mKVValue < 70) {
            ++mKVValue;
            ui->lineEditKV->setText(QString::number(mKVValue) + "kV");
            qDebug() << (std::string("SetExposureKv:") + std::to_string(mKVValue));
            emit signalSetExposureKv(mKVValue);
        }
    });
    connect(ui->pushButtonKVDes, &QPushButton::clicked, this, [this]() {
        if (mKVValue > 40) {
            --mKVValue;
            ui->lineEditKV->setText(QString::number(mKVValue) + "kV");
            qDebug() << (std::string("SetExposureKv:") + std::to_string(mKVValue));
            emit signalSetExposureKv(mKVValue);
        }
    });
    connect(ui->pushButtonSecAdd, &QPushButton::clicked, this, [this]() {
        QList<qreal> list = GetExposureTimeParam();
        if (mSecIndex < list.size()) {
            mSecIndex++;
            ui->lineEditSec->setText(QString::number(list.at(mSecIndex)) + "s");
        }
    });
    connect(ui->pushButtonSecDes, &QPushButton::clicked, this, [this]() {
        if (mSecIndex > 0) {
            mSecIndex--;
            QList<qreal> list = GetExposureTimeParam();
            ui->lineEditSec->setText(QString::number(list.at(mSecIndex)) + "s");
        }
    });

    WidgetHelper widgetHelper;
    widgetHelper.IncreaseAllChildWidgetsFontSize(this, 2);
}

CalibrationDialog::~CalibrationDialog()
{
    QObject::disconnect(nullptr, nullptr, this, nullptr);
    delete ui;
    qDebug() << ("CalibrationDialog close");
}

void CalibrationDialog::SetupWork()
{ // todo com is connection?
    emit signalSetXRay(mKVValue, mSecIndex, mUAValue);
    XVDetectorWorker *worker = XVDetectorWorker::Instance();
    qDebug() << ("CalibrationDialog::SetupWork");
    if (worker->IsConnectionCom()) {
        if (!mEnableExposure) {
            slotStatus(3);
            mEnableExposure = true;
            mEnableContinue = true;
            emit signalSetupCalibration();
        }
    } else {
        // todo Msg_Cal_Import_NoSensorPluggedin
        ui->labelStatusInfo->setText(tr("Make sure the sensor was plugged in properly!"));
    }
}

void CalibrationDialog::ImageDataReceived(int width, int height, int bit, QString filename)
{
    {
        mEnableExposure = false;
    }
    mImageWidth = width;
    mImageHeight = height;
    QDateTime now = QDateTime::currentDateTime();
    qDebug() << (QString("ImageDataReceived width:%1, height:%2, bit:%3")
                     .arg(width)
                     .arg(height)
                     .arg(bit)
                     .toStdString());
    QString destination = mPreCalFolder + now.toString("yyyyMMddTHHmmss") + ".raw";
    // log
    qDebug() << (QString("filename:%1").arg(destination).toStdString());
    bool success = QFile::copy(filename, destination);
    if (success) {
        slotStatus(0);
        ShowImage();
        // ShowImage(width, height, bit, filename);
        mCalibrationRawFiles[mCurrentIndex] = destination;
        mCurrentIndex++;
        if (mCurrentIndex > 4 && mCalibrationRawFiles.size() < 4) {
            QList<int> keys = mCalibrationRawFiles.keys();
            for (int var = 1; var <= keys.size(); ++var) {
                if (!keys.contains(var)) {
                    mCurrentIndex = var;
                    break;
                }
            }
        }
        if (mCalibrationRawFiles.size() != 4) {
            // todo wait 4 sec
            QPointer<CalibrationDialog> guard(this);
            QTimer::singleShot(3000, this, [guard]() {
                if (!guard) {
                    qDebug() << ("CalibrationDialog distoryed");
                    return;
                }
                if (guard.data()->mEnableContinue && !guard.data()->mEnableExposure) {
                    emit guard.data()->signalSetupWork();
                }
            });
        } else {
            emit singleGenCalibrationFile();
        }
    } else {
        ui->labelStatusInfo->setText(tr("copy failed"));
    }
}

void CalibrationDialog::DataReceivedError(QString err)
{
    qDebug() << (QString("error:%1").arg(err).toStdString());
    ui->labelStatusInfo->setText(XVDetectorError::ErrorString(err));
    slotStatus(0);
    mEnableExposure = false;
    mEnableContinue = false;
}

void CalibrationDialog::slotDataReceivedFirmwareID(int oralMajor,
                                                   int oralMinor,
                                                   int oralPatch,
                                                   QString addr)
{
    mOralMajor = oralMajor;
    mOralMinor = oralMinor;
    mOralPatch = oralPatch;
    mOralAddr = addr;
}

void CalibrationDialog::slotStatus(uint status)
{
    try {
        switch (status) {
        case 1:
            ui->widgetExposureStatus->setStyleSheet("background-color: #008000;");
            ui->labelStatus->setText(tr("Ready"));
            ui->pushButtonCancel->setVisible(true);
            break;
        case 2:
            ui->widgetExposureStatus->setStyleSheet("background-color: #FFFF00;");
            ui->labelStatus->setText(tr("Acquiring"));
            break;
        case 3:
            ui->labelStatus->setText(tr("Preparing"));
            ui->labelStatusInfo->setText("");
            break;
        default:
            ui->widgetExposureStatus->setStyleSheet("background-color: #C3C3C3;");
            ui->labelStatus->setText(tr("Standby"));
            ui->pushButtonCancel->setVisible(false);
            qDebug() << (QString("Standby%1").arg(ui->labelStatus->text()).toStdString());
            break;
        }
    } catch (std::exception ex) {
        qDebug() << (std::string("status exception:") + ex.what());
    }
}

void CalibrationDialog::GenCalibrationFile()
{
    // QDateTime now = QDateTime::currentDateTime();
    qDebug() << (QString("GenCalibrationFile origin:%1").arg(mPreCalFolder).toStdString());
    QDir directory(mPreCalFolder);
    directory.setFilter(QDir::Files);
    QFileInfoList fileList = directory.entryInfoList();
    if (fileList.size() == 0) {
        // todo Tip_CorrectImage_NoFoundRaws
        // QMessageBox::warning(this, "Warning", "No valid data. Calibration suspended.");
        qDebug() << ("No valid data. Calibration suspended.");
        ui->labelStatusInfo->setText(tr("No valid data. Calibration suspended."));
        return;
    }

    auto algorithmic = XPectAlgorithmic::Instance();
    QString targetFile = qApp->applicationDirPath() + "/Cal/" + QString::number(mOralMajor) + ".0/"
                         + mOralAddr;
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
    int res = algorithmic->GenerateCorrectionFile(mPreCalFolder,
                                                  targetFile,
                                                  mImageWidth,
                                                  mImageHeight,
                                                  mImageBit);

    if (res == 0) {
        qDebug() << ("End generating calibration file Successful");
        // todo Tip_CorrectImage_ResponseOK
        // QMessageBox::information(this, "Info", "Calibration completed.");
        ui->labelStatusInfo->setText(tr("Calibration completed."));
    } else {
        // todo Tip_CorrectImage_ResponseFailure
        qDebug() << ("End generating calibration file Failed");
        // QMessageBox::warning(this, "Warning", "Calibration failed..");
        ui->labelStatusInfo->setText(tr("Calibration failed.."));
    }
}

void CalibrationDialog::CalibrationSetCompleted(bool ok)
{
    if (ok) {
        emit signalSetupWork();
    } else {
        mEnableExposure = false;
        mEnableContinue = false;
        slotStatus(0);
    }
}

void CalibrationDialog::XRayResultReceived(QString cmd, int result, QString error)
{
    if (result == 0) {
        ui->labelStatusInfo->setText(tr("X-ray was not connected"));
    } else {
        if (result != 1) {
            qDebug() << (QString("%1, error: %2").arg(cmd).arg(error).toStdString());
        }
        if (cmd == "CMD_EXP_TIME") {
            ui->labelStatusInfo->setText(result == 1 ? "" : tr("SetExposureTimeMs:failure"));
        } else if (cmd == "CMD_EXP_KV") {
            ui->labelStatusInfo->setText(result == 1 ? "" : tr("SetExposureKv:failure"));
        } else if (cmd == "CMD_EXP_MA") {
            ui->labelStatusInfo->setText(result == 1 ? "" : tr("SetExposureMa:failure"));
        } else if (cmd == "CMD_STAT") {
            // ui->labelStatusInfo->setText(result == 1 ? "" : tr("CMD_GET_EXP_DEV_STA:failure"));
        } else if (cmd == "DONE") {
            ui->labelStatusInfo->setText("");
        }
    }
}

void CalibrationDialog::InitXRayConnection()
{
    XRayWrokerQF *worker = XRayWrokerQF::Instance();

    QObject::connect(worker,
                     &XRayWrokerQF::signalResult,
                     this,
                     &CalibrationDialog::XRayResultReceived);

    QObject::connect(this, &CalibrationDialog::signalSetXRay, worker, &XRayWrokerQF::SetXRay);
}

void CalibrationDialog::InitDetectorConnection()
{
    qDebug() << "CalibrationDialog::InitDetectorConnection() called";

    XVDetectorWorker *worker = XVDetectorWorker::Instance();

    // 检查 worker 是否存在
    if (!worker) {
        qWarning() << "XVDetectorWorker::Instance() returned nullptr!";
        ui->labelStatusInfo->setText(tr("Detector worker not initialized"));
        return;
    }

    qDebug() << "XVDetectorWorker instance:" << worker;

    // 检查 GetProtocol() 是否返回有效指针
    XVDetectorProtocol *protocol = worker->GetProtocol();
    if (!protocol) {
        qWarning() << "worker->GetProtocol() returned nullptr!";
        ui->labelStatusInfo->setText(tr("Detector protocol not initialized"));
        return;
    }

    qDebug() << "XVDetectorProtocol instance:" << protocol;

    QObject::connect(this,
                     &CalibrationDialog::signalSetupCalibration,
                     worker->GetProtocol(),
                     &XVDetectorProtocol::SetupCalibration);
    QObject::connect(worker->GetProtocol(),
                     &XVDetectorProtocol::sinalCalibrationSetCompleted,
                     this,
                     &CalibrationDialog::CalibrationSetCompleted);
    QObject::connect(this,
                     &CalibrationDialog::signalSetupWork,
                     worker->GetProtocol(),
                     &XVDetectorProtocol::SetupWork);

    QObject::connect(worker->GetProtocol(),
                     &XVDetectorProtocol::signalImageDataReceived,
                     this,
                     &CalibrationDialog::ImageDataReceived);
    QObject::connect(worker->GetProtocol(),
                     &XVDetectorProtocol::signalDataReceivedError,
                     this,
                     &CalibrationDialog::DataReceivedError);
    QObject::connect(worker->GetProtocol(),
                     &XVDetectorProtocol::signalFirmwareVerID,
                     this,
                     &CalibrationDialog::slotDataReceivedFirmwareID);

    worker->GetProtocol()->GetFirmwareVerID(mOralMajor, mOralAddr);
    qDebug() << (QString("InitDetectorConnection:%1, %2")
                     .arg(mOralMajor)
                     .arg(mOralAddr)
                     .toStdString());

    QObject::connect(worker->GetProtocol(),
                     &XVDetectorProtocol::signalStatus,
                     this,
                     &CalibrationDialog::slotStatus);

    connect(ui->pushButtonCancel,
            &QPushButton::clicked,
            worker->GetProtocol(),
            &XVDetectorProtocol::StopWork);
}

void CalibrationDialog::ShowImage()
{
    QLabel *label;
    switch (mCurrentIndex) {
    case 1:
        label = ui->labelImageShow_1;
        break;
    case 2:
        label = ui->labelImageShow_2;
        break;
    case 3:
        label = ui->labelImageShow_3;
        break;
    case 4:
        label = ui->labelImageShow_4;
        break;
    case 5:
        label = ui->labelImageShow_5;
        break;
    case 6:
        label = ui->labelImageShow_6;
        break;
    case 7:
        label = ui->labelImageShow_7;
        break;
    case 8:
        label = ui->labelImageShow_8;
        break;
    default:
        break;
    }

    // 创建一个xxx像素大小的 QPixmap
    srand(time(0));
    int width = label->width() - 6;
    int height = label->height() - 6;
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::white);
    // 开始在 pixmap 上绘图
    QPainter painter(&pixmap);
    // 遍历每个像素，设置随机灰度值
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int grayValue = rand() % 256;
            QRgb color = qRgb(grayValue, grayValue, grayValue);
            painter.setPen(color);
            painter.drawPoint(x, y);
        }
    }
    painter.end();
    // show
    label->setText("");
    label->setPixmap(pixmap);
    label->setScaledContents(true);
    label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
}

void CalibrationDialog::ShowImage(int width, int height, int bit, QString filename)
{
    auto algorithmic = XPectAlgorithmic::Instance();
    qint64 filesize = 0;
    std::unique_ptr<unsigned char[]> rawdata = algorithmic->ReadRawFile(filename, &filesize);
    const int bytesPerLine = width * 2;
    // QImage::Format format = QImage::Format_Grayscale16;
    QImage qimage(reinterpret_cast<const unsigned char *>(rawdata.get()),
                  width,
                  height,
                  bytesPerLine,
                  QImage::Format_Grayscale16);
    QPixmap pixmap = QPixmap::fromImage(qimage);
    if (!pixmap.isNull()) {
        QLabel *label;
        switch (mCurrentIndex) {
        case 1:
            label = ui->labelImageShow_1;
            break;
        case 2:
            label = ui->labelImageShow_2;
            break;
        case 3:
            label = ui->labelImageShow_3;
            break;
        case 4:
            label = ui->labelImageShow_4;
            break;
        case 5:
            label = ui->labelImageShow_5;
            break;
        case 6:
            label = ui->labelImageShow_6;
            break;
        case 7:
            label = ui->labelImageShow_7;
            break;
        case 8:
            label = ui->labelImageShow_8;
            break;
        default:
            break;
        }
        label->setText("");
        label->setPixmap(pixmap.scaled(label->width(), label->height(), Qt::KeepAspectRatio));
        label->setScaledContents(true);
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    }
}

void CalibrationDialog::showEvent(QShowEvent *event)
{
    // showFullScreen();
    slotStatus(0);
    QWidget::showEvent(event);
}
