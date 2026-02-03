#ifndef POSTPROCPARA_H
#define POSTPROCPARA_H


class PostProcPara
{
public:
    PostProcPara()
    {
        m_iMethod = 0;
        m_iMethodCount = 0;
        m_pfProcPara = nullptr;
        m_pfPostProcPara = nullptr;
    }

    ~PostProcPara()
    {
        if (m_pfProcPara != nullptr)
        {
            delete[] m_pfProcPara;
            m_pfProcPara = nullptr;
        }

        if (m_pfPostProcPara != nullptr)
        {
            if (m_iMethodCount > 0)
            {
                for (int i = 0; i < m_iMethodCount; i++)
                {
                    if (m_pfPostProcPara[i] != nullptr)
                    {
                        delete[] m_pfPostProcPara[i];
                        m_pfPostProcPara[i] = nullptr;
                    }
                }
            }

            delete[] m_pfPostProcPara;
            m_pfPostProcPara = nullptr;
        }
    }

    //需要循环的次数
    int m_iMethodCount;

    int m_iMethod;

    //存预处理参数
    float* m_pfProcPara;
    unsigned int m_pfProcParaCount;

    //存后处理参数
    float** m_pfPostProcPara;
};
#endif // POSTPROCPARA_H
