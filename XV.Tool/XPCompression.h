#ifndef XPCOMPRESSION_H
#define XPCOMPRESSION_H

#include <string>

class XPCompression
{
public:
    XPCompression();

    /**
     * @brief 解压 ZIP 文件到指定目录
     * 
     * @param zipPath ZIP 文件的路径
     * @param outputDir 解压输出目录
     * @return true 解压成功
     * @return false 解压失败
     */
    bool Unzip(const std::string& zipPath, const std::string& outputDir);
};

#endif // XPCOMPRESSION_H
