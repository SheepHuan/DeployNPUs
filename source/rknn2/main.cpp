#include <cstdio>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstring>
#include "glog/logging.h"
#include "gflags/gflags.h"
#include "rknn_api.h"
#include "Timer.hpp"
#include <tuple>
#include <vector>
#include "tabulate.hpp"
#include "nlohmann/json.hpp"
#include <chrono>
#include <sys/utsname.h>

// 定义模型文件的路径
DEFINE_string(model, "path", "The file path to the rknn model.");

// 定义预热运行的次数，用于模型初始化或数据预加载
DEFINE_int32(num_warmup, 10, "The number of warmup runs before actual benchmarking.");

// 定义实际运行的次数，用于获取模型性能的平均值
DEFINE_int32(num_run, 10, "The number of runs to measure the model's performance.");

// 定义输出文件路径
DEFINE_string(output_file, "output/rknn_profile_result.json", "The file path to the output json file.");

// 是否对操作进行性能分析，如果设置为true，将输出操作级别的性能数据
DEFINE_bool(enable_profiling, false, "Flag to enable profiling of individual operations within the model.");

static void dump_tensor_attr(rknn_tensor_attr *attr, bool is_input = true)
{
    std::string tensor_type = is_input ? "input tensor" : "output tensor";
    LOG(INFO) << tensor_type << ", " << "index=" << attr->index << ", name=" << attr->name << ", n_dims=" << attr->n_dims
              << ", dims=[" << attr->dims[0] << ", " << attr->dims[1] << ", " << attr->dims[2] << ", " << attr->dims[3] << "], fmt=" << get_format_string(attr->fmt) << ", n_elems=" << attr->n_elems << ", size=" << attr->size << ", type=" << get_type_string(attr->type) << ", qnt_type=" << get_qnt_type_string(attr->qnt_type) << ", zp=" << attr->zp << ", scale=" << attr->scale;
}

int batch_benchmark(const char *model, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, LatencyPerfData>> &batch_perf_results, nlohmann::json &result);

int main(int argc, char *argv[])
{
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr = true;
    FLAGS_log_prefix = true;
    FLAGS_logbufsecs = 0;

    std::string model_path = FLAGS_model;
    int num_warmup = FLAGS_num_warmup;
    int num_run = FLAGS_num_run;
    bool enable_profiling = FLAGS_enable_profiling;

    std::vector<std::tuple<std::string, LatencyPerfData>> batch_perf_results;
    nlohmann::json all_models_result;  // 创建总的JSON对象

    // 检查输入路径是否为目录
    if (std::filesystem::is_directory(model_path))
    {
        // 存储找到的文件名
        std::vector<std::filesystem::path> rknn_files;

        // 遍历目录查找.rknn文件
        for (const auto &entry : std::filesystem::recursive_directory_iterator(model_path))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".rknn")
            {
                rknn_files.push_back(entry.path());
            }
        }

        // 对每个找到的模型文件进行测试
        for (const auto &file : rknn_files)
        {
            nlohmann::json model_result;  // 单个模型的结果
            batch_benchmark(file.string().c_str(), num_warmup, num_run, enable_profiling, batch_perf_results, model_result);
            all_models_result.push_back(model_result);  // 添加到总结果中
        }
    }
    else
    {
        // 修改单个模型文件测试逻辑
        nlohmann::json model_result;
        batch_benchmark(model_path.c_str(), num_warmup, num_run, enable_profiling, batch_perf_results, model_result);
        all_models_result.push_back(model_result);
    }

    // 创建输出目录（如果不存在）
    std::filesystem::path output_path(FLAGS_output_file);
    std::filesystem::create_directories(output_path.parent_path());

    // 保存所有结果到指定的输出文件
    std::ofstream json_file(FLAGS_output_file);
    json_file << std::setw(4) << all_models_result << std::endl;

    google::ShutdownGoogleLogging();
    return 0;
}

int batch_benchmark(const char *model, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, LatencyPerfData>> &batch_perf_results, nlohmann::json &result)
{
    // 获取模型文件名(不包含路径)
    std::string model_name = std::filesystem::path(model).filename().string();
    LOG(INFO) << "Profiling model:" << model_name;
    rknn_context ctx;
    int flag = RKNN_FLAG_COLLECT_PERF_MASK;
    
    // 添加初始化时间统计
    auto init_start = std::chrono::high_resolution_clock::now();
    int ret = rknn_init(&ctx, (void *)model, 0, flag, nullptr);
    auto init_end = std::chrono::high_resolution_clock::now();
    double init_time = std::chrono::duration<double, std::milli>(init_end - init_start).count();

    if (ret < 0)
    {
        LOG(ERROR) << "rknn_init fail! ret=" << ret << "\n";
        return -1;
    }
    
    LOG(INFO) << "Model initialization time: " << init_time << " ms";

    // 在模型初始化后，查询内存使用情况
    rknn_mem_size mem_size;
    ret = rknn_query(ctx, RKNN_QUERY_MEM_SIZE, &mem_size, sizeof(mem_size));
    if (ret != RKNN_SUCC) {
        LOG(ERROR) << "Failed to query memory size, ret=" << ret;
    } else {
        LOG(INFO) << "Model memory usage:";
        LOG(INFO) << "  total memory size: " << mem_size.total_weight_size / 1024.0 / 1024.0 << " MB";
        LOG(INFO) << "  weight memory size: " << mem_size.total_weight_size / 1024.0 / 1024.0 << " MB";
        LOG(INFO) << "  internal memory size: " << mem_size.total_internal_size / 1024.0 / 1024.0 << " MB";
    }

    // Get Model Input Output Number
    rknn_input_output_num io_num;
    ret = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != RKNN_SUCC)
    {
        LOG(ERROR) << "rknn_query fail! ret=" << ret << "\n";
        return -1;
    }
    LOG(INFO) << "model input num: " << io_num.n_input << ", output num: " << io_num.n_output;
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    rknn_input inputs[io_num.n_input];
    memset(inputs, 0, sizeof(inputs));
    for (int i = 0; i < io_num.n_input; i++)
    {
        input_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            LOG(ERROR) << "rknn_query fail! ret="
                       << ret << "\n";
            return -1;
        }
        dump_tensor_attr(&(input_attrs[i]), true);
        inputs[i].index = input_attrs[i].index;
        inputs[i].type = input_attrs[i].type;
        inputs[i].size = input_attrs[i].size;
        inputs[i].fmt = input_attrs[i].fmt;
        inputs[i].buf = malloc(inputs[i].size);
    }
    // Get Model Output Info
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    rknn_output outputs[io_num.n_output];
    memset(outputs, 0, sizeof(outputs));
    for (int i = 0; i < io_num.n_output; i++)
    {
        output_attrs[i].index = i;
        ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC)
        {
            LOG(ERROR) << "rknn_query fail! ret="
                       << ret << "\n";
            return -1;
        }
        outputs[i].index = output_attrs[i].index;
        outputs[i].size = output_attrs[i].size;
        outputs[i].buf = malloc(output_attrs[i].size);
        outputs[i].is_prealloc = true;
        dump_tensor_attr(&(output_attrs[i]), false);
    }

    ret = rknn_inputs_set(ctx, io_num.n_input, inputs);
    if (ret < 0)
    {
        LOG(ERROR) << "rknn_input_set fail! ret=" << ret << "\n";
        // printf("rknn_input_set fail! ret=%d\n", ret);
        return -1;
    }
    auto benchmark_function = [](rknn_context ctx)
    {
        int ret = rknn_run(ctx, nullptr);
        if (ret < 0)
        {
            LOG(ERROR) << "rknn_run fail! ret=" << ret << "\n";
            // printf("rknn_run fail! ret=%d\n", ret);
            // return -1;
        }
    };
    Timer timer(num_warmup, num_run, benchmark_function, ctx);

    timer.run();
    auto data = timer.report();
    batch_perf_results.push_back(std::make_tuple(model, std::get<1>(data)));

    ret = rknn_outputs_get(ctx, io_num.n_output, outputs, nullptr);
    if (ret < 0)
    {
        LOG(ERROR) << "rknn_outputs_get fail! ret=" << ret << "\n";
        // printf("rknn_outputs_get fail! ret=%d\n", ret);
    }
    rknn_perf_detail perf_detail;

    // 在模型运行完成后，查询性能详情
    ret = rknn_query(ctx, RKNN_QUERY_PERF_DETAIL, &perf_detail, sizeof(perf_detail));
    if (ret != RKNN_SUCC) {
        LOG(ERROR) << "Failed to query performance detail, ret=" << ret;
    } else if (enable_profiling) {
        LOG(INFO) << "Performance detail for " << model_name << ":";
        LOG(INFO) << perf_detail.perf_data;
        
        // 使用模型名称作为键
        result[model_name]["RKNN_API_PerformanceDetail"] = perf_detail.perf_data;
    }


    for (int i = 0; i < io_num.n_input; i++)
    {
        if (inputs[i].buf != nullptr)
        {
            free(inputs[i].buf);
        }
    }
    for (int i = 0; i < io_num.n_output; i++)
    {
        if (outputs[i].buf != nullptr)
        {
            free(outputs[i].buf);
        }
    }

 // // 获取RKNN版本信息
    rknn_sdk_version version;
    ret = rknn_query(ctx, RKNN_QUERY_SDK_VERSION, &version, sizeof(version));
    if (ret == RKNN_SUCC) {
        result[model_name]["MetaInfo"]["BackendName"] = "RKNN";
        result[model_name]["MetaInfo"]["BackendVersion"] = std::string(version.api_version) + 
                                                 " (driver version: " + version.drv_version + ")";
        LOG(INFO) << "RKNN SDK Version: " << version.api_version << " (driver version: " << version.drv_version << ")";
    }

    rknn_destroy(ctx);

    // 设置模型分析结果
    result[model_name]["RuntimeResult"]["Warmups"] = num_warmup;
    result[model_name]["RuntimeResult"]["Rounds"] = num_run;
    result[model_name]["RuntimeResult"]["InitTime"] = init_time;
    result[model_name]["RuntimeResult"]["InitMemory"] = mem_size.total_weight_size / 1024.0 / 1024.0;  // 转换为 MB
    
    // 设置平均延迟等统计数据
    result[model_name]["RuntimeResult"]["AvgTotalRoundLatency"] = std::get<1>(data).mean;
    result[model_name]["RuntimeResult"]["AvgPeakMemory"] = 
        (mem_size.total_weight_size + mem_size.total_internal_size) / 1024.0 / 1024.0;  // 总内存使用（MB）
    result[model_name]["RuntimeResult"]["AvgPeakPower"] = 0.0;  // 如果有功耗数据，在这里设置
    result[model_name]["RuntimeResult"]["StdTotalRoundLatency"] = std::get<1>(data).stdev;
    result[model_name]["RuntimeResult"]["MinTotalRoundLatency"] = std::get<1>(data).min;
    result[model_name]["RuntimeResult"]["MaxTotalRoundLatency"] = std::get<1>(data).max;
    
    // 添加每轮运行结果
    const auto& raw_times = timer.durations_warmup_;  // 预热轮次
    for(size_t i = 0; i < raw_times.size(); i++) {
        nlohmann::json round;
        round["RoundIndex"] = -1;
        round["WarmupIndex"] = i;
        round["TotalRoundLatency"] = raw_times[i];
        round["TotalRoundPeakMemory"] = 
            (mem_size.total_weight_size + mem_size.total_internal_size) / 1024.0 / 1024.0;
        round["TotalRoundAvgPower"] = 0.0;  // 如果有功耗数据，在这里设置
        result[model_name]["RuntimeResult"]["MultiRoundsProfileResult"].push_back(round);
    }
    
    const auto& normal_times = timer.durations_normal_;  // 正式运行轮次
    for(size_t i = 0; i < normal_times.size(); i++) {
        nlohmann::json round;
        round["RoundIndex"] = i;
        round["WarmupIndex"] = -1;
        round["TotalRoundLatency"] = normal_times[i];
        round["TotalRoundPeakMemory"] = 
            (mem_size.total_weight_size + mem_size.total_internal_size) / 1024.0 / 1024.0;
        round["TotalRoundAvgPower"] = 0.0;  // 如果有功耗数据，在这里设置
        result[model_name]["RuntimeResult"]["MultiRoundsProfileResult"].push_back(round);
    }

    // 添加元数据信息
    result[model_name]["MetaInfo"]["ModelName"] = std::filesystem::path(model).filename().string();
    result[model_name]["MetaInfo"]["ModelPath"] = model;
    
    // 获取系统信息
    struct utsname system_info;
    if (uname(&system_info) == 0) {
        result[model_name]["MetaInfo"]["OSName"] = system_info.sysname;
        // result[model_name]["MetaInfo"]["OSVersion"] = system_info.release;
        result[model_name]["MetaInfo"]["Machine"] = system_info.machine;
        result[model_name]["MetaInfo"]["KernelVersion"] = system_info.version;
        result[model_name]["MetaInfo"]["KernelRelease"] = system_info.release;
    }
    
   
    
    // 获取设备信息
    // result["MetaInfo"]["Hardware"]["DeviceCount"] = 1;
    // result["MetaInfo"]["Hardware"]["Devices"].push_back("RKNN Device");
    
    // 添加时间戳
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    result[model_name]["MetaInfo"]["Timestamp"] = std::ctime(&now_time_t);

    LOG(INFO) << "Profiling model:" << model << " done!";
    return 0;
}