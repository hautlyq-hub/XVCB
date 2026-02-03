#ifndef CIMAGEPROCESS_H
#define CIMAGEPROCESS_H

#include <string>

#include <QString>

#include "ImageProConfig.h"
#include "PostProcPara.h"

#define __CHIP_FUNC__
#define __AI_MODULE__

class CImageProcess
{
public:
    explicit CImageProcess(std::string sConfigPath);

    ~CImageProcess();

    //生成校正文件
    //参数1：源文件路径（校正时拍摄的RAW图存放路径）
    //参数2：目标文件全路径（.dat文件存放的路径，包括文件名和后缀）
    //参数3：图像宽
    //参数4：图像高
    //参数5：图像位深
    // int GenerateCorrectionFile(std::string sSrcFileFolder, std::string sDstFileFullPath, int nChipWidth, int nChipHeight, int nbits);
    int GenerateCorrectionFile(QString sSrcFileFolder,
                               QString sDstFileFullPath,
                               int nChipWidth,
                               int nChipHeight,
                               int nbits);

    //设置矫正文件.dat路径
    void SetCorrectionFilePath(std::string Path);

    //加载校正文件(将校正文件以二进制的方式读到内存中)
    int ReLoadCorrectionFilePath(std::string Path);

    //对图像数据进行校正处理
    //参数1：源图数据存放地址
    //参数2：源图宽
    //参数3：源图 高
    //参数4：图像位数
    //参数5：校正后的图像存放地址（由调用者申请和释放）
    //参数6：校正后的图像宽
    //参数7：校正后的图像高
    bool CorrectImageData(unsigned char* pusSrc, int iSrcWidth, int iSrcHeight, int nbits,
                          unsigned char* pusDst, int iDstWidth, int iDstHeight);

    //对图像数据进行后处理
    //参数1：源图数据存放地址
    //参数2：源图宽
    //参数3：源图 高
    //参数4：图像位数
    // bool ProcessImageData(unsigned char* pusSrc, int iSrcWidth, int iSrcHeight, int nbits,
    //                       unsigned int* pusDst, int iDstWidth, int iDstHeight, const PostProcPara* para);
    bool ProcessImageData(unsigned int* pusSrc,
                          int iSrcWidth,
                          int iSrcHeight,
                          int nbits,
                          unsigned int* pusDst,
                          const PostProcPara* para);

    // //图像裁减,上下左右裁减同样的像素数
    // bool Crop(unsigned char* pusSrc, int iSrcWidth, int iSrcHeight, int nPixelNum,
    //           unsigned char* pusDst, int iDstWidth, int iDstHeight,int iElementSize);

    // //图像裁减，上下左右分别裁减不同的像素数
    // bool Crop(unsigned char* pusSrc, int iSrcWidth, int iSrcHeight, int iElementSize,
    //           int top, int bottom, int left, int right, unsigned char* pusDst);

    //插值放大
    bool ReSizeImage(char* pnDataIn, int nDataWidth, int nDataHeight, int nDataBytes, int nDataBits, int nDstWidth,
                     int nDstHeight, int nResizeType, char* pnDataOut);

    //后处理
    // bool PostProcAlgorithm(unsigned char* pusSrc, int iSrcWidth, int iSrcHeight, int nbits, POSTPROCESSTYPE ProcType);

    //计算图像的平均灰度
    double CalcAvgGray(unsigned short* pusSrc, int iSrcWidth, int iSrcHeight);
    double CalcAvgGray(unsigned int* pusSrc, int iSrcWidth, int iSrcHeight);

    unsigned char* ReadRawFile(const std::string & filePath, uint64_t* fileSize);
    //配置文件参数
    ImageProConfig* m_pImageProConfig;
private:

    //校正文件.dat路径
    std::string m_sCorrectionFilePath;

    //存储校正文件数据的地址
    unsigned char* m_pCorrectTemplateData;

    //校正文件数据长度
    unsigned long m_iCorrectTemplateDataLen;


    //表示正负向，一般直接传1即可
    int m_iPixelIntensityRelationshipSign;

    //体位信息，牙科传一个全为0的数组即可，大小定20即可
    float* m_pfaExpPara;

    //是否要进行背景分割，SDK不需要进行分割，直接传false
    bool m_bBkgSeg;

    //后处理的方法号，牙科使用方法7
    //int m_iMethod;

    //背景分割标记，传一个全为0的数组即可，用不到
    unsigned int* m_piMaskOut;

    //原始图像数据直方图,大小为1<<nbit,nbit为图像位数
    int* m_piaOrgHist;

    //处理后的图像数据直方图,大小为1<<nbit,nbit为图像位数
    int* m_piaHistDataOut;

    //大小定10个，目前只用了三个
    float* m_pfaCurvePara;
};

#endif // CIMAGEPROCESS_H
