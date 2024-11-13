#include "HiAiModelManagerService.h"
#include <graph/attr_value.h>
#include <graph/operator_hiai_reg.h>
#include <graph/op/all_ops.h>
#include <graph/compatible/operator_reg.h>
#include <graph/graph.h>
#include <graph/model.h>
#include <graph/compatible/all_ops.h>
#include <hiai_ir_build.h>
#include <graph/buffer.h>
#include <vector>
#include "Timer.hpp"
#include "glog/logging.h"
#include "gflags/gflags.h"
#include <filesystem>
#include "tabulate.hpp"

// 定义模型文件的路径
DEFINE_string(model, "path", "The file path to the rknn model.");

// 定义预热运行的次数，用于模型初始化或数据预加载
DEFINE_int32(num_warmup, 10, "The number of warmup runs before actual benchmarking.");

// 定义实际运行的次数，用于获取模型性能的平均值
DEFINE_int32(num_run, 10, "The number of runs to measure the model's performance.");

// 是否对操作进行性能分析，如果设置为true，将输出操作级别的性能数据
DEFINE_bool(enable_profiling, false, "Flag to enable profiling of individual operations within the model.");

// 批量基准测试
DEFINE_bool(enable_batch_benchmark, false, "Flag to enable batch benchmark performance.");

#define CHECK_STATUS(ret)                                                                                        \
    if ((ret) != hiai::SUCCESS)                                                                                  \
    {                                                                                                            \
        LOG(INFO) << "Error: " << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ":" << ret << std::endl; \
       exit(-1) ;                                                                                             \
    }

int batch_benchmark(const char *model_path, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, LatencyPerfData>> &batch_perf_results);

int main(int argc, char **argv)
{
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;      // 设置日志消息是否转到标准输出而不是日志文件
    FLAGS_alsologtostderr = true;  // 设置日志消息除了日志文件之外是否去标准输出
    FLAGS_colorlogtostderr = true; // 设置记录到标准输出的颜色消息（如果终端支持）
    FLAGS_log_prefix = true;       // 设置日志前缀是否应该添加到每行输出
    FLAGS_logbufsecs = 0;          // 设置可以缓冲日志的最大秒数，0指实时输出

    std::string model = FLAGS_model;
    int num_warmup = FLAGS_num_warmup;
    int num_run = FLAGS_num_run;
    bool enable_profiling = FLAGS_enable_profiling;
    bool enable_batch_benchmark = FLAGS_enable_batch_benchmark;

    {
        std::string directoryPath = model;

        // 存储找到的文件名
        std::vector<std::filesystem::path> omFiles;

        // 遍历目录
        for (const auto &entry : std::filesystem::recursive_directory_iterator(directoryPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".om")
            {
                omFiles.push_back(entry.path());
            }
        }
        std::vector<std::tuple<std::string, LatencyPerfData>> batch_perf_results;
        // 打印所有找到的文件
        for (const auto &file : omFiles)
        {
            // std::cout << file << std::endl;
            batch_benchmark(file.string().c_str(), num_warmup, num_run, enable_profiling, batch_perf_results);
        }
        // std::ostringstream pt_oss;
        tabulate::Table profileTable;
        // pt_oss << "\nmodel\t avg\t std\t min\t max\n";
        profileTable.add_row({"model", "avg", "std", "min", "max"});
        for (const auto &perf_result : batch_perf_results)
        {

            profileTable.add_row({std::get<0>(perf_result),
                            std::to_string(std::get<1>(perf_result).mean),
                            std::to_string(std::get<1>(perf_result).stdev),
                            std::to_string(std::get<1>(perf_result).max),
                            std::to_string(std::get<1>(perf_result).min)});
        }
          // center-align and color header cells
        for (size_t i = 0; i < 5; ++i) {
            profileTable[0][i].format()
            .font_color(tabulate::Color::yellow)
            .font_align(tabulate::FontAlign::center)
            .font_style({tabulate::FontStyle::bold});
        }
        LOG(INFO) <<"\n" <<profileTable <<"\n";
        
    }
    google::ShutdownGoogleLogging();
    return 0;
}

int batch_benchmark(const char *model_path, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, LatencyPerfData>> &batch_perf_results)
{
    LOG(INFO)<<"Profiling model:"<< model_path;
    // 第一阶段
    hiai::Status ret;

    hiai::ModelInitOptions initOptions;
    initOptions.buildOptions.precisionMode = hiai::PrecisionMode::PRECISION_MODE_FP16;
    initOptions.perfMode = hiai::PerfMode::HIGH;

    std::shared_ptr<hiai::IBuiltModel> builtModel = hiai::CreateBuiltModel();
    CHECK_STATUS(builtModel->RestoreFromFile(model_path));

    std::shared_ptr<hiai::IModelManager> modelManager = hiai::CreateModelManager();
    CHECK_STATUS(modelManager->Init(initOptions, builtModel, nullptr));

    std::vector<std::shared_ptr<hiai::INDTensorBuffer>> inputTensors;
    std::vector<std::shared_ptr<hiai::INDTensorBuffer>> outputTensors;
    std::vector<hiai::NDTensorDesc> inputDesc = builtModel->GetInputTensorDescs();
    for (size_t i = 0; i < inputDesc.size(); i++)
    {
        std::shared_ptr<hiai::INDTensorBuffer> inputTensorBuffer = hiai::CreateNDTensorBuffer(inputDesc[i]);
        inputTensors.push_back(inputTensorBuffer);
    }
    std::vector<hiai::NDTensorDesc> outputDesc = builtModel->GetOutputTensorDescs();
    for (size_t i = 0; i < outputDesc.size(); i++)
    {
        std::shared_ptr<hiai::INDTensorBuffer> outputTensorBuffer = hiai::CreateNDTensorBuffer(outputDesc[i]);
        outputTensors.push_back(outputTensorBuffer);
    }
   
    auto benchmark_function = [](std::shared_ptr<hiai::IModelManager> modelManager, std::vector<std::shared_ptr<hiai::INDTensorBuffer>> inputTensors, std::vector<std::shared_ptr<hiai::INDTensorBuffer>> outputTensors)
    {
        CHECK_STATUS(modelManager->Run(*(const_cast<std::vector<std::shared_ptr<hiai::INDTensorBuffer>> *>(&inputTensors)), *(const_cast<std::vector<std::shared_ptr<hiai::INDTensorBuffer>> *>(&outputTensors))));
    };

    Timer timer(num_warmup, num_run, benchmark_function, modelManager, inputTensors, outputTensors);
    timer.run();
    auto data = timer.report();
    batch_perf_results.push_back(std::make_tuple(model_path, std::get<1>(data)));
   
    return 0;
}