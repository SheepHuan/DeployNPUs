#include "MyBPU.hpp"
#include "Timer.hpp"
#include "Helper.hpp"
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

int batch_benchmark(const char *model, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, LatencyPerfData>> &batch_perf_results);
int main(int argc, char **argv)
{
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;      // 设置日志消息是否转到标准输出而不是日志文件
    FLAGS_alsologtostderr = true;  // 设置日志消息除了日志文件之外是否去标准输出
    FLAGS_colorlogtostderr = true; // 设置记录到标准输出的颜色消息（如果终端支持）
    FLAGS_log_prefix = true;       // 设置日志前缀是否应该添加到每行输出
    FLAGS_logbufsecs = 0;          // 设置可以缓冲日志的最大秒数，0指实时输出

    char const *hbdnn_version = hbDNNGetVersion();
    LOG(INFO) << "HB DNN Version: " << hbdnn_version;

    std::string model = FLAGS_model;
    int num_warmup = FLAGS_num_warmup;
    int num_run = FLAGS_num_run;
    bool enable_profiling = FLAGS_enable_profiling;
    bool enable_batch_benchmark = FLAGS_enable_batch_benchmark;
    std::vector<std::tuple<std::string, LatencyPerfData>> batch_perf_results;
    if (enable_batch_benchmark == false)
    {

        batch_benchmark(model.c_str(), num_warmup, num_run, enable_profiling, batch_perf_results);
    }
    else
    {
        std::vector<std::filesystem::path> files = glob_files(model, ".bin");
        for (auto path : files)
        {
            LOG(INFO) << "Find model: " << path;
            batch_benchmark(path.c_str(), num_warmup, num_run, enable_profiling, batch_perf_results);
        }
    }
    std::ostringstream pt_oss;
    pt_oss << "\nmodel\t avg\t std\t min\t max\n";
    for (const auto &perf_result : batch_perf_results)
    {
        pt_oss << std::get<0>(perf_result) << "\t" << std::to_string(std::get<1>(perf_result).mean) << "\t" << std::to_string(std::get<1>(perf_result).stdev) << "\t" << std::to_string(std::get<1>(perf_result).min) << "\t" << std::to_string(std::get<1>(perf_result).max) << "\n";
    }
    LOG(INFO) << pt_oss.str();
    google::ShutdownGoogleLogging();
    return 0;
}

int batch_benchmark(const char *model, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, LatencyPerfData>> &batch_perf_results)
{
    const char **modelFileNames = new const char *[1];
    hbPackedDNNHandle_t packedDNNHandle;
    hbDNNHandle_t dnnHandle;

    modelFileNames[0] = model;
    int ret = hbDNNInitializeFromFiles(&packedDNNHandle, modelFileNames, 1);
    if (ret != HB_SYS_SUCCESS)
    {
        LOG(ERROR) << "Failed to initialize packed DNN.";
        return -1;
    }
    char const **modelNameList = nullptr;
    int32_t modelNameCount;
    ret = hbDNNGetModelNameList(&modelNameList,
                                &modelNameCount,
                                packedDNNHandle);

    if (ret != HB_SYS_SUCCESS)
    {
        LOG(ERROR) << "Failed to get model name list.";
        return -1;
    }
    for (int i = 0; i < modelNameCount; i++)
    {
        LOG(INFO) << "modelName: " << modelNameList[i];
        ret = hbDNNGetModelHandle(&dnnHandle,
                                  packedDNNHandle,
                                  modelNameList[i]);
        if (ret != HB_SYS_SUCCESS)
        {
            LOG(ERROR) << "Failed to get model handle.";
            return -1;
        }
        int32_t inputCount;
        ret = hbDNNGetInputCount(&inputCount, dnnHandle);
        if (ret != HB_SYS_SUCCESS)
        {
            LOG(ERROR) << "Failed to get input count.";
            return -1;
        }
        LOG(INFO) << "inputCount: " << inputCount;
        int32_t outputCount;
        ret = hbDNNGetOutputCount(&outputCount, dnnHandle);
        if (ret != HB_SYS_SUCCESS)
        {
            LOG(ERROR) << "Failed to get output count.";
            return -1;
        }

        hbDNNTensorProperties *inputProperties = new hbDNNTensorProperties[inputCount];
        hbDNNTensor *inputTensor = new hbDNNTensor[inputCount];
        for (int index = 0; index < inputCount; index++)
        {
            ret = hbDNNGetInputTensorProperties(&inputProperties[index], dnnHandle, index);
            if (ret != HB_SYS_SUCCESS)
            {
                LOG(ERROR) << "Failed to get input tensor properties.";
                return -1;
            }
            const char *inputName;
            ret = hbDNNGetInputName(&inputName, dnnHandle, index);
            dump_tensor_properties(index, inputName, inputProperties[index], true);

            inputTensor[index].properties = inputProperties[index];
            uint32_t size = inputTensor[index].properties.alignedByteSize;
            ret = hbSysAllocMem(&inputTensor[index].sysMem[0], size);
            if (ret != HB_SYS_SUCCESS)
            {
                LOG(ERROR) << "Failed to allocate input tensor memory.";
                return -1;
            }
        }
        hbDNNTensorProperties *outputProperties = new hbDNNTensorProperties[outputCount];
        hbDNNTensor *outputTensor = new hbDNNTensor[outputCount];
        for (int index = 0; index < outputCount; index++)
        {
            ret = hbDNNGetOutputTensorProperties(&outputProperties[index], dnnHandle, index);
            if (ret != HB_SYS_SUCCESS)
            {
                LOG(ERROR) << "Failed to get output tensor properties.";
                return -1;
            }
            outputTensor[index].properties = outputProperties[index];
            uint32_t size = outputTensor[index].properties.alignedByteSize;
            ret = hbSysAllocMem(&outputTensor[index].sysMem[0], size);
            if (ret != HB_SYS_SUCCESS)
            {
                LOG(ERROR) << "Failed to allocate output tensor memory.";
                return -1;
            }
        }

        auto benchmark_function = [](hbDNNHandle_t dnnHandle, hbDNNTensor *inputTensor, hbDNNTensor *outputTensor)
        {
            hbDNNTaskHandle_t taskHandle = nullptr;
            hbDNNInferCtrlParam inferCtrlParam = {
                .bpuCoreId = 0,
                .dspCoreId = 0,
                .priority = 90,
                .more = 0,
                .customId = 0,
                .reserved1 = 0,
                .reserved2 = 0};

            int ret = hbDNNInfer(&taskHandle, &outputTensor, inputTensor, dnnHandle, &inferCtrlParam);
            if (ret != HB_SYS_SUCCESS)
            {
                LOG(ERROR) << "Failed to run inference. Return code: " << ret;
                exit(1);
            }
            ret = hbDNNWaitTaskDone(taskHandle, 0);
            if (ret != HB_SYS_SUCCESS)
            {
                LOG(ERROR) << "Failed to wait task done. Return code: " << ret;
                exit(1);
            }
            ret = hbDNNReleaseTask(taskHandle);
            if (ret != HB_SYS_SUCCESS)
            {
                LOG(ERROR) << "Failed to release task. Return code: " << ret;
                exit(1);
            }
        };
        Timer timer(num_warmup, num_run, benchmark_function, dnnHandle, inputTensor, outputTensor);
        timer.run();
        auto data = timer.report();
        batch_perf_results.push_back(std::make_tuple(model, std::get<1>(data)));

        delete[] inputProperties;
        delete[] outputProperties;
        delete[] inputTensor;
        delete[] outputTensor;
    }

    delete[] modelFileNames;
    return 0;
}