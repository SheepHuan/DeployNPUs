#include "openvino/openvino.hpp"
#include "glog/logging.h"
#include "gflags/gflags.h"


// 定义模型文件的路径
DEFINE_string(model, "path", "The file path to the rknn model.");

// 定义预热运行的次数，用于模型初始化或数据预加载
DEFINE_int32(num_warmup, 10, "The number of warmup runs before actual benchmarking.");

// 定义实际运行的次数，用于获取模型性能的平均值
DEFINE_int32(num_run, 10, "The number of runs to measure the model's performance.");

void query_device();

int main(int argc, char **argv)
{
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;      // 设置日志消息是否转到标准输出而不是日志文件
    FLAGS_alsologtostderr = true;  // 设置日志消息除了日志文件之外是否去标准输出
    FLAGS_colorlogtostderr = true; // 设置记录到标准输出的颜色消息（如果终端支持）
    FLAGS_log_prefix = true;       // 设置日志前缀是否应该添加到每行输出
    FLAGS_logbufsecs = 0;          // 设置可以缓冲日志的最大秒数，0指实时输出
    LOG(INFO) << "Start to query device";   
    query_device();
    google::ShutdownGoogleLogging();
    return 0;
}


void query_device()
{
    try
    {
        ov::Version version = ov::get_openvino_version();
        LOG(INFO) << "openvino version:" << version.buildNumber << ", " << version.description;
        ov::Core core;

        // -------- Step 2. Get list of available devices --------
        std::vector<std::string> availableDevices = core.get_available_devices();
        // spdlog::info("Available devices:");
        for (auto &&device : availableDevices)
        {
            // spdlog::info("{}", device);
            LOG(INFO) << "Find Deivce: " << device;
            // Query supported properties and print all of them
            // spdlog::info("\tSUPPORTED_PROPERTIES: ");
            auto supported_properties = core.get_property(device, ov::supported_properties);
            for (auto &&property : supported_properties)
            {
                std::stringstream ss;
                if (property != ov::supported_properties.name())
                {
                    ss << "\t\t" << (property.is_mutable() ? "Mutable: " : "Immutable: ") 
                       << property << " : " << core.get_property(device, property).as<std::string>();
                }
                if (property == supported_properties.back()) {
                    LOG(INFO) << "Device " << device << " properties:" << ss.str();
                }
            }

            // slog::info << slog::endl;
        }
    }
    catch (const std::exception &ex)
    {
        LOG(ERROR) << "Exception occurred: " << ex.what();
    }
}