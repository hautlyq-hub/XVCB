#include "XPectAlgorithmic.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "CImageProcess.h"

XPectAlgorithmic *XPectAlgorithmic::Instance()
{
    static XPectAlgorithmic instance;
    return &instance;
}

XPectAlgorithmic::XPectAlgorithmic()
{
    // todo check algorithmic file exists
}

XPectAlgorithmic::~XPectAlgorithmic()
{
    if (mImageProcess) {
        delete mImageProcess;
    }
}

bool XPectAlgorithmic::LoadConfig(QString fileName)
{
    // mImageProcess = std::make_shared<CImageProcess>(fileName.toStdString());
    mImageProcess = new CImageProcess(fileName.toStdString());
    return mImageProcess != nullptr;
}

bool XPectAlgorithmic::SetCorrectionFilePath(QString path)
{
    if (mImageProcess) {
        int res = mImageProcess->ReLoadCorrectionFilePath(path.toStdString());
        return res == 0;
    }
    return false;
}

// bool XPectAlgorithmic::ProcessArquireImage(
//     int width, int height, int bit, QString filename, int top, int right, int bottom, int left)
// {
//     qint64 rawSize = 0;
//     std::unique_ptr<unsigned char[]> raw = ReadRawFile(filename, &rawSize);
//     int widthnew = width - right - left;
//     int heightnew = height - top - bottom;
//     bool res = true;
//     if (!(top == 0 && right == 0 && bottom == 0 && left == 0)) {
//         uint rawSizeNew = widthnew * heightnew * bit / 8;
//         std::unique_ptr<unsigned char[]> buffer = std::make_unique<unsigned char[]>(rawSizeNew);
//         res = mImageProcess
//                   ->Crop(raw.get(), width, height, bit / 8, top, bottom, left, right, buffer.get());
//         if (!res) {
//             return res;
//         }
//         raw = std::move(buffer);
//     }
//     res = CalibrateArquireImage(widthnew, heightnew, bit, raw);
//     return res;
// }

std::unique_ptr<unsigned short[]> XPectAlgorithmic::CalibrateArquireImage(
    int width, int height, int bit, const unsigned char *imageData, qint64 dataSize, bool &zoomTwice)
{
    if (!imageData || dataSize <= 0) {
        qWarning() << "CalibrateArquireImage: invalid image data";
        return nullptr;
    }

    // 检查数据大小是否匹配
    qint64 expectedSize = static_cast<qint64>(width) * height * bit / 8;
    if (dataSize < expectedSize) {
        qWarning() << "CalibrateArquireImage: data size mismatch. Expected:" << expectedSize
                   << "Got:" << dataSize;
        return nullptr;
    }

    // 计算放大后的数据大小（字节数）
    uint rawSize2, width_new, height_new;
    if (zoomTwice) {
        width_new = width * 2;
        height_new = height * 2;
    } else {
        width_new = width;
        height_new = height;
    }
    rawSize2 = width_new * height_new * bit / 8;

    // 分配缓冲区（unsigned char 类型用于 CorrectImageData）
    std::unique_ptr<unsigned char[]> buffer = std::make_unique<unsigned char[]>(rawSize2);

    // 调用校正函数
    bool res = mImageProcess->CorrectImageData(const_cast<unsigned char *>(imageData),
                                               width,
                                               height,
                                               bit,
                                               buffer.get(),
                                               width_new,
                                               height_new);

    if (res) {
        qDebug() << "DataCorrect Successfully generated.";

        // 将 unsigned char 数据转换为 unsigned short
        // 注意：rawSize2 是字节数，需要除以2得到unsigned short的元素个数
        size_t elementCount = rawSize2 / sizeof(unsigned short);
        auto result = std::make_unique<unsigned short[]>(elementCount);

        // 直接内存拷贝
        memcpy(result.get(), buffer.get(), rawSize2);

        return result;
    } else {
        qWarning() << "DataCorrect failed." << res;
        zoomTwice = false;
        return nullptr;
    }
}

// bool XPectAlgorithmic::CalibrateArquireImage(
//     int width, int height, int bit, QString filename, std::unique_ptr<unsigned char[]> &raw)
// {
//     return false;
// }

std::unique_ptr<unsigned char[]> XPectAlgorithmic::ReadRawFile(const QString &filePath,
                                                               qint64 *fileSize)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file:" << file.errorString();
        return nullptr;
    }
    *fileSize = file.size();
    if (*fileSize <= 0) {
        return nullptr;
    }
    // 避免初始化（需手动覆盖，否则内容未定义）
    // std::unique_ptr<unsigned char[]> buffer(new unsigned char[fileSize]);
    // 分配并管理内存
    auto buffer = std::make_unique<unsigned char[]>(static_cast<size_t>(*fileSize));
    // 读取数据
    if (file.read(reinterpret_cast<char *>(buffer.get()), *fileSize) != *fileSize) {
        return nullptr;
    }
    return buffer;
}

std::unique_ptr<unsigned short[]> XPectAlgorithmic::ReadRawFileTo16(const QString &filePath,
                                                                    qint64 *fileSize)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file:" << file.errorString();
        return nullptr;
    }
    *fileSize = file.size();
    if (*fileSize <= 0) {
        return nullptr;
    }

    // 验证文件大小是偶数字节（每个 unsigned short = 2 字节）
    if (*fileSize % 2 != 0) {
        qDebug() << "File size is not even, cannot read as 16-bit values";
        return nullptr;
    }
    // 分配元素数量 = 字节数 / 2
    const size_t elementCount = static_cast<size_t>(*fileSize / 2);
    auto buffer = std::make_unique<unsigned short[]>(elementCount);

    // 读取时仍需使用 char* 接口（底层字节操作）
    if (file.read(reinterpret_cast<char *>(buffer.get()), *fileSize) != *fileSize) {
        return nullptr;
    }
    *fileSize /= 2;
    return buffer;
}

bool XPectAlgorithmic::SaveToFile(const std::string &filename,
                                  const std::unique_ptr<unsigned char[]> &raw,
                                  size_t rawSize)
{ // 检查参数有效性
    if (rawSize == 0 || !raw) {
        return false; // 或抛出 std::invalid_argument
    }
    // 以二进制模式打开文件（覆盖写入）
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open()) {
        return false; // 或抛出 std::runtime_error("无法打开文件")
    }
    // 写入数据（转换指针类型）
    ofs.write(reinterpret_cast<const char *>(raw.get()), rawSize);
    // 检查写入是否成功（流状态）
    return ofs.good();
}

void XPectAlgorithmic::CalculateWindowParams(const unsigned short *image,
                                             int width,
                                             int height,
                                             double &windowCenter,
                                             double &windowWidth,
                                             float lowerPercentile,
                                             float upperPercentile)
{
    const int totalPixels = width * height;
    // 1. 拷贝并排序像素值
    std::vector<unsigned short> sortedPixels(totalPixels);
    std::copy(image, image + totalPixels, sortedPixels.begin());
    std::sort(sortedPixels.begin(), sortedPixels.end());
    // 2. 计算百分位位置
    int lowIdx = static_cast<int>(std::round((lowerPercentile / 100.0f) * (totalPixels - 1)));
    int highIdx = static_cast<int>(std::round((upperPercentile / 100.0f) * (totalPixels - 1)));
    // 3. 获取百分位对应的像素值
    unsigned short lowVal = sortedPixels[lowIdx];
    unsigned short highVal = sortedPixels[highIdx];
    // 4. 计算窗口参数
    windowCenter = (static_cast<double>(lowVal) + highVal) / 2.0;
    windowWidth = static_cast<double>(highVal - lowVal);
    // 边界情况处理
    if (windowWidth < 1) {
        windowWidth = 1; // 防止窗宽为零
        windowCenter = (static_cast<double>(sortedPixels.front()) + sortedPixels.back()) / 2.0;
    }
}

void XPectAlgorithmic::ApplyWindowing(
    unsigned short *imageData, int width, int height, double windowCenter, double windowWidth)
{
    double lower = windowCenter - windowWidth / 2.0;
    double upper = windowCenter + windowWidth / 2.0;

    int totalPixels = width * height;

    for (int i = 0; i < totalPixels; ++i) {
        if (imageData[i] < static_cast<unsigned short>(lower)) {
            imageData[i] = 0;
        } else if (imageData[i] > static_cast<unsigned short>(upper)) {
            imageData[i] = 65535;
        } else {
            // 线性映射到 0-65535
            imageData[i] = static_cast<unsigned short>(
                ((static_cast<double>(imageData[i]) - lower) / (upper - lower)) * 65535.0
                + 0.5 // 四舍五入
            );
        }
    }
}

bool XPectAlgorithmic::ProcessImageData(int width,
                                        int height,
                                        int bit,
                                        std::unique_ptr<unsigned short[]> &raw)
{
    if (mImageProcess && mImageProcess->m_pImageProConfig) {
        qDebug() << "ProcessImageData";
        size_t elementCount = static_cast<size_t>(width * height);
        auto rawptr = std::make_unique<unsigned int[]>(elementCount);
        for (size_t i = 0; i < elementCount; ++i) {
            rawptr[i] = static_cast<unsigned int>(raw[i]);
        }

        std::unique_ptr<unsigned int[]> rawout = std::make_unique<unsigned int[]>(elementCount);
        bool ret = mImageProcess->ProcessImageData(rawptr.get(),
                                                   width,
                                                   height,
                                                   bit,
                                                   rawout.get(),
                                                   mImageProcess->m_pImageProConfig->m_pfPostTooth);

        qDebug() << "ProcessImageData: " << ret;
        if (ret) {
            for (size_t i = 0; i < elementCount; ++i) {
                raw[i] = static_cast<unsigned short>(rawout[i]);
            }
        }
        // Invert pixel
        for (int i = 0; i < width * height; i++) {
            raw[i] = 65535 - raw[i];
        }
        return ret;
    } else {
        qDebug() << "ProcessImageData: unexecuted ; ";
    }
    return false;
}

int XPectAlgorithmic::GenerateCorrectionFile(
    QString rawDir, QString targetFile, int width, int height, int bit)
{
    int res = mImageProcess->GenerateCorrectionFile(rawDir, targetFile, width, height, bit);
    qInfo() << "GenerateCorrectionFile result:" << res;
    return res;
}
