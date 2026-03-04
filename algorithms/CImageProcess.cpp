#include "CImageProcess.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include "GlobalDef.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
// #include <new>

#include "DataProc.h"
#include "ErrorCode.h"

CImageProcess::CImageProcess(std::string sConfigPath)
{
    m_sCorrectionFilePath="";
    m_iCorrectTemplateDataLen = 0;
    m_pCorrectTemplateData = nullptr;

    //申请增强图像需要用的参数空间
    {
        //表示正负向，一般直接传1即可
        m_iPixelIntensityRelationshipSign = 1;

        //体位信息，牙科传一个全为0的数组即可，大小定20即可
        m_pfaExpPara = new float[20]();
        // memset(m_pfaExpPara, 0, sizeof(float) * 20);

        //是否要进行背景分割，SDK不需要进行分割，直接传false
        m_bBkgSeg = false;

        //背景分割标记，传一个全为0的数组即可，用不到
        m_piMaskOut = new unsigned int[688*834]();
        // memset(m_piMaskOut, 0, sizeof(unsigned int) * 688*834);

        //原始图像数据直方图,大小为1<<nbit,nbit为图像位数
        int len = 1 << 18;
        m_piaOrgHist = new int[len]();
        // memset(m_piaOrgHist, 0, sizeof(int) * len);

        //处理后的图像数据直方图,大小为1<<nbit,nbit为图像位数
        m_piaHistDataOut = new int[len]();
        // memset(m_piaHistDataOut, 0, sizeof(int) * len);

        //大小定10个，目前只用了三个
        m_pfaCurvePara = new float[10]();
        // memset(m_pfaCurvePara, 0, sizeof(float) * 10);
    }

    m_pImageProConfig = new ImageProConfig(sConfigPath);

    // 在任何已初始化QCoreApplication的位置调用：
    QString appDir = QCoreApplication::applicationDirPath();
    // 添加末尾的路径分隔符（若不存在）
    if (!appDir.endsWith(QDir::separator())) {
        appDir += QDir::separator();
    }

    QString currentDate = QDate::currentDate().toString("yyyy-MM-dd");
    QString logFileName = QString("image_process_%1.log").arg(currentDate);

    // 构建日志目录路径
    QString logDir = appDir + "log";
    // 创建日志目录（如果不存在）
    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath("."); // 创建目录
    }

    // 构建完整的日志文件路径
    QString logPath = logDir + QDir::separator() + logFileName;

    qDebug() << "DataProc log : " << logPath;

    // 初始化日志
    InitLog(logPath.toUtf8().data());

    qDebug() << "Current app directory:" << appDir;

    QByteArray appdir = appDir.toUtf8();
    char* appdir_ = appdir.data();
    int algmodule = InitAiModule(appdir_);
    qDebug() << "InitAiModule : " << algmodule;
}

CImageProcess::~CImageProcess()
{
    //
    // Need to uninit AI module in destructor
    //
    UninitAiModule();

    if (m_pCorrectTemplateData != nullptr)
    {
        delete[] m_pCorrectTemplateData;
        m_pCorrectTemplateData = nullptr;
    }

    if (m_pImageProConfig != nullptr)
    {
        delete m_pImageProConfig;
        m_pImageProConfig = nullptr;
    }

    if (m_pfaExpPara!=nullptr)
    {
        delete[] m_pfaExpPara;
        m_pfaExpPara = nullptr;
    }

    if (m_piMaskOut!=nullptr)
    {
        delete[] m_piMaskOut;
        m_piMaskOut = nullptr;
    }

    if (m_piaOrgHist!=nullptr)
    {
        delete[] m_piaOrgHist;
        m_piaOrgHist = nullptr;
    }

    if (m_piaHistDataOut!=nullptr)
    {
        delete[] m_piaHistDataOut;
        m_piaHistDataOut = nullptr;
    }

    if (m_pfaCurvePara!=nullptr)
    {
        delete[] m_pfaCurvePara;
        m_pfaCurvePara = nullptr;
    }
}

// int CImageProcess::GenerateCorrectionFile(std::string sSrcFileFolder, std::string sDstFileFullPath, int nChipWidth, int nChipHeight, int nbits)

int CImageProcess::GenerateCorrectionFile(
    QString sSrcFileFolder, QString sDstFileFullPath, int nChipWidth, int nChipHeight, int nbits)
{
    //如果目标文件所在的文件夹不存在，则创建
    // QDir().mkpath(QString::fromStdString(sDstFileFullPath));
    QFileInfo fileinfo(sDstFileFullPath);
    QDir().mkpath(fileinfo.path());

    qDebug() << "sSrcFileFolder = " << sSrcFileFolder
             << ", sDstFileFullPath = " << sDstFileFullPath;

    std::wstring rawDirWstr = sSrcFileFolder.toStdWString();
    std::wstring targetPathWstr = sDstFileFullPath.toStdWString();
    wchar_t* szSrcFileFolder = rawDirWstr.data();
    wchar_t* szCorrectionFileName = targetPathWstr.data();
    int iRet = -999;
    if ((m_pImageProConfig && m_pImageProConfig->m_pfGenCorrectFilePara)) {
        iRet = GenCorrectionFile(szSrcFileFolder,
                                 szCorrectionFileName,
                                 nChipWidth,
                                 nChipHeight,
                                 nbits,
                                 m_pImageProConfig->m_pfGenCorrectFilePara,
                                 1);
    } else {
        qCritical() << "ImageProConfig:" << (m_pImageProConfig ? "1" : "0")
                    << ",GenCorrectFilePara:"
                    << (m_pImageProConfig && m_pImageProConfig->m_pfGenCorrectFilePara ? "1" : "0");
    }
    if (iRet == PROC_SUCCESS)
    {
        qDebug()<<"Successfully generated correction file.";
    }
    else
    {
        qCritical()<<"Failed to generate correction file, result: "<<iRet;
    }
    return iRet;
}

void CImageProcess::SetCorrectionFilePath(std::string path)
{
    m_sCorrectionFilePath = path;
}

int CImageProcess::ReLoadCorrectionFilePath(std::string path)
{
    if (m_pCorrectTemplateData)
    {
        delete[] m_pCorrectTemplateData;
        m_pCorrectTemplateData = nullptr;

        m_iCorrectTemplateDataLen = 0;
    }

    if (QFile::exists(QString::fromStdString(path)))
    {
        m_sCorrectionFilePath = path;
        uint64_t filesize=0;
        m_pCorrectTemplateData = ReadRawFile(path,&filesize);
        m_iCorrectTemplateDataLen = filesize;

        if (m_pCorrectTemplateData|| filesize==0)
        {
            qDebug()<<"Successfully loaded correction file: "<<path;

            return XVD_API_SUCCESS;
        }
        else
        {
            qCritical()<<"Failed to load correction file, path: "<< path;
            return XVD_API_ERROR_FAILED_TO_LOAD_CALIBRATION_FILE;
        }
    }

    qCritical()<<"Correction file does not exist, path: "<< path;

    return XVD_API_ERROR_CALIBRATION_FILE_NOT_FOUND;
}

// 返回 unique_ptr 包装的 unsigned char 数组
unsigned char* CImageProcess::ReadRawFile(const std::string & filePath, uint64_t* fileSize) {
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file:" << file.errorString();
        return nullptr;
    }
    *fileSize = static_cast<uint64_t>(file.size());
    if (*fileSize <= 0) {
        return nullptr;
    }
    // // 避免初始化（需手动覆盖，否则内容未定义）
    // // std::unique_ptr<unsigned char[]> buffer(new unsigned char[fileSize]);
    // // 分配并管理内存
    // auto buffer = std::make_unique<unsigned char[]>(static_cast<size_t>(*fileSize));
    // // 读取数据
    // if (file.read(reinterpret_cast<char*>(buffer.get()), *fileSize) != *fileSize) {
    //     return nullptr;
    // }
    // 动态分配内存
    unsigned char* buffer = new (std::nothrow) unsigned char[static_cast<size_t>(*fileSize)];
    if (!buffer) {
        qDebug() << "Memory allocation failed.";
        return nullptr;
    }
    // 读取文件内容
    if (file.read(reinterpret_cast<char*>(buffer), *fileSize) != *fileSize) {
        delete[] buffer;
        return nullptr;
    }
    return buffer;
}

bool CImageProcess::CorrectImageData(unsigned char* pusSrc, int iSrcWidth, int iSrcHeight, int nbits, unsigned char* pusDst,
                                     int iDstWidth, int iDstHeight)
{
    if (m_pCorrectTemplateData == nullptr)
    {
        qCritical()<<("Correction template is not loaded.");
        return false;
    }

    if (m_pImageProConfig->m_pfDoCorrectPara == nullptr)
    {
        qCritical()<<("Correction parameter is not loaded.");
        return false;
    }

    bool bOutputTwice = iDstWidth / iSrcWidth == 2 && iDstHeight / iSrcHeight == 2 ? true : false;

    // std::ofstream file("/home/pi/pusSrc.raw", std::ios::binary | std::ios::out);
    // if (file.is_open()) {
    //     file.write(reinterpret_cast<const char*>(pusSrc), iSrcWidth * iSrcHeight * 2);
    //     file.close();
    // }
    int iRet = DataCorrect(pusSrc,
                           iSrcWidth,
                           iSrcHeight,
                           nbits,
                           2,
                           0,
                           m_pCorrectTemplateData,
                           m_iCorrectTemplateDataLen,
                           m_pImageProConfig->m_pfDoCorrectPara,
                           pusDst,
                           bOutputTwice);
    // std::ofstream file1("/home/pi/pusDst.raw", std::ios::binary | std::ios::out);
    // if (file1.is_open()) {
    //     file1.write(reinterpret_cast<const char*>(pusDst), iSrcWidth * iSrcHeight * 4 * 2);
    //     file1.close();
    // }
    if (PROC_SUCCESS == iRet)
    {
        qDebug()<<"Successfully corrected image.";
        return true;
    }
    else
    {
        qCritical()<<("Failed to correct image, iRet: ")<<iRet;
        return false;
    }
}

bool CImageProcess::ProcessImageData(unsigned int* pusSrc,
                                     int iSrcWidth,
                                     int iSrcHeight,
                                     int nbits,
                                     unsigned int* pusDst,
                                     const PostProcPara* para)

{
    //背景分割标记，传一个全为0的数组即可，用不到
    unsigned int* maskout = new unsigned int[(size_t) iSrcHeight * iSrcWidth]();
    //进行图像增强处理
    int iRet = Alg_ImagePreProcess(pusSrc,
                                   iSrcWidth,
                                   iSrcHeight,
                                   0,
                                   nbits,
                                   m_iPixelIntensityRelationshipSign,
                                   m_pfaExpPara,
                                   m_bBkgSeg,
                                   para->m_iMethod,
                                   para->m_pfProcPara,
                                   pusDst,
                                   maskout,
                                   m_piaOrgHist,
                                   m_piaHistDataOut,
                                   m_pfaCurvePara);

    if (maskout != nullptr) {
        delete[] maskout;
        maskout = nullptr;
    }
    qDebug()<<"Alg_ImagePreProcess process result: "<<iRet <<", method: "<< para->m_iMethod;

    //第二个函数的参数
    if (iRet == PROC_SUCCESS)
    {
        //PostProc = "Alg=1;Limit=1.8;Size=(3,3)|Alg=2;Size=(7,7);Sigma=2;Weight=0.2"
        for (int i = 0; i < para->m_iMethodCount; i++)
        {
            int m = (int)para->m_pfPostProcPara[i][0];
            float fp = *(para->m_pfPostProcPara[i] + 1);
            iRet = Alg_ImagePostProcess(
                pusDst, iSrcWidth, iSrcHeight, 0, nbits,
                (int)para->m_pfPostProcPara[i][0],
                para->m_pfPostProcPara[i] + 1);

            qDebug()<<"Alg_ImagePostProcess process result: "<<iRet <<", nMethod: "<< m <<", pfProcPara: "<< fp;
        }
    }

    if (iRet == PROC_SUCCESS)
    {
        ///////////////////////////////////////////////////////////
        //CString finename3 = "preproc-3.raw";
        //CFile file3(finename3, CFile::modeCreate | CFile::modeWrite);
        //file3.Write(pusDst, iSrcWidth * iSrcHeight * 4);
        //file3.Close();
        ///////////////////////////////////////////////////////////

        return true;
    }
    else
    {
        return false;
    }
}

// bool CImageProcess::Crop(unsigned char* pusSrc, int iSrcWidth, int iSrcHeight, int nPixelNum,
//                          unsigned char* pusDst, int iDstWidth, int iDstHeight,int iElementSize)
// {

//     if (pusSrc == nullptr || iSrcHeight <= 0 || iSrcWidth <= 0 || nPixelNum < 0 ||
//         pusDst == nullptr || nPixelNum >= iSrcWidth || nPixelNum >= iSrcHeight)
//     {
//         qCritical()<<("Error corp image data.");
//         return false;
//     }

//     for (size_t i=0;i<iDstHeight;i++)
//     {
//         memcpy(pusDst + iDstWidth * i * iElementSize,
//                pusSrc + ((i + nPixelNum) * iSrcWidth + nPixelNum) * iElementSize,
//                (size_t)iDstWidth * iElementSize);
//     }

//     return true;
// }

// bool CImageProcess::Crop(unsigned char* pusSrc, int iSrcWidth, int iSrcHeight, int iElementSize,
//                          int top, int bottom, int left, int right, unsigned char* pusDst)
// {
//     if (pusSrc == nullptr || pusDst == nullptr)
//     {
//         qCritical()<<("Error corp image data.");
//         return false;
//     }

//     if (top < 0 || bottom<0 || (top + bottom) > iSrcHeight ||
//         left < 0 || right < 0 || (left + right > iSrcWidth))
//     {
//         qCritical()<<("Error corp image data param");
//         return false;
//     }

//     int new_heigth = iSrcHeight - (top + bottom);
//     int new_width = iSrcWidth - (left + right);

//     int src_pos = (iSrcWidth * top + left) * iElementSize;
//     int dst_pos = 0;
//     int count = new_width * iElementSize;

//     int src_offset = iSrcWidth * iElementSize;
//     int dst_offset = new_width * iElementSize;

//     for (size_t i = 0; i < new_heigth; i++)
//     {
//         memcpy(pusDst + dst_pos, pusSrc + src_pos, count);
//         src_pos += src_offset;
//         dst_pos += dst_offset;
//     }

//     return true;
// }

bool CImageProcess::ReSizeImage(char* pnDataIn, int nDataWidth, int nDataHeight, int nDataBytes, int nDataBits,
                                int nDstWidth, int nDstHeight, int nResizeType, char* pnDataOut)
{
    if (pnDataIn == nullptr || pnDataOut == nullptr)
    {
        qCritical()<<("Error parameter.");
        return false;
    }

    int iRet = Alg_ResizeMultiByte(pnDataIn, nDataWidth, nDataHeight, nDataBytes, nDataBits, nDstWidth, nDstHeight,
                                   4, pnDataOut);

    if (iRet != PROC_SUCCESS)
    {
        return false;
    }

    qDebug() << "Resize image data, result:" << iRet;
    return true;
}

// bool CImageProcess::PostProcAlgorithm(unsigned char* pusSrc, int iSrcWidth, int iSrcHeight, int nbits, POSTPROCESSTYPE ProcType)
// {
//     //准备一系列参数用于调用算法接口
//
//     //通过算法枚举查找到对应的参数
//     map<int, float*>::iterator iter;
//     iter = m_pImageProConfig->m_PostProcParaMap.find((int)ProcType);
//     if (iter == m_pImageProConfig->m_PostProcParaMap.end())
//     {
//         // 没有对应算法的参数，算法枚举
//         qCritical()<<("No parameter for algorithm , procType: %d", ProcType);
//         return false;
//     }
//
//     int iRet = Alg_ImagePostProcess((unsigned int*)pusSrc, iSrcWidth, iSrcHeight, nbits, (int)ProcType, iter->second);
//     LOG_INFORMATION("Alg_ImagePostProcess result: %d, ProcType: %d", iRet, ProcType);
//
//     memcpy(pusSrc, (unsigned char*)pusSrc, (size_t)iSrcHeight * iSrcWidth * 4);
//
//     if (iRet == PROC_SUCCESS)
//     {
//         return true;
//     }
//     else
//     {
//         return false;
//     }
// }

double CImageProcess::CalcAvgGray(unsigned short* pusSrc, int iSrcWidth, int iSrcHeight)
{
    if (pusSrc == nullptr)
    {
        qCritical()<<("pusSrc == nullptr");
        return 0;
    }

    float fGraySum = 0;
    for (int i=0;i<iSrcHeight*iSrcWidth;i++)
    {
        fGraySum += pusSrc[i];
    }

    return fGraySum / (iSrcWidth * iSrcHeight);
}

double CImageProcess::CalcAvgGray(unsigned int* pusSrc, int iSrcWidth, int iSrcHeight)
{
    if (pusSrc == nullptr)
    {
        qCritical()<<("pusSrc == nullptr");
        return 0;
    }

    float fGraySum = 0;
    for (int i = 0; i < iSrcHeight * iSrcWidth; i++)
    {
        fGraySum += pusSrc[i];
    }

    return fGraySum / (iSrcWidth * iSrcHeight);

}
