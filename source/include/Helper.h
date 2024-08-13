#ifndef HELPER_H
#define HELPER_H
#include <filesystem>
#include <vector>
#include <string>
std::vector<std::filesystem::path> glob_files(std::string directoryPath, std::string pattern)
{
    // 存储找到的文件名
    std::vector<std::filesystem::path> foundFiles;
    // 遍历目录
    for (const auto &entry : std::filesystem::recursive_directory_iterator(directoryPath))
    {
        if (entry.is_regular_file() && entry.path().extension() == pattern)
        {
            foundFiles.push_back(entry.path());
        }
    }
    return foundFiles;
}

#endif