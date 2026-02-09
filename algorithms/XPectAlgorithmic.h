#ifndef XPECTALGORITHMIC_H
#define XPECTALGORITHMIC_H

#include <fstream>

#include <QCoreApplication>
#include <QFile>
#include <QString>

#include "CImageProcess.h"

class XPectAlgorithmic
{
public:
    static XPectAlgorithmic* Instance();

public:
    explicit XPectAlgorithmic();
    ~XPectAlgorithmic();

    bool LoadConfig(QString fileName);

    //设置矫正文件.dat路径
    bool SetCorrectionFilePath(QString Path);

    // Crop int top, int left, int right, int bottom
    // bool ProcessArquireImage(int width, int height, int bit, QString filename, int top, int right, int bottom, int left);

    std::unique_ptr<unsigned short[]> CalibrateArquireImage(int width,
                                                            int height,
                                                            int bit,
                                                            const unsigned char* imageData,
                                                            qint64 dataSize,
                                                            bool& zoomTwice);

    bool CalibrateArquireImage(int width, int height, int bit, QString filename, bool& zoomTwice);
    // bool CalibrateArquireImage(int width, int height, int bit, QString filename, std::unique_ptr<unsigned char[]>& buffer);

    std::unique_ptr<unsigned char[]> ReadRawFile(const QString& filePath, qint64* fileSize);
    std::unique_ptr<unsigned short[]> ReadRawFileTo16(const QString& filePath, qint64* fileSize);

    // todo 是否保留中间结果; 不需要则不用存校正后的raw文件
    bool SaveToFile(const std::string& filename,
                    const std::unique_ptr<unsigned char[]>& raw,
                    size_t rawSize);

    // todo 从这里抽出单独class设计
    void GenerateDicomFile(const QString& file);
    void GenerateDicomFile2(const QString& file);

    //  Calculate windowCenter windowWidth
    void CalculateWindowParams(const unsigned short* image,  // 16位灰度图像数据指针
                               int width,                    // 图像宽度
                               int height,                   // 图像高度
                               double& windowCenter,         // 输出的窗口中心(Window Level)
                               double& windowWidth,          // 输出的窗宽(Window Width)
                               float lowerPercentile = 1.0f, // 默认使用1%的像素作为下界
                               float upperPercentile = 99.0f // 默认使用99%的像素作为上界
    );

    // 把窗宽映射到 0-65535 ;
    void ApplyWindowing(
        unsigned short* imageData, int width, int height, double windowCenter, double windowWidth);

    bool ProcessImageData(int width, int height, int bit, std::unique_ptr<unsigned short[]>& raw);

    int GenerateCorrectionFile(QString rawDir, QString targetFile, int width, int height, int bit);

private:
    // std::shared_ptr<CImageProcess> mImageProcess = nullptr;
    CImageProcess* mImageProcess = nullptr;
};

#endif // XPECTALGORITHMIC_H
