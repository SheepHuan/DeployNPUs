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
    LOG(INFO) << "Profiling model:" << model;
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
        LOG(INFO) << "Performance detail:";
        LOG(INFO) << perf_detail.perf_data;
        
        // 将性能详情添加到JSON结果中
        result["ModelAnalysisResult"]["RKNN_API_PerformanceDetail"] = perf_detail.perf_data;
    }

    // // 查询运行时性能数据
    // rknn_perf_run perf_run;
    // ret = rknn_query(ctx, RKNN_QUERY_PERF_RUN, &perf_run, sizeof(perf_run));
    // if (ret != RKNN_SUCC) {
    //     LOG(ERROR) << "Failed to query performance run, ret=" << ret;
    // } else {
    //     LOG(INFO) << "Performance run:";
    //     LOG(INFO) << "  system occupy CPU ratio: " << perf_run.run_duration << " us";
    //     // LOG(INFO) << "  NPU running time: " << perf_run.execute_time << " us";
    //     // LOG(INFO) << "  CPU running time: " << perf_run.cpu_execute_time << " us";
        
    //     // // 将运行时性能数据添加到JSON结果中
    //     // result["ModelAnalysisResult"]["RuntimeResult"]["SystemOccupyCPURatio"] = perf_run.system_occupy_ratio;
    //     // result["ModelAnalysisResult"]["RuntimeResult"]["NPUExecuteTime"] = perf_run.execute_time;
    //     // result["ModelAnalysisResult"]["RuntimeResult"]["CPUExecuteTime"] = perf_run.cpu_execute_time;
    // }

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
    rknn_destroy(ctx);

    // 设置模型分析结果
    result["ModelAnalysisResult"]["RuntimeResult"]["Warmups"] = num_warmup;
    result["ModelAnalysisResult"]["RuntimeResult"]["Rounds"] = num_run;
    result["ModelAnalysisResult"]["RuntimeResult"]["InitTime"] = init_time;
    result["ModelAnalysisResult"]["RuntimeResult"]["InitMemory"] = mem_size.total_weight_size / 1024.0 / 1024.0;  // 转换为 MB
    
    // 设置平均延迟等统计数据
    result["ModelAnalysisResult"]["RuntimeResult"]["AvgTotalRoundLatency"] = std::get<1>(data).mean;
    result["ModelAnalysisResult"]["RuntimeResult"]["AvgPeakMemory"] = 
        (mem_size.total_weight_size + mem_size.total_internal_size) / 1024.0 / 1024.0;  // 总内存使用（MB）
    result["ModelAnalysisResult"]["RuntimeResult"]["AvgPeakPower"] = 0.0;  // 如果有功耗数据，在这里设置
    result["ModelAnalysisResult"]["RuntimeResult"]["StdTotalRoundLatency"] = std::get<1>(data).stdev;
    result["ModelAnalysisResult"]["RuntimeResult"]["MinTotalRoundLatency"] = std::get<1>(data).min;
    result["ModelAnalysisResult"]["RuntimeResult"]["MaxTotalRoundLatency"] = std::get<1>(data).max;
    
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
        result["ModelAnalysisResult"]["RuntimeResult"]["MultiRoundsProfileResult"].push_back(round);
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
        result["ModelAnalysisResult"]["RuntimeResult"]["MultiRoundsProfileResult"].push_back(round);
    }

    

    LOG(INFO) << "Profiling model:" << model << " done!";
    return 0;
}