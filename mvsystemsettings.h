// mvsystemsettings.h
#ifndef MVSYSTEMSETTINGS_H
#define MVSYSTEMSETTINGS_H

#include <QButtonGroup>
#include <QComboBox>
#include <QFutureWatcher>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include "XV.Communication/XProtocolManager.h"
#include "XV.Tool/XDConfigFileManager.h"
#include "XV.Tool/mvXmlOptionItem.h"
#include "XV.Tool/mvconfig.h"
#include "XV.Tool/mvdatalocations.h"

namespace Ui {
class mvsystemsettings;
}

using namespace mv;
// 前向声明
struct HWImageData;
struct CorrectRelationView;

class mvsystemsettings : public QWidget
{
    Q_OBJECT

public:
    explicit mvsystemsettings(QWidget* parent = nullptr);
    ~mvsystemsettings();

    void initData();
    void enableUIComponents(bool enabled);
    void resetUI();

private slots:

    // 校准相关槽函数 - 与UI中的控件对应
    void onBtnEnableExpClicked(bool checked);
    void onBtnGenerateClicked();
    void onRBtnXtoggled(bool checked);
    void onRBtnYtoggled(bool checked);

    void onComboBoxChanged();
    void savePortSettings();
    void refreshComPorts();
    void loadSelectedPorts();
    void selectPortInComboBox(QComboBox* comboBox, const QString& portName);
    void checkPortChanges();
    void onInfoReceived(const QString& message);
    void onWarningReceived(const QString& message);
    void onErrorReceived(const QString& message);
    void onNotificationReceived(const QString& message);
    void onExposureFinished();
    void onExposureError(const QString& error);
    void onExposureProcess(ExposureState state);
    void onImagesReady(const QVector<HWImageData>& images);
    void onReconnecttoggled();

    // 状态更新
    void updateInfoPanel(const QString& message, int type);
    void updateDeviceState(ExposureState state);

    // 图像处理
    void onImageReceived(const QVector<HWImageData>& images);

    // 计时器
    void onInfoTimerTimeout();
    void onAutoExposureTimeout();

    void AutoNextExposure();
    void AcceptImage(const QVector<HWImageData>& raws);

private:
    Ui::mvsystemsettings* ui;

    // 校准相关成员变量 - 保持原有C#代码中的命名
    bool mIsExposing = false;
    bool mRBtnXChecked = true; // 对应 mRBtnX.Checked

    // 计时器 - 保持原有命名
    QTimer* mInfoTimer;
    QTimer* mAutoExposureTimer;
    XProtocolManager* mManager;

    // 图像数据 - 保持原有命名
    HWImageData* mHWImageDataX = nullptr;
    HWImageData* mHWImageDataY = nullptr;
    QVector<CorrectRelationView*> mToothPosition; // 注意：这里保持复数形式

    // 当前目录
    QString mCurrentDir;

    QTimer* mSaveSettingsTimer;
    QTimer* mPortMonitorTimer;

    void initializeOrientationButtons();

    void Init();
    void InitLanguage();

    // 校准功能 - 使用原函数名
    void StartExposure();
    void StopExposure();
    void ShowImage(const HWImageData& imageData, int xOy);
    QImage convertRawToImage(const QByteArray& rawData);

    // 清理环境
    void CleanupEnv();

    // 文件操作
    void SaveRawImage(const HWImageData& data);
    void GenerateCalibrationFiles();

    // 工具函数
    bool canStartExposure() const;

    ExposureState m_exposureState = ExposureState::Idle;

    // 状态枚举
    enum XPInfoType { Normal = 0, Warning = 1, Error = 2 };

};

struct CorrectRelationView
{
    int XaxisOrYaxis; // 0:x, 1:y
    int ViewId;
    QString ViewIconFile;
    QWidget* ImagePanel;
    bool Expose;
    QString ImgFileName;

    CorrectRelationView()
        : XaxisOrYaxis(0)
        , ViewId(0)
        , Expose(false)
        , ImagePanel(nullptr)
    {}
};

#endif // MVSYSTEMSETTINGS_H
