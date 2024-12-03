#include "openvino/openvino.hpp"
#include <cstdint>
#include "glog/logging.h"
#include "gflags/gflags.h"
#include "Timer.hpp"
#include <filesystem>
#include "tabulate.hpp"
// 定义模型文件的路径
DEFINE_string(model, "model path", "The file path to the rknn model.");
// DEFINE_string(bin, "bin path", "The file path to the rknn model.");
DEFINE_string(device, "MYRIAD", "The device to run the model.");
// 定义预热运行的次数，用于模型初始化或数据预加载
DEFINE_int32(num_warmup, 1, "The number of warmup runs before actual benchmarking.");
// 定义实际运行的次数，用于获取模型性能的平均值
DEFINE_int32(num_run, 10, "The number of runs to measure the model's performance.");

void query_device();
void copy_tensor_data(ov::Tensor &dst, const ov::Tensor &src);
int batch_benchmark(const char *model_path, const char *bin_path, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, LatencyPerfData>> &batch_perf_results);

int main(int argc, char **argv)
{
    // 参考 https://github.com/openvinotoolkit/openvino/blob/master/samples/c/hello_classification/main.c
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;      // 设置日志消息是否转到标准输出而不是日志文件
    FLAGS_alsologtostderr = true;  // 设置日志消息除了日志文件之外是否去标准输出
    FLAGS_colorlogtostderr = true; // 设置记录到标准输出的颜色消息（如果终端支持）
    FLAGS_log_prefix = true;       // 设置日志前缀是否应该添加到每行输出
    FLAGS_logbufsecs = 0;          // 设置可以缓冲日志的最大秒数，0指实时输出
    query_device();
    std::string model = FLAGS_model;
    int num_warmup = FLAGS_num_warmup;
    int num_run = FLAGS_num_run;
    bool enable_profiling = false;
    bool enable_batch_benchmark = true;

    std::vector<std::tuple<std::string, LatencyPerfData>> batch_perf_results;

    if (std::filesystem::is_regular_file(model))
    {
        auto bin_path = std::filesystem::path(model);
        bin_path.replace_extension(".bin");
        if (std::filesystem::exists(bin_path))
        {
            batch_benchmark(model.c_str(), bin_path.string().c_str(),
                            num_warmup, num_run, enable_profiling, batch_perf_results);
        }
        else
        {
            LOG(ERROR) << "Cannot find corresponding .bin file for: " << model;
            return -1;
        }
    }
    else if (std::filesystem::is_directory(model))
    {
        std::string directoryPath = model;

        // 存储找到的文件名
        std::vector<std::filesystem::path> modelFiles;
        std::vector<std::filesystem::path> binFiles;

        // 遍历目录
        for (const auto &entry : std::filesystem::recursive_directory_iterator(directoryPath))
        {
            // LOG(INFO) << "entry: " << entry.path().string();
            if (entry.is_regular_file() && entry.path().extension() == ".xml")
            {
                // 查找对应的bin文件
                auto binPath = entry.path();
                binPath.replace_extension(".bin");
                LOG(INFO) << "binPath: " << binPath.string();
                if (std::filesystem::exists(binPath))
                {
                    binFiles.push_back(binPath);
                    modelFiles.push_back(entry.path());
                }
            }
        }

        // 打印所有找到的文件
        for (size_t i = 0; i < modelFiles.size(); i++)
        {
            const auto &model_file = modelFiles[i];
            const auto &bin_file = binFiles[i];
            LOG(INFO) << "model_file: " << model_file.string() << ", bin_file: " << bin_file.string();
            batch_benchmark(model_file.string().c_str(), bin_file.string().c_str(), num_warmup, num_run, enable_profiling, batch_perf_results);
        }
    }
    if (batch_perf_results.size() > 0)
    {
        // tabulate::Table profileTable;
        // profileTable.add_row({"model", "avg", "std", "min", "max"});
        // for (const auto &perf_result : batch_perf_results)
        // {

        //     profileTable.add_row({std::get<0>(perf_result),
        //                           std::to_string(std::get<1>(perf_result).mean),
        //                           std::to_string(std::get<1>(perf_result).stdev),
        //                           std::to_string(std::get<1>(perf_result).max),
        //                           std::to_string(std::get<1>(perf_result).min)});
        // }
        // // center-align and color header cells
        // for (size_t i = 0; i < 5; ++i)
        // {
        //     profileTable[0][i].format().font_color(tabulate::Color::yellow).font_align(tabulate::FontAlign::center).font_style({tabulate::FontStyle::bold});
        // }
        // LOG(INFO) << "\n"
        //           << profileTable << "\n";

        tabulate::Table profileTable;
        profileTable.add_row({"index", "model", "avg", "std", "min", "max"});
        int iteration = 0;
        for (const auto &perf_result : batch_perf_results)
        {

            profileTable.add_row({std::to_string(iteration),
                                  std::get<0>(perf_result),
                                  std::to_string(std::get<1>(perf_result).mean),
                                  std::to_string(std::get<1>(perf_result).stdev),
                                  std::to_string(std::get<1>(perf_result).max),
                                  std::to_string(std::get<1>(perf_result).min)});
            ++iteration;
        }
        // center-align and color header cells
        for (size_t i = 0; i < 5; ++i)
        {
            profileTable[0][i].format().font_color(tabulate::Color::yellow).font_align(tabulate::FontAlign::center).font_style({tabulate::FontStyle::bold});
        }
        LOG(INFO) << "\n"
                  << profileTable << "\n";
    }

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
                if (property == supported_properties.back())
                {
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

void copy_tensor_data(ov::Tensor &dst, const ov::Tensor &src)
{
    if (src.get_shape() != dst.get_shape() || src.get_byte_size() != dst.get_byte_size())
    {
        throw std::runtime_error(
            "Source and destination tensors shapes and byte sizes are expected to be equal for data copying.");
    }

    memcpy(dst.data(), src.data(), src.get_byte_size());
}

int batch_benchmark(const char *model_path, const char *bin_path, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, LatencyPerfData>> &batch_perf_results)
{
    ov::shutdown();
    // ov::enable_profiling();
    LOG(INFO) << "Profiling model:" << model_path;
    // -------- Get OpenVINO runtime version --------
    ov::Version version = ov::get_openvino_version();
    LOG(INFO) << "openvino version:" << version.buildNumber << ", " << version.description;

    ov::Core core;

    // -------- Step 2. Read a model --------
    LOG(INFO) << "Loading model files: " << model_path;
    std::shared_ptr<ov::Model> model = core.read_model(model_path, bin_path);

    LOG(INFO) << "Device: " << core.get_versions(FLAGS_device);

    ov::CompiledModel compiledModel = core.compile_model(model, FLAGS_device);
    std::vector<ov::Output<const ov::Node>> modelInputs = compiledModel.inputs();
    std::vector<ov::Output<const ov::Node>> modelOutputs = compiledModel.outputs();
    ov::InferRequest inferRequest = compiledModel.create_infer_request();
    for (size_t j = 0; j < modelInputs.size(); j++)
    {
        const auto &input = modelInputs[j];
        const auto &shape = input.get_shape();

        // 创建随机输入数据
        size_t input_size = ov::shape_size(shape);
        std::vector<float> input_data(input_size);
        // 创建输入张量
        ov::Tensor input_tensor(input.get_element_type(), shape, input_data.data());

        auto requestTensor = inferRequest.get_tensor(input.get_any_name());
        copy_tensor_data(requestTensor, input_tensor);
    }
    auto benchmark_function = [](ov::InferRequest inferRequest)
    {
        inferRequest.infer();
    };
    // 释放资源
    modelInputs.clear();
    modelOutputs.clear();

    model.reset();
    // compiledModel.reset();

    Timer timer(num_warmup, num_run, benchmark_function, inferRequest);
    timer.run();
    auto data = timer.report();
    batch_perf_results.push_back(std::make_tuple(model_path, std::get<1>(data)));
    ov::shutdown();
    return 0;
}