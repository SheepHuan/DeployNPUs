#include "glog/logging.h"
#include "gflags/gflags.h"
#include "tabulate.hpp"
#include <unistd.h>
#include <iostream>
#include "arch_test.hpp"

void lscpu();



int main(int argc, char **argv)
{
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;      // 设置日志消息是否转到标准输出而不是日志文件
    FLAGS_alsologtostderr = true;  // 设置日志消息除了日志文件之外是否去标准输出
    FLAGS_colorlogtostderr = true; // 设置记录到标准输出的颜色消息（如果终端支持）
    FLAGS_log_prefix = true;       // 设置日志前缀是否应该添加到每行输出
    FLAGS_logbufsecs = 0;          // 设置可以缓冲日志的最大秒数，0指实时输出

    
    return 0;
}

void lscpu()
{

    struct CpuInformation cpuInfo;
#if defined(__linux__) || defined(linux)
    LOG(INFO) << "Operating system is Linux.";
#endif

    cpuInfo.model = getCpuModel();
    cpuInfo.vendor = getVendor();
    cpuInfo.architecture = getArchitecture();
    cpuInfo.coreCount = getCoreCount();

    if (cpuInfo.coreCount > 0)
    {
        for (int coreId = 0; coreId < cpuInfo.coreCount; coreId++)
        {
            struct CpuCoreInformation coreInfo;
            coreInfo.coreId = coreId;
            coreInfo.availableFrequencies = getCoreAvailableFrequencies(coreId);
            coreInfo.cacheInfoList = getCpuCacheInfo(coreId);

            cpuInfo.coresInformationList.push_back(coreInfo);
        }
    }

    printCpuInformation(cpuInfo);
}