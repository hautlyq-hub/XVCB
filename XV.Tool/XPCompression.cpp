#include "XPCompression.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#include <minizip/unzip.h>

XPCompression::XPCompression() {}

bool XPCompression::Unzip(const std::string &zipPath, const std::string &outputDir)
{
    // 打开 ZIP 文件
    unzFile zipfile = unzOpen(zipPath.c_str());
    if (!zipfile) {
        std::cerr << "无法打开ZIP文件: " << zipPath << std::endl;
        return false;
    }

    // 获取ZIP文件信息
    unz_global_info global_info;
    if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK) {
        std::cerr << "读取ZIP信息失败" << std::endl;
        unzClose(zipfile);
        return false;
    }

    // 创建输出目录
    std::filesystem::create_directories(outputDir);

    // 循环解压所有文件
    char filename[256];
    unz_file_info file_info;
    for (int i = 0; i < static_cast<int>(global_info.number_entry); ++i) {
        if (unzGetCurrentFileInfo(
                zipfile, &file_info, filename, sizeof(filename), nullptr, 0, nullptr, 0)
            != UNZ_OK) {
            std::cerr << "读取文件信息失败: " << i << std::endl;
            continue;
        }

        std::string fullPath = outputDir + "/" + filename;

        // 如果是目录则创建
        if (filename[strlen(filename) - 1] == '/') {
            std::filesystem::create_directories(fullPath);
        } else {
            // 打开压缩文件
            if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
                std::cerr << "打开压缩文件失败: " << filename << std::endl;
                continue;
            }

            // 创建输出文件
            FILE *out = fopen(fullPath.c_str(), "wb");
            if (!out) {
                std::cerr << "创建文件失败: " << fullPath << std::endl;
                unzCloseCurrentFile(zipfile);
                continue;
            }

            // 写入数据
            char buffer[4096];
            int bytes;
            while ((bytes = unzReadCurrentFile(zipfile, buffer, sizeof(buffer))) > 0) {
                fwrite(buffer, 1, bytes, out);
            }

            // 关闭文件
            fclose(out);
            unzCloseCurrentFile(zipfile);
        }

        // 处理下一个文件
        if (i + 1 < static_cast<int>(global_info.number_entry)) {
            if (unzGoToNextFile(zipfile) != UNZ_OK) {
                std::cerr << "遍历ZIP文件失败" << std::endl;
                break;
            }
        }
    }

    unzClose(zipfile);
    return true;
}
