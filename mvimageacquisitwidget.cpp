#include "mvimageacquisitwidget.h"
#include "ui_mvimageacquisitwidget.h"

#include <algorithm>

#include <QDesktopServices>
#include <QImageReader>
#include <QList>
#include <QMouseEvent>
#include <QObject>
#include <QPdfWriter>
#include <QSortFilterProxyModel>
#include <QTextBlock>
#include <QTextBlockUserData>
#include <QtCore/QString>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QScroller>
#include <cxxabi.h>
#include <execinfo.h>

#include <QGuiApplication>
#include <QScreen>

#include <iostream>
#include <string.h>

namespace mv {

mvImageAcquisitWidget::mvImageAcquisitWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::mvImageAcquisitWidget)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setObjectName("mvImageAcquisitWidget");

    initView();
    initConnect();
    initializePreviewFrame();
    initComm();

    // Create detector
    QString model_path = DataLocations::getRootConfigPath()
                         + "/model/DNUNetRes_NB_Nature_tiny.onnx";
    detector = std::make_unique<CableDetector>(0.7, 10, "onnx", model_path.toStdString());
}

mvImageAcquisitWidget::~mvImageAcquisitWidget() {}

void mvImageAcquisitWidget::initView()
{
    s1 = new SliderStyle();
    s2 = new SliderStyle();

    ui->mWWVerticalSlider->setSingleStep(1);
    ui->mWLVerticalSlider->setSingleStep(1);

    ui->mWWVerticalSlider->setStyle(s1);
    ui->mWLVerticalSlider->setStyle(s2);

    ui->mWWVerticalSlider->setRange(1, 15);
    ui->mWLVerticalSlider->setRange(1, 15);

    ui->pushButtonFinished->setEnabled(false);
    ui->pushButtonStart->setEnabled(true);

    ui->mFrmThumImag->setVisible(false);
    ui->mWWWLFrm->setVisible(false);
}

void mvImageAcquisitWidget::initConnect()
{
    mManager = XProtocolManager::getInstance();
    connect(mManager, &XProtocolManager::info, this, &mvImageAcquisitWidget::onInfoReceived);
    connect(mManager, &XProtocolManager::warning, this, &mvImageAcquisitWidget::onWarningReceived);
    connect(mManager,
            &XProtocolManager::errorOccurred,
            this,
            &mvImageAcquisitWidget::onErrorReceived);
    connect(mManager,
            &XProtocolManager::notification,
            this,
            &mvImageAcquisitWidget::onNotificationReceived);
    connect(mManager,
            &XProtocolManager::exposureError,
            this,
            &mvImageAcquisitWidget::onExposureError);

    // 连接图像接收信号
    connect(mManager, &XProtocolManager::imagesReady, this, &mvImageAcquisitWidget::onImagesReady);
    connect(mManager,
            &XProtocolManager::exposureProcess,
            this,
            &mvImageAcquisitWidget::onExposureProcess);

    connect(ui->mWWVerticalSlider,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(slotWWVSliderValueChanged(int)));
    connect(ui->mWLVerticalSlider,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(slotWLVSliderValueChanged(int)));
    // connect(ui->mWWVerticalSlider, SIGNAL(valueChanged(int)), s1, SLOT(setValue(int)));
    // connect(ui->mWLVerticalSlider, SIGNAL(valueChanged(int)), s2, SLOT(setValue(int)));
    connect(ui->mBtnWWAdd, SIGNAL(clicked()), this, SLOT(onWWAddClicked()));
    connect(ui->mBtnWWSub, SIGNAL(clicked()), this, SLOT(onWWSubClicked()));
    connect(ui->mBtnWLAdd, SIGNAL(clicked()), this, SLOT(onWLAddClicked()));
    connect(ui->mBtnWLSub, SIGNAL(clicked()), this, SLOT(onWLSubClicked()));

    connect(ui->pushButtonAddView, SIGNAL(clicked()), this, SLOT(onAddViewButton()));
    connect(ui->pushButtonDeleteView, SIGNAL(clicked()), this, SLOT(onDeleteViewButton()));
    connect(ui->pushButtonFinished, SIGNAL(clicked()), this, SLOT(onFinishedButton()));

    connect(ui->pushButtonStart, SIGNAL(clicked()), this, SLOT(onStartButton()));
}

void mvImageAcquisitWidget::initializePreviewFrame()
{
    ui->mWWVerticalSlider->setRange(1, 100);
    double targetZoom = 1.5;
    int sliderValue = 1 + (targetZoom - 0.1) * 49 / 4.9;
    sliderValue = qBound(1, sliderValue, 50);
    ui->mWWVerticalSlider->setValue(sliderValue);

    // 初始化但不显示任何内容
    ui->mPreviewFrame->reset(); // 重置为不显示状态

    // 只设置参数，但不触发显示
    ui->mPreviewFrame->setInnerEllipseCenter(QPointF(1, -1));
    ui->mPreviewFrame->setInnerEllipseMinorRadius(12.0);
    ui->mPreviewFrame->setInnerEllipseMajorRadius(13.0);
    ui->mPreviewFrame->setOuterEllipseMinorRadius(26);
    ui->mPreviewFrame->setOuterEllipseMajorRadius(29);

    // 设置坐标系范围但不显示
    ui->mPreviewFrame->setAxisRange(15.0);

    // 重置为不显示
    ui->mPreviewFrame->setWidgetVisible(false);

    ui->mCurveFrame->setMaxDataPoints(100); // 显示最近100条记录
    ui->mCurveFrame->setAutoYAxis(true);    // Y轴自动缩放
    ui->mCurveFrame->reset();
}

void mvImageAcquisitWidget::initComm() {}

void mvImageAcquisitWidget::onWWAddClicked()
{
    int _step = 1;
    int currentValue = ui->mWWVerticalSlider->value();
    int maxValue = ui->mWWVerticalSlider->maximum();

    // 检查是否超过最大值
    int ww = currentValue + _step;
    if (ww > maxValue) {
        ww = maxValue; // 不超过最大值
    }

    if (s1) {
        s1->setValue(ww);
    }
    ui->mWWVerticalSlider->setValue(ww);
}

void mvImageAcquisitWidget::onWWSubClicked()
{
    int _step = 1;
    int currentValue = ui->mWWVerticalSlider->value();
    int minValue = ui->mWWVerticalSlider->minimum();

    // 检查是否低于最小值
    int ww = currentValue - _step;
    if (ww < minValue) {
        ww = minValue; // 不低于最小值
    }

    if (s1) {
        s1->setValue(ww);
    }
    ui->mWWVerticalSlider->setValue(ww);
}

void mvImageAcquisitWidget::onWLAddClicked()
{
    int _step = 1;
    int wl = ui->mWLVerticalSlider->value() + _step;
    s2->setValue(wl);
    ui->mWLVerticalSlider->setValue(wl);
    ui->mWLVerticalSlider->update();
}
void mvImageAcquisitWidget::onWLSubClicked()
{
    int _step = 1;
    int wl = ui->mWLVerticalSlider->value() - _step;
    s2->setValue(wl);
    ui->mWLVerticalSlider->setValue(wl);
    ui->mWLVerticalSlider->update();
}

void mvImageAcquisitWidget::slotWWVSliderValueChanged(int value)
{
    int minSlider = ui->mWWVerticalSlider->minimum();
    int maxSlider = ui->mWWVerticalSlider->maximum();

    double minZoom = 0.1;
    double maxZoom = 5.0;

    // 线性计算缩放因子
    double zoomFactor = minZoom
                        + (maxZoom - minZoom) * (value - minSlider) / (maxSlider - minSlider);

    ui->mPreviewFrame->setZoomFactor(zoomFactor);
}

void mvImageAcquisitWidget::slotWLVSliderValueChanged(int) {}

void mvImageAcquisitWidget::onAddViewButton()
{
    QString databaseDirectory = DataLocations::getRootConfigPath() + "/";
    QString modality = SessionHelper::getInstance()->getCurrentStudy().modality;
    QString studyUID = SessionHelper::getInstance()->getCurrentStudyUID();
    QStringList listSopUID = DicomDatabase::instance()->getSopUIDByStudyUID(studyUID);
    QString SeriesUID;
    QString SOPUID;
    int Num = 0;
    if (!listSopUID.isEmpty() && listSopUID.count() > 0) {
        QList<int> listNum;
        for (int j = 0; j < listSopUID.count(); j++) {
            QString str = listSopUID.at(j);
            QStringList list = str.split(".");

            listNum.append(list.last().toInt());
        }
        std::sort(listNum.begin(), listNum.end());
        Num = listNum.last();
        {
            QString str = listSopUID.at(0);
            QStringList list = str.split(".");
            list.removeAt(list.count() - 1);
            SOPUID = list.join(".");
            QString temp = SOPUID;
            SeriesUID = temp.replace(".103.024192077058038104.", ".102.024192077058038104.");
        }
    } else {
        QString currentDate1 = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
        SeriesUID = "1.2.156.112728.101.102.024192077058038104." + currentDate1;
        SOPUID = "1.2.156.112728.101.103.024192077058038104." + currentDate1;
    }

    QString ViewName = "";
    QString ViewDescription = "";
    QString BodyPartExamined = "";
    QString ViewPosition = "";
    QString PatientOrientation = "";
    QString ImageLaterality = "";
    QString ViewIconFile = DataLocations::getRootConfigPath() + "/image/Layer.png";
    QString ExposeStatus = "Scheduled";

    QString SeriesInstanceUID = SeriesUID;
    QString seriesNumber = "1";
    DicomDatabase::instance()->insertSeries(studyUID,
                                            SeriesInstanceUID,
                                            "",
                                            ViewDescription,
                                            modality,
                                            seriesNumber,
                                            ImageLaterality,
                                            ViewName,
                                            BodyPartExamined,
                                            ExposeStatus);

    QString SOPInstanceUID = SOPUID + "." + QString::number(Num + 1);
    DicomDatabase::instance()->insertImage(SeriesInstanceUID,
                                           SOPInstanceUID,
                                           PatientOrientation,
                                           ImageLaterality,
                                           ViewIconFile,
                                           ViewName,
                                           ViewDescription,
                                           ViewPosition,
                                           QString::number(Num + 1));

    exam_type* newExam = new exam_type();
    newExam->id = listSopUID.count() + 1;
    newExam->SeriesInstanceUID = SeriesInstanceUID;
    newExam->SeriesID = ViewName;
    newExam->SOPUID = SOPInstanceUID;
    newExam->ImageFileFullName = ViewIconFile;
    newExam->isExam = false;
    examList.push_back(newExam);

    QFile _file(ViewIconFile);

    if (_file.exists()) {
        QPixmap pix1;
        _file.open(QIODevice::ReadOnly);
        pix1.loadFromData(_file.readAll());
        QSize picSize(60, 60);

        QPixmap scaledPixmap = pix1.scaled(picSize);
        QString ret = ui->ThumbnailsWidget->GetLastThumbnailLable();
        ui->ThumbnailsWidget->addThumbnail(scaledPixmap,
                                           QString::number(ret.toInt() + 1) + "|" + ViewIconFile);
        DicomDatabase::instance()->updateImagesInstanceNumberBySOPUID(SOPInstanceUID,
                                                                      QString::number(ret.toInt()
                                                                                      + 1));
    }
}

void mvImageAcquisitWidget::onDeleteViewButton()
{
    int index = currentExamID;
    if (index >= 0 && index < examList.count()) {
        //            int result = XDMessageBox::showQuestion(nullptr,"",tr("Confirm whether to delete the Image?"),QMessageBox::Yes | QMessageBox::No);
        //            if (result != QMessageBox::Yes)
        //                return;

        if (DicomDatabase::instance()->deleteForImagebySopUID(examList.at(index)->SOPUID)) {
            ui->ThumbnailsWidget->deleteThumbnail(index);
            ui->ThumbnailsWidget->onThumbnailNotSelected();
            examList.remove(index);
            currentExamID = index;
            autoSwitchNextPosition();
        }
    } else {
        XDMessageBox::showInformation(nullptr, "", tr("Select the view you want to delete"));
    }
}

void mvImageAcquisitWidget::onFinishedButton()
{
    StopExposure();
    // emit sigReturnMainPage(0);

    // const int FIRST_PAGE = 0;
    // const int SECOND_PAGE = 1;

    // int currentIndex = ui->mStateShowStackWidget->currentIndex();
    // int nextIndex = (currentIndex == FIRST_PAGE) ? SECOND_PAGE : FIRST_PAGE;

    // ui->mStateShowStackWidget->setCurrentIndex(nextIndex);
}

void mvImageAcquisitWidget::onStartButton()
{
#ifdef QT_DEBUG
    imagePathX = "/home/pi/XVBVThiness/build/config/model/1.raw";
    imagePathY = "/home/pi/XVBVThiness/build/config/model/2.raw";
    ui->mStateShowStackWidget->setCurrentIndex(1);

    StatusCode status;
    cv::Mat lr_profile, ud_profile, lr_img, ud_img;
    MeasurementData measure_data;

    cv::Size raw_size(344, 417);
    cv::Size crop_size(304, 137);

    std::tie(status, lr_profile, ud_profile, lr_img, ud_img, measure_data) = detector->measure(
        imagePathX.toStdString(), imagePathY.toStdString(), raw_size, "uint16", crop_size, true, 3);

    // 打印状态（枚举转换为整数）
    qDebug() << "=== Measurement Results ===";
    qDebug() << "Status:" << static_cast<int>(status);

    // 打印LR profile信息（不直接遍历cv::Mat）
    qDebug() << "LR Profile - rows:" << lr_profile.rows << "cols:" << lr_profile.cols
             << "channels:" << lr_profile.channels();
    qDebug() << "LR Profile type:" << lr_profile.type();

    // 打印UD profile信息
    qDebug() << "UD Profile - rows:" << ud_profile.rows << "cols:" << ud_profile.cols
             << "channels:" << ud_profile.channels();
    qDebug() << "UD Profile type:" << ud_profile.type();

    // 打印图像信息
    qDebug() << "LR Image - rows:" << lr_img.rows << "cols:" << lr_img.cols
             << "channels:" << lr_img.channels();
    qDebug() << "UD Image - rows:" << ud_img.rows << "cols:" << ud_img.cols
             << "channels:" << ud_img.channels();

    int zoom = 1;
    ui->mPreviewFrame->setAxisRange(measure_data.outer_ellipse.x_diameter
                                            > measure_data.inner_ellipse.y_diameter
                                        ? measure_data.outer_ellipse.x_diameter * zoom
                                        : measure_data.inner_ellipse.y_diameter * zoom);
    ui->mPreviewFrame->setInnerEllipseCenter(QPointF(measure_data.inner_ellipse.x_diameter * zoom,
                                                     measure_data.inner_ellipse.y_diameter * zoom));
    ui->mPreviewFrame->setInnerEllipseMinorRadius(measure_data.inner_ellipse.y_diameter / 2 * zoom);
    ui->mPreviewFrame->setInnerEllipseMajorRadius(measure_data.inner_ellipse.x_diameter / 2 * zoom);
    ui->mPreviewFrame->setOuterEllipseMinorRadius(measure_data.outer_ellipse.x_diameter / 2 * zoom);
    ui->mPreviewFrame->setOuterEllipseMajorRadius(measure_data.outer_ellipse.y_diameter / 2 * zoom);

    ui->mPreviewFrame->setAutoMeasurementAngles(measure_data.wall_thickness.min_angle);

    if (measure_data.wall_thickness.spec_thickness.size() >= 8) {
        ui->mPreviewFrame->setMeasurementDisplayText(0,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[0])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(45,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[1])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(90,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[2])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(135,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[3])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(180,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[4])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(225,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[5])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(270,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[6])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(315,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[7])
                                                         + "mm");
    }

    ui->mCurveFrame->addData(measure_data.inner_ellipse.diameter,
                             measure_data.outer_ellipse.diameter);

#else
    ui->mStateShowStackWidget->setCurrentIndex(0);
    StartExposure();

#endif
}

void mvImageAcquisitWidget::autoSwitchNextPosition()
{
    if (currentExamID >= examList.count()) {
        currentExamID = 0;
        ui->ThumbnailsWidget->setCurrentThumbnail(currentExamID);
        return;
    }

    while (examList.at(currentExamID)->isExam) {
        if (currentExamID < examList.size() - 1) {
            currentExamID++;
        } else {
            break;
        }
    }
    ui->ThumbnailsWidget->setCurrentThumbnail(currentExamID);
}

void mvImageAcquisitWidget::setCurrentPatient()
{
    ui->mPreviewFrame->reset();
    ui->mCurveFrame->reset();

    mCurrentStudyRecord = SessionHelper::getInstance()->getCurrentStudy();
    ui->labelInnerDiameter->setText(mCurrentStudyRecord.patientBirth);
    ui->labelHotOuterDiameter->setText(mCurrentStudyRecord.age);
    ui->labelEccentric->setText(mCurrentStudyRecord.perPhysician);
    ui->labelWallThickness->setText(mCurrentStudyRecord.reqPhysician);
    ui->labelInnerDiameterTolerance->setText(mCurrentStudyRecord.procId);
    ui->labelHotOuterDiameterTolerance->setText(mCurrentStudyRecord.protocalCode);
    ui->labelEccentricTolerance->setText(mCurrentStudyRecord.modality);
    ui->labelWallThicknessTolerance->setText(mCurrentStudyRecord.protocalMeaning);

    currentPatientName = SessionHelper::getInstance()->getCurrentPatientName();
    currentStudyUID = SessionHelper::getInstance()->getCurrentStudyUID();

    qDeleteAll(examList);
    examList.clear();
    ui->ThumbnailsWidget->clearThumbnails();

    for (int i = 0; i < 2; i++) {
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
            mOralAddr = tr("Unknown"); // 默认值
        }

        if (!mOralAddr.isEmpty()) {
            AlgSetCorrectionFile(mOralMajor, mOralAddr);
        }
    }

    resetUI();

    //        this->getImagePath(examList, currentStudyUID);

    //        QString databaseDirectory = DataLocations::getRootConfigPath() + "/";
    //        int nNum = 0;
    //        for (int i = 0; i < examList.size(); i++)
    //        {
    //            QString temp = examList.at(i)->ImageFileFullName;
    //            QString tempLabel = examList.at(i)->SeriesID;
    //            QString filepath = temp.replace("\\", "/");

    //            QFile _file(filepath);

    //            if (_file.exists())
    //            {
    //                QPixmap pix1;
    //                _file.open(QIODevice::ReadOnly);
    //                pix1.loadFromData(_file.readAll());
    //                QSize picSize(60, 60);

    //                QPixmap scaledPixmap = pix1.scaled(picSize);
    //                ui->ThumbnailsWidget->addThumbnail(scaledPixmap, QString::number(examList.at(i)->id) + "|" + filepath);

    //                if (i == 0)
    //                {
    //                    ui->ThumbnailsWidget->setCurrentThumbnail(i);
    //                    currentExamID = 0;
    //                }
    //            }
    //            else
    //            {
    //                nNum++;
    //            }
    //        }

    //        if (examList.size() > 0)
    //        {
    //            currentExamID = 0;
    //        }
}

void mvImageAcquisitWidget::setCurrentPage(int index)
{
    mIsExposeOrPreviewFlag = index;
}

void mvImageAcquisitWidget::getImagePath(QVector<exam_type*>& examList, QString studyUID)
{
    QStringList sopData = DicomDatabase::instance()->getSOPUIDAndSeriesUIDAndViewNameForStudy(
        studyUID);
    for (int i = 0; i < sopData.size(); i++) {
        exam_type* _exam = new exam_type();
        QString temp = sopData.at(i);
        QStringList mTemp = temp.split("|");
        QString SOPUID = mTemp.at(0);
        QString seriesUID = mTemp.at(1);
        QString ViewName = mTemp.at(2);
        QString InstanceNumber = mTemp.at(3);
        QStringList list = DicomDatabase::instance()->viewDescriptionAndFileForInstance(SOPUID);
        if (!list.isEmpty()) {
            _exam->id = InstanceNumber.toInt();
            _exam->SeriesInstanceUID = seriesUID;
            _exam->SeriesID = ViewName;
            _exam->SOPUID = SOPUID;
            _exam->ImageFileFullName = list.at(0);
            _exam->ImageFileFullName = _exam->ImageFileFullName.replace(".dcm", ".png");
            _exam->isRemote = list.at(1) == "Remote" ? true : false;
            QStringList exam = DicomDatabase::instance()->getImageStatusBySOPUID(SOPUID);
            if (exam.size() > 0) {
                if (exam.at(0) == "Completed" || exam.at(0) == "Refused") {
                    _exam->isExam = true;
                } else {
                    _exam->isExam = false;
                }
            }
            examList.push_back(_exam);
        }
    }
}

void mvImageAcquisitWidget::StartExposure()
{
    mIsExposing = true;

    ui->pushButtonFinished->setEnabled(true);
    ui->pushButtonStart->setEnabled(false);
    ui->mStateShowStackWidget->setCurrentIndex(0);

    updateInfoPanel(tr("Setting up work mode..."), Normal);
    updateDeviceState(ExposureState::SettingUp);

    qApp->processEvents();

    if (!canStartExposure()) {
        resetUI();
        updateInfoPanel(tr("Lack of exposure conditions"), Error);
        updateDeviceState(ExposureState::Fault);
        return;
    }

    bool success = mManager->setupWorkMode(true, 0);

    if (!success) {
        resetUI();
        updateInfoPanel(tr("Failed to set work mode, please try again"), Error);
        updateDeviceState(ExposureState::Fault);
        return;
    }
}

void mvImageAcquisitWidget::StopExposure()
{
    if (mManager) {
        mManager->stopExposure();
    }

    resetUI();
}

bool mvImageAcquisitWidget::canStartExposure() const
{
    if (!mManager) {
        return false;
    }

    if (!mManager->isHardwareReady()) {
        return false;
    }

    return true;
}

void mvImageAcquisitWidget::AlgSetCorrectionFile(int mOralMajor, QString mOralAddr)
{
    qDebug() << (QString("InitDetectorConnection:%1, %2")
                     .arg(mOralMajor)
                     .arg(mOralAddr)
                     .toStdString());
    auto algorithmic = XPectAlgorithmic::Instance();

    QString filename = DataLocations::getRootConfigPath() + "/Cal/" + QString::number(mOralMajor)
                       + ".0/" + mOralAddr + "/Calibration.dat";
    ;

    qDebug() << (QString("CorrectionFile:%1").arg(filename).toStdString());

    if (QFile::exists(filename)) {
        algorithmic->SetCorrectionFilePath(filename);
    } else {
        qDebug() << ("Correction file does not exist ");
        mcalibrationFileExists = false;
    }
}

void mvImageAcquisitWidget::resetUI()
{
    ui->pushButtonFinished->setEnabled(false);
    ui->pushButtonStart->setEnabled(true);

    mIsExposing = false;

    updateInfoPanel("", Normal);
    updateDeviceState(ExposureState::Idle);
    ui->mStateShowStackWidget->setCurrentIndex(0);
}

void mvImageAcquisitWidget::updateInfoPanel(const QString& message, int type)
{
    if (message.isEmpty()) {
        ui->lbError->setText("");
        return;
    }

    QString fullMessage = message;

    // 限制消息长度，避免显示过长
    if (fullMessage.length() > 100) {
        fullMessage = fullMessage.left(97) + "...";
    }

    QString color;
    QFont font = ui->lbExposeText->font();

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

    ui->lbError->setStyleSheet(style);
    ui->lbError->setFont(font);
    ui->lbError->setText(fullMessage);

    qDebug() << fullMessage;
}

void mvImageAcquisitWidget::updateDeviceState(ExposureState state)
{
    QString text;
    QString style;

    switch (state) {
    case ExposureState::Idle:
        text = tr("Idle");
        style = "color: gray; font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::SettingUp:
        text = tr("Setting Up");
        style = "color: orange;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Exposing:
        text = tr("Exposing");
        style = "color: yellow;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Acquiring:
        text = tr("Acquiring");
        style = "color: blue;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Processing:
        text = tr("Processing");
        style = "color: purple;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Completed:
        text = tr("Completed");
        style = "color: green;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Fault:
        text = tr("Fault");
        style = "color: red;font-weight: bold; font-size: 25px;";
        break;
    case ExposureState::Timeout:
        text = tr("Timeout");
        style = "color: red;font-weight: bold; font-size: 25px;";
        break;
    }

    ui->lbExposeText->setStyleSheet(style);
    ui->lbExposeText->setText(text);
}

void mvImageAcquisitWidget::onInfoReceived(const QString& message)
{
    if (!this->isVisible()) {
        return;
    }

    updateInfoPanel(message, Normal);
}
void mvImageAcquisitWidget::onWarningReceived(const QString& message)
{
    if (!this->isVisible()) {
        return;
    }

    updateInfoPanel(tr("Warning: ") + message, Warning);
}
void mvImageAcquisitWidget::onErrorReceived(const QString& message)
{
    if (!this->isVisible()) {
        return;
    }

    updateInfoPanel(tr("Error: ") + message, Error);
    updateDeviceState(ExposureState::Fault);
}
void mvImageAcquisitWidget::onNotificationReceived(const QString& message)
{
    if (!this->isVisible()) {
        return;
    }

    updateInfoPanel(tr("Notification: ") + message, Normal);
}

void mvImageAcquisitWidget::onExposureError(const QString& error)
{
    if (!this->isVisible()) {
        return;
    }

    updateInfoPanel(tr("Exposure error: ") + error, Error);
    updateDeviceState(ExposureState::Fault);
}

void mvImageAcquisitWidget::onImagesReady(const QVector<HWImageData>& images)
{
    if (!this->isVisible()) {
        return;
    }

    updateInfoPanel(QString(tr("Received %1 images")).arg(images.size()), Normal);
    updateDeviceState(ExposureState::Processing);

    bool zoomTwice = false;
    QString fileName;
    QString fileName1;

    auto algorithmic = XPectAlgorithmic::Instance();

    QString timestamp = QDateTime::currentDateTime().toString("hhmmsszzz");

    imagePathX = "";
    imagePathY = "";

    for (const HWImageData& image : images) {
        QString directory = DataLocations::getDicomPath();
        QString dateStr = QDateTime::currentDateTime().toString("yyyyMMdd");

        if (image.orientation % 2 == 0) {
            fileName = QDir::toNativeSeparators(directory + "/" + dateStr + "/" + timestamp + "/X/"
                                                + QString("image_%1_init.raw").arg(timestamp));
            fileName1 = QDir::toNativeSeparators(directory + "/" + dateStr + "/" + timestamp + "/X/"
                                                 + QString("image_%1_cal.raw").arg(timestamp));

        } else {
            fileName = QDir::toNativeSeparators(directory + "/" + dateStr + "/" + timestamp + "/Y/"
                                                + QString("image_%1_init.raw").arg(timestamp));
            fileName1 = QDir::toNativeSeparators(directory + "/" + dateStr + "/" + timestamp + "/Y/"
                                                 + QString("image_%1_cal.raw").arg(timestamp));
        }

        QFileInfo fInfo(fileName);
        if (!QDir().mkpath(fInfo.path())) {
            qWarning() << tr("Failed to create directory:") << fInfo.path();
            return;
        }

        QFileInfo fInfo1(fileName1);
        if (!QDir().mkpath(fInfo1.path())) {
            qWarning() << tr("Failed to create directory:") << fInfo1.path();
            return;
        }

        if (!image.imageData.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(image.imageData);
                file.close();
                qDebug() << tr("Raw data saved to:") << fileName;
            } else {
                qWarning() << tr("Cannot write file:") << fileName;
            }
        }

        if (!image.imageData.isEmpty()) {
            QFile file(fileName1);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(image.imageData);
                file.close();
                qDebug() << tr("Raw data saved to:") << fileName1;
            } else {
                qWarning() << tr("Cannot write file:") << fileName1;
            }
        }

        algorithmic->CalibrateArquireImage(image.width,
                                           image.height,
                                           image.bitDepth,
                                           fileName1,
                                           zoomTwice);

        int widthnew = zoomTwice ? image.width * 2 : image.width;
        int heightnew = zoomTwice ? image.height * 2 : image.height;

        qint64 filesize = 0;
        std::unique_ptr<unsigned short[]> raw = algorithmic->ReadRawFileTo16(fileName1, &filesize);
        std::unique_ptr<unsigned short[]> raw2image = std::make_unique<unsigned short[]>(filesize
                                                                                         * 2);
        // pro image data

        std::copy(raw.get(), raw.get() + filesize, raw2image.get() + filesize);
        qInfo() << (QString("width:%1,height:%1, filesize:%3")
                        .arg(widthnew)
                        .arg(heightnew)
                        .arg(filesize)
                        .toStdString());
        double windowCenterActual = 32768, windowWidthActual = 65536;
        // 这里要么计算 windowCenter windowWidth 要么吧数据映射到 0-65535
        algorithmic->CalculateWindowParams(raw.get(),
                                           widthnew,
                                           heightnew,
                                           windowCenterActual,
                                           windowWidthActual);
        qInfo() << (QString("windowCenterActual:%1, windowWidthActual:%2")
                        .arg(windowCenterActual)
                        .arg(windowWidthActual)
                        .toStdString());

        bool resprocess = algorithmic->ProcessImageData(widthnew, heightnew, image.bitDepth, raw);
        qInfo() << (QString("acquisition ProcessImageData:%1 ").arg(resprocess).toStdString());
        std::copy(raw.get(), raw.get() + filesize, raw2image.get());
        QString imagePath;
        if (image.orientation % 2 == 0) {
            imagePath = QDir::toNativeSeparators(directory + "/" + dateStr + "/" + timestamp + "/X/"
                                                 + QString("image_%1.raw").arg(timestamp));
            imagePathX = imagePath;
        } else {
            imagePath = QDir::toNativeSeparators(directory + "/" + dateStr + "/" + timestamp + "/Y/"
                                                 + QString("image_%1.raw").arg(timestamp));
            imagePathY = imagePath;
        }

        if (raw && filesize > 0) {
            QFile file(imagePath);
            if (file.open(QIODevice::WriteOnly)) {
                // 直接写入 raw 数据
                qint64 bytesWritten = file.write(reinterpret_cast<const char*>(raw.get()), filesize);
                file.close();

                if (bytesWritten == filesize) {
                    qDebug() << tr("Raw pro data saved to:") << imagePath;

                    // 更新 image 对象的文件路径
                    HWImageData& mutableData = const_cast<HWImageData&>(image);
                    mutableData.filePath = imagePath;

                    // 如果需要，也可以将数据保存到 imageData
                    // image.imageData = QByteArray(reinterpret_cast<const char*>(raw.get()), filesize);
                } else {
                    qWarning() << tr("Incomplete write:") << bytesWritten << tr("of") << filesize;
                }
            } else {
                qWarning() << tr("Cannot write file:") << imagePath;
            }
        }
    }

    StatusCode status;
    cv::Mat lr_profile, ud_profile, lr_img, ud_img;
    MeasurementData measure_data;

    cv::Size raw_size(344, 417);
    cv::Size crop_size(304, 137);

    std::tie(status, lr_profile, ud_profile, lr_img, ud_img, measure_data)
        = detector->measure(imagePathX.toStdString(),
                            imagePathY.toStdString(),
                            raw_size,
                            "uint16",
                            crop_size,
                            true,
                            ui->labelInnerDiameter->text().toFloat());
    int zoom = 1;
    ui->mPreviewFrame->setAxisRange(measure_data.outer_ellipse.x_diameter
                                            > measure_data.inner_ellipse.y_diameter
                                        ? measure_data.outer_ellipse.x_diameter * zoom
                                        : measure_data.inner_ellipse.y_diameter * zoom);
    ui->mPreviewFrame->setInnerEllipseCenter(QPointF(measure_data.inner_ellipse.x_diameter * zoom,
                                                     measure_data.inner_ellipse.y_diameter * zoom));
    ui->mPreviewFrame->setInnerEllipseMinorRadius(measure_data.inner_ellipse.y_diameter / 2 * zoom);
    ui->mPreviewFrame->setInnerEllipseMajorRadius(measure_data.inner_ellipse.x_diameter / 2 * zoom);
    ui->mPreviewFrame->setOuterEllipseMinorRadius(measure_data.outer_ellipse.x_diameter / 2 * zoom);
    ui->mPreviewFrame->setOuterEllipseMajorRadius(measure_data.outer_ellipse.y_diameter / 2 * zoom);

    ui->mPreviewFrame->setAutoMeasurementAngles(measure_data.wall_thickness.min_angle);

    if (measure_data.wall_thickness.spec_thickness.size() >= 8) {
        ui->mPreviewFrame->setMeasurementDisplayText(0,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[0])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(45,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[1])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(90,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[2])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(135,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[3])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(180,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[4])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(225,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[5])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(270,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[6])
                                                         + "mm");
        ui->mPreviewFrame->setMeasurementDisplayText(315,
                                                     QString::number(measure_data.wall_thickness
                                                                         .spec_thickness[7])
                                                         + "mm");

        ui->mCurveFrame->addData(measure_data.inner_ellipse.diameter,
                                 measure_data.outer_ellipse.diameter);
    }

    resetUI();
}

void mvImageAcquisitWidget::onExposureProcess(ExposureState state)
{
    if (!this->isVisible()) {
        return;
    }
    updateInfoPanel("", Normal);
    updateDeviceState(state);
}
} // namespace mv
