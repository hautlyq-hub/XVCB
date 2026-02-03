#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QDialog>
#include <atomic>

#include "XV.Communication/XRayWrokerQF.h"
#include "XV.Communication/XVDetectorWorker.h"

#include "algorithms/XPectAlgorithmic.h"

namespace Ui {
class CalibrationDialog;
}

class CalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationDialog(QWidget *parent = nullptr);
    ~CalibrationDialog();

signals:
    void signalSetupCalibration();
    void signalSetupWork();
    void signalSetExposureKv(uint val);
    void signalSetExposureTimeMs(int index);
    void signalSetExposureMa(uint val);
    void signalSetXRay(int kvval, int msindex, int maval);
    void singleGenCalibrationFile();

private slots:
    void SetupWork();
    void ImageDataReceived(int width, int height, int bit, QString filename);
    void DataReceivedError(QString err);
    void slotDataReceivedFirmwareID(int oralMajor, int oralMinor, int oralPatch, QString addr);
    void slotStatus(uint status);
    void GenCalibrationFile();
    void CalibrationSetCompleted(bool ok);
    void XRayResultReceived(QString cmd, int result, QString error);

private:
    void InitXRayConnection();

    void InitDetectorConnection();
    void ShowImage();
    void ShowImage(int width, int height, int bit, QString filename);

    QList<qreal> GetExposureTimeParam()
    {
        static QList<qreal> list = {0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09, 0.10,
                                    0.11, 0.12, 0.13, 0.14, 0.15, 0.16, 0.17, 0.18, 0.19, 0.20,
                                    0.25, 0.30, 0.35, 0.40, 0.45, 0.50, 0.55, 0.60, 0.65, 0.70,
                                    0.75, 0.80, 0.85, 0.90, 0.95, 1.00, 1.05, 1.10, 1.15, 1.20,
                                    1.25, 1.30, 1.35, 1.40, 1.45, 1.50, 1.55, 1.60, 1.65, 1.70,
                                    1.75, 1.80, 1.85, 1.90, 1.95, 2.00};
        return list;
    }
    Ui::CalibrationDialog *ui;

    // 口内的
    int mOralMajor = 0, mOralMinor = 0, mOralPatch = 0;
    QString mOralAddr = "";
    // 1,2,3,4
    int mCurrentIndex = 1;
    QMap<int, QString> mCalibrationRawFiles;
    QString mPreCalFolder;

    std::atomic<bool> mEnableExposure = false;
    std::atomic<bool> mEnableContinue = false;
    int mImageWidth = 0, mImageHeight = 0, mImageBit = 16;

    // 0-20
    int mSecIndex = 35;
    // 60-70
    int mKVValue = 65;
    int mUAValue = 0;

protected:
    void showEvent(QShowEvent *event) override;
};

#endif // CALIBRATIONDIALOG_H
