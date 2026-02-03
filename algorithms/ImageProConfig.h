#ifndef IMAGEPROCONFIG_H
#define IMAGEPROCONFIG_H

#include <string>
#include <map>
#include "PostProcPara.h"


class ImageProConfig
{
public:

    //声明构造函数，需要传入图像宽、高、位数、配置文件路径
    // DataProcFeature.xml
    ImageProConfig(std::string sConfigPath);

    ~ImageProConfig();

    // 设置校正时使用的算法
    //   0: AI
    //   1: Hybrid(Classical + AI)
    //   2: Classical
    bool SetDataCorrectAlgorithm(int nAlgorithm);

private:

    //读取并解析生成校正文件所需的参数 xml DataCorrect BPDetect
    bool GetGenCorrectFilePara();

    //读取并解析进行校正时所需的参数 xml DataCorrect BPCorrect
    bool GetDoCorrectDataPara();

    //读取并解析进行后处理所需的参数 xml ProcPara
    bool GetProcessImageDataPara(std::string NodeName, PostProcPara* pProcPara);

    //读取是否需要初始化AI
    //bool GetAiControl();

    //读取后处理配置的所有参数，并存入map中
    // bool InitPostProcessCfgToMap();

public:

    //生成校正文件需要的float数组，大小根据读出的配置项来定
    //<BPDetect	Para = "Border_W=15;Border_B=75;Border_D=14;GlobalBP_W=10;GlobalBP_T=30;SpatialBP_NW=3;SpatialBP_T1=3.5;
    //SpatialBP_T2=2;SpatialBP_T4=20;SpatialBP_LNW=7;SpatialBP_LR=0.33" / >
    float* m_pfGenCorrectFilePara;

    //进行校正处理需要的参数，大小根据读出的配置项来定
    //<BPCorrect Para = "NBP_LS=10;NBP_HS=5535;NBP_AT=0.01;MixSig=0.2;DL_CT=6.8;DL_GT=0.3;DL_WW=13;IPM_OAIMZC=1" / >
    float* m_pfDoCorrectPara;
    size_t m_nCorrectParaLength;

    //预处理需要的参数

    //int m_iMethod;

    //从配置文件读出的参数数组,一般为7个参数，此为预处理参数，由于几种风格的预处理参数是一致的，所以只用一个预处理参数
    //float* m_pfProcPara;

    //Alg_ImagePostProcess需要调用的参数
    //需要循环的次数
    //int m_iMethodCount;

    //对应的参数
    //Tooth_Factory
    //Tooth_NGNP
    //Tooth_1
    //Tooth_2
    //Tooth_3

    // PostProcPara* m_pfPostProcPara_Tooth_Factory;
    // PostProcPara* m_pfPostProcPara_Tooth_NGNP;
    // PostProcPara* m_pfPostProcPara_1;
    // PostProcPara* m_pfPostProcPara_2;
    // PostProcPara* m_pfPostProcPara_3;
    PostProcPara* m_pfPostTooth;
    // PostProcPara* m_pfPostTooth_SR;

    //是否开启AI算法
    //bool m_bInitAiAlg;

    std::map<int, float*> m_PostProcParaMap;

private:

    std::string m_sConfigPath;
};
#endif // IMAGEPROCONFIG_H
