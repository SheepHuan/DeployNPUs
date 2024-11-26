#ifndef ARCH_TEST_HPP
#define ARCH_TEST_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <filesystem>

struct CacheInfo {
    std::string level;
    std::string type;
    int sizeKB;
};

struct CpuCoreInformation {
    int coreId;
    std::vector<float> availableFrequencies;
    std::vector<CacheInfo> cacheInfoList;
};

struct CpuInformation {
    std::string model;
    std::string vendor;
    std::string architecture;
    int coreCount;
    std::vector<CpuCoreInformation> coresInformationList;
};

std::string getCpuModel() {
#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        std::cerr << "ERROR: Could not open /proc/cpuinfo" << std::endl;
        return "";
    }

    std::string line;
    std::string modelName;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                modelName = line.substr(pos + 2);  // Skip ": "
                break;
            }
        }
    }

    return modelName;
#else
    std::cerr << "ERROR: Unsupported architecture" << std::endl;
    return "";
#endif
}

std::string getVendor() {
#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        std::cerr << "ERROR: Could not open /proc/cpuinfo" << std::endl;
        return "Unknown";
    }

    std::string line;
    std::regex vendorRegex("vendor_id\\s+:\\s+(.+)");
    std::smatch match;

    while (std::getline(cpuinfo, line)) {
        if (std::regex_search(line, match, vendorRegex)) {
            return match[1];
        }
    }
    return "Unknown";
#else
    std::cerr << "ERROR: Unsupported architecture" << std::endl;
    return "Unknown";
#endif
}

std::string getArchitecture() {
#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    std::string arch;
    FILE* fp = popen("uname -m", "r");
    if (fp) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            arch = buffer;
            arch.erase(arch.find_last_not_of(" \n\r\t") + 1);
        }
        pclose(fp);
    }
    return arch;
#else
    std::cerr << "ERROR: Unsupported architecture" << std::endl;
    return "Unknown";
#endif
}

int getCoreCount() {
#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (!cpuinfo.is_open()) {
        std::cerr << "ERROR: Could not open /proc/cpuinfo" << std::endl;
        return 0;
    }

    std::string line;
    int count = 0;

    while (std::getline(cpuinfo, line)) {
        if (line.find("processor") != std::string::npos) {
            ++count;
        }
    }
    return count;
#else
    std::cerr << "ERROR: Unsupported architecture" << std::endl;
    return 0;
#endif
}

std::vector<float> getCoreAvailableFrequencies(int coreId) {
#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(coreId) + "/cpufreq/scaling_available_frequencies";
    std::ifstream freqFile(path);

    std::vector<float> frequencies;

    if (!freqFile.is_open()) {
        std::cerr << "ERROR: Could not open frequency file for core " << coreId << std::endl;
        return frequencies;
    }

    std::string line;
    if (std::getline(freqFile, line)) {
        std::istringstream iss(line);
        std::string freqStr;
        while (iss >> freqStr) {
            float freq = std::stof(freqStr) / 1000.0;  // Convert kHz to MHz
            frequencies.push_back(freq);
        }
    }

    return frequencies;
#else
    std::cerr << "ERROR: Unsupported architecture" << std::endl;
    return {};
#endif
}

std::vector<CacheInfo> getCpuCacheInfo(int coreId) {
#if defined(__linux__) && (defined(__x86_64__) || defined(__i386__))
    std::vector<CacheInfo> cacheInfoList;
    std::string basePath = "/sys/devices/system/cpu/cpu" + std::to_string(coreId) + "/cache";

    try {
        for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
            std::string levelPath = entry.path().string() + "/level";
            std::string typePath = entry.path().string() + "/type";
            std::string sizePath = entry.path().string() + "/size";

            std::ifstream levelFile(levelPath);
            std::ifstream typeFile(typePath);
            std::ifstream sizeFile(sizePath);

            if (levelFile.is_open() && typeFile.is_open() && sizeFile.is_open()) {
                std::string level, type, sizeStr;
                std::getline(levelFile, level);
                std::getline(typeFile, type);
                std::getline(sizeFile, sizeStr);

                int sizeKB = std::stoi(sizeStr.substr(0, sizeStr.size() - 1)); // Remove trailing 'K'
                CacheInfo cacheInfo = {level, type, sizeKB};
                cacheInfoList.push_back(cacheInfo);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "ERROR: Filesystem error - " << e.what() << std::endl;
    }

    return cacheInfoList;
#else
    std::cerr << "ERROR: Unsupported architecture" << std::endl;
    return {};
#endif
}

void printCpuInformation(const CpuInformation& cpuInfo) {
    std::cout << "CPU Model: " << cpuInfo.model << std::endl;
    std::cout << "Vendor: " << cpuInfo.vendor << std::endl;
    std::cout << "Architecture: " << cpuInfo.architecture << std::endl;
    std::cout << "Core Count: " << cpuInfo.coreCount << std::endl;

    for (const auto& coreInfo : cpuInfo.coresInformationList) {
        std::cout << "Core ID: " << coreInfo.coreId << std::endl;
        std::cout << "Available Frequencies (MHz): ";
        for (const auto& freq : coreInfo.availableFrequencies) {
            std::cout << freq << " ";
        }
        std::cout << std::endl;
        for (const auto& cacheInfo : coreInfo.cacheInfoList) {
            std::cout << "Cache Level: " << cacheInfo.level
                      << ", Type: " << cacheInfo.type
                      << ", Size: " << cacheInfo.sizeKB << " KB" << std::endl;
        }
        std::cout << std::endl;
    }
}

#endif