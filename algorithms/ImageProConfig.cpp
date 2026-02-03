#include "ImageProConfig.h"

#include <QDebug>
#include <vector>

#include "StringUtility.h"
#include "tinyxml2.h"

ImageProConfig::ImageProConfig(std::string sConfigPath)
    : m_pfDoCorrectPara(nullptr), m_nCorrectParaLength(0)
{
    //根据图像大小和配置文件的配置项申请内存并初始化
    m_sConfigPath = sConfigPath;

    //m_iMethodCount = 0;

    //m_bInitAiAlg = false;

    //获取生成校文件正需要的参数
    GetGenCorrectFilePara();

    //获取进行校正需要的参数
    GetDoCorrectDataPara();

    //获取后处理需要的参数
    //Tooth_Factory
    //Tooth_NGNP
    //Tooth_1
    //Tooth_2
    //Tooth_3
    // m_pfPostProcPara_Tooth_Factory = new PostProcPara;
    // m_pfPostProcPara_Tooth_NGNP = new PostProcPara;
    // m_pfPostProcPara_1 = new PostProcPara;
    // m_pfPostProcPara_2 = new PostProcPara;
    // m_pfPostProcPara_3 = new PostProcPara;
    m_pfPostTooth = new PostProcPara;
    // m_pfPostTooth_SR = new PostProcPara;

    // GetProcessImageDataPara("Tooth_Factory", m_pfPostProcPara_Tooth_Factory);
    // GetProcessImageDataPara("Tooth_NGNP", m_pfPostProcPara_Tooth_NGNP);
    // GetProcessImageDataPara("Tooth_1", m_pfPostProcPara_1);
    // GetProcessImageDataPara("Tooth_2", m_pfPostProcPara_2);
    // GetProcessImageDataPara("Tooth_3", m_pfPostProcPara_3);
    GetProcessImageDataPara("TOOTH", m_pfPostTooth);
    // GetProcessImageDataPara("TOOTH_SR", m_pfPostTooth_SR);

    //读取是否初始化AI模块
    //m_bInitAiAlg = GetAiControl();

    //读取后处理参数
    // InitPostProcessCfgToMap();
}

ImageProConfig::~ImageProConfig()
{
    //需要释放参数申请的空间
    if (m_pfGenCorrectFilePara != nullptr)
    {
        delete[] m_pfGenCorrectFilePara;
        m_pfGenCorrectFilePara = nullptr;
    }

    if (m_pfDoCorrectPara != nullptr)
    {
        delete[] m_pfDoCorrectPara;
        m_pfDoCorrectPara = nullptr;
    }

    // if (m_pfPostProcPara_Tooth_Factory != nullptr)
    // {
    //     delete m_pfPostProcPara_Tooth_Factory;
    //     m_pfPostProcPara_Tooth_Factory = nullptr;
    // }

    // if (m_pfPostProcPara_Tooth_NGNP != nullptr)
    // {
    //     delete m_pfPostProcPara_Tooth_NGNP;
    //     m_pfPostProcPara_Tooth_NGNP = nullptr;
    // }

    // if (m_pfPostProcPara_1 != nullptr)
    // {
    //     delete m_pfPostProcPara_1;
    //     m_pfPostProcPara_1 = nullptr;
    // }

    // if (m_pfPostProcPara_2 != nullptr)
    // {
    //     delete m_pfPostProcPara_2;
    //     m_pfPostProcPara_2 = nullptr;
    // }

    // if (m_pfPostProcPara_3 != nullptr)
    // {
    //     delete m_pfPostProcPara_3;
    //     m_pfPostProcPara_3 = nullptr;
    // }

    if (m_pfPostTooth != nullptr)
    {
        delete m_pfPostTooth;
        m_pfPostTooth = nullptr;
    }

    // if (m_pfPostTooth_SR != nullptr)
    // {
    //     delete m_pfPostTooth_SR;
    //     m_pfPostTooth_SR = nullptr;
    // }

    auto iter = m_PostProcParaMap.begin();
    while (iter != m_PostProcParaMap.end())
    {
        float* fPara = iter->second;
        if (fPara)
        {
            delete[] fPara;
        }

        ++iter;
    }
    m_PostProcParaMap.clear();
}

bool ImageProConfig::SetDataCorrectAlgorithm(int nAlgorithm)
{
    static const int INDEX_OF_DATA_CORRECT_ALGORITHM = 7;
    if (m_nCorrectParaLength < (INDEX_OF_DATA_CORRECT_ALGORITHM + 1))
    {
        qCritical()<<("Invalid length of attribute 'Para' of node 'DataCorrect/BPCorrect' in config file.");

        return false;
    }

    //   0: AI
    //   1: Hybrid(Classical + AI)
    //   2: Classical
    if (nAlgorithm == 0 || nAlgorithm == 1 || nAlgorithm == 2)
    {
        m_pfDoCorrectPara[INDEX_OF_DATA_CORRECT_ALGORITHM] = static_cast<float>(nAlgorithm);

        return true;
    }
    else
    {
        qCritical()<<("Invalid data correct algorithm: ")<<nAlgorithm;

        return false;
    }
}

bool ImageProConfig::GetGenCorrectFilePara()
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(m_sConfigPath.c_str()) != tinyxml2::XML_SUCCESS)
    {
        qCritical()<<"Failed to load config file: "<<m_sConfigPath.c_str() <<" , Error: "<< doc.ErrorStr();

        return false;
    }

    tinyxml2::XMLElement* root = doc.RootElement();//获取根节点

    tinyxml2::XMLElement* pDataCorrectNode = root->FirstChildElement("DataCorrect");//获取DataCorrect节点
    if (pDataCorrectNode)
    {
        tinyxml2::XMLElement* pChildNode = pDataCorrectNode->FirstChildElement("BPDetect");
        if (pChildNode)
        {
            const char* ParaValue = pChildNode->Attribute("Para");
            if (ParaValue == nullptr)
            {
                qCritical()<<("No attribute 'Para' in node 'DataCorrect/BPCorrect'.");
                return false;
            }
            else
            {
                // //对ParaValue进行解析
                int paramCount = 0;
                m_pfGenCorrectFilePara = StringUtility::DataCorrectPara(ParaValue,&paramCount);
            }
        }
        else
        {
            qCritical()<<("Could not find node 'DataCorrect/BPCorrect'.");
        }
    }

    return true;
}

bool ImageProConfig::GetDoCorrectDataPara()
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(m_sConfigPath.data()) != 0)
    {
        qCritical()<<"Failed to load config file: "<< m_sConfigPath<< " , Error: "<< doc.ErrorStr();

        return false;
    }

    tinyxml2::XMLElement* root = doc.RootElement();//获取根节点

    tinyxml2::XMLElement* pDataCorrectNode = root->FirstChildElement("DataCorrect");//获取DataCorrect节点

    if (pDataCorrectNode ==nullptr)
    {
        qCritical()<<"%s文件的DataCorrect节点丢失"<<m_sConfigPath;
        return false;
    }

    tinyxml2::XMLElement* pChildNode = pDataCorrectNode->FirstChildElement("BPCorrect");

    if (pChildNode ==nullptr)
    {
        qCritical()<<"%s文件的BPCorrect节点丢失"<< m_sConfigPath.c_str();
        return false;
    }

    const char* paraValue = pChildNode->Attribute("Para");
    if (paraValue == nullptr)
    {
        qCritical()<<"%s文件的BPCorrect节点Para丢失"<< m_sConfigPath.c_str();
        return false;
    }

    // //对Para进行解析, 格式："NBP_LS=10;NBP_HS=5535;NBP_AT=0.01;MixSig=0.2;DL_CT=6.8;DL_GT=0.3;DL_WW=13;IPM_OAIMZC=1"
    int paramCount = 0;
    m_pfDoCorrectPara = StringUtility::DataCorrectPara(paraValue,&paramCount);
    m_nCorrectParaLength = paramCount;
    // At least 8 parameters
    if (paramCount < 8)
    {
        qCritical()<<"Invalid length of attribute 'Para' of node 'DataCorrect/BPCorrect' in config file "<<m_sConfigPath <<" , para: "<<paraValue;

        return false;
    }

    return true;
}

bool ImageProConfig::GetProcessImageDataPara(std::string nodeName, PostProcPara* pProcPara)
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(m_sConfigPath.data()) != 0)
    {
        qCritical()<<"Failed to load config file: "<<m_sConfigPath<<" , Error: " << doc.ErrorStr();

        return false;
    }

    tinyxml2::XMLElement* root = doc.RootElement();//获取根节点

    tinyxml2::XMLElement* pProcParaNode = root->FirstChildElement("ProcPara");//("ProcPara");ProcPara节点则只读出方法7对应的参数

    if (pProcParaNode)
    {
        tinyxml2::XMLElement* pChildNode = pProcParaNode->FirstChildElement(nodeName.data());

        //将节点ProcPara下的nodeName的所有Para属性读取出来
        if (pChildNode)
        {
            //
            //读出Method的值
            //
            const char* pMethod = pChildNode->Attribute("Method");
            //解析Method的值
            pProcPara->m_iMethod = std::atoi(pMethod);

            //
            //读出Para的值
            //
            const char* ParaValue1 = pChildNode->Attribute("Para");
            //解析Para的值 MODE=3211;GC=(0.005,0.9998);DNT=2;L=4;CTB=(0.3072,0.6144,0.9216,1.2288);NE=1;Sigma=0.5;Clip=(0.0005,0.9995);NMAL=0.7;NMAM=0.8;NMAH=0.9;MRLH=11;GL=(1.5,1.2,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0);GM=(2.5,1.5,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0);GH=(3.5,1.8,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0);QL=(10,0.3),(10,0.3),(10,0.3),(10,0.3),(10,0.3),(10,0.3),(10,0.3),(10,0.3),(10,0.3),(10,0.3),(0,0);QM=(20,0.2),(20,0.2),(20,0.2),(20,0.2),(20,0.2),(20,0.2),(20,0.2),(20,0.2),(20,0.2),(20,0.2),(0,0);QH=(20,0.2),(20,0.2),(30,0.2),(40,0.15),(40,0.1),(40,0.1),(40,0.1),(40,0.1),(40,0.1),(40,0.1),(0,0);Z=(0,0,0,0)

            unsigned int paramCount = 0;
            pProcPara->m_pfProcPara = StringUtility::ProcParaPara(ParaValue1,&paramCount);
            pProcPara->m_pfProcParaCount=paramCount;
            // std::string temp = ParaValue1;
            // temp.Replace(",", ";");
            // temp.Replace("(", "");
            // temp.Replace(")", "");
            //
            // std::vector<std::string> vProcPara;
            // StringUtility::Split(temp, vProcPara, ";");
            // size_t iSize = vProcPara.size();
            // if (iSize > 0)
            // {
            //     pProcPara->m_pfProcPara = new float[iSize];
            //     for (int i = 0; i < iSize; i++)
            //     {
            //         std::string var = vProcPara.at(i);
            //         //每次取出等号后面的值赋值给参数数组
            //         //去除空格
            //         var.Replace(" ", "");
            //         int index = var.Find("=");
            //         var = var.Mid(index + 1, var.GetLength());
            //         pProcPara->m_pfProcPara[i] = static_cast<float>(atof(var));
            //     }
            // }

            // //
            // //读出PostProc的值
            // //
            // const char* ParaValue2 = pChildNode->Attribute("PostProc");
            // //解析Para的值 Alg = 1; Limit = 1.8; Size = (3, 3) | Alg = 2; Size = (7, 7); Sigma = 2; Weight = 0.2
            // vector<std::string> vPostProcPara;
            // StringUtility::Split(ParaValue2, vPostProcPara, "|");
            // pProcPara->m_iMethodCount = vPostProcPara.size();
            // if (pProcPara->m_iMethodCount > 0)
            // {
            //     pProcPara->m_pfPostProcPara = new float* [pProcPara->m_iMethodCount];
            //     for (int i = 0; i < pProcPara->m_iMethodCount; i++)
            //     {
            //         vector<std::string> varTemp;

            //         std::string vPostProcParaTemp = vPostProcPara.at(i);
            //         vPostProcParaTemp.Replace(" ", "");
            //         vPostProcParaTemp.Replace("(", "");
            //         vPostProcParaTemp.Replace(")", "");
            //         vPostProcParaTemp.Replace(",", ";=");

            //         StringUtility::Split(vPostProcParaTemp, varTemp, ";");

            //         size_t iSize = varTemp.size();
            //         pProcPara->m_pfPostProcPara[i] = new float[(size_t)iSize + 1];

            //         for (int j = 0; j < iSize; j++)
            //         {
            //             std::string var = varTemp.at(j);

            //             int index = var.Find("=");
            //             var = var.Mid(index + 1, var.GetLength());
            //             pProcPara->m_pfPostProcPara[i][j] = static_cast<float>(atof(var));
            //         }
            //     }
            // }
        }
    }
    return true;
}


// bool ImageProConfig::InitPostProcessCfgToMap()
// {
//     tinyxml2::XMLDocument doc;
//     if (doc.LoadFile(m_sConfigPath.data()) != 0)
//     {
//         qCritical()<<("Failed to load config file: %s, Error: %s", m_sConfigPath.c_str(), doc.ErrorStr());

//         return false;
//     }

//     tinyxml2::XMLElement* root = doc.RootElement();//获取根节点

//     tinyxml2::XMLElement* pPostProcAlgorithmNode = root->FirstChildElement("PostProcAlgorithm");
//     //遍历PostProcAlgorithm节点下的所有属性的值
//     tinyxml2::XMLElement* pAttribute = pPostProcAlgorithmNode->FirstChildElement();
//     while (pAttribute)
//     {
//         const char* method = pAttribute->Attribute("Method");
//         int iKey = atoi(method);

//         const char* para = pAttribute->Attribute("Para");
//         std::string vPostProcParaTemp = para;
//         vPostProcParaTemp.Replace(" ", "");
//         vPostProcParaTemp.Replace("(", "");
//         vPostProcParaTemp.Replace(")", "");
//         vPostProcParaTemp.Replace(",", ";=");
//         std::vector<std::string> varTemp;
//         StringUtility::Split(vPostProcParaTemp, varTemp, ";");

//         size_t iSize = varTemp.size();

//         float* fPara = new float[iSize];
//         for (int i = 0; i < iSize; i++)
//         {
//             std::string var = varTemp.at(i);
//             //每次取出等号后面的值赋值给参数数组
//             int index = var.Find("=");
//             var = var.Mid(index + 1, var.GetLength());
//             fPara[i] = static_cast<float>(atof(var));
//         }

//         pAttribute = pAttribute->NextSiblingElement();

//         m_PostProcParaMap.insert(std::make_pair(iKey, fPara));
//     }
//     return true;
// }

