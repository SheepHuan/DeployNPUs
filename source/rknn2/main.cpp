#include <cstdio>
#include <iostream>
#include <filesystem>
#include <cstring>
#include "glog/logging.h"
#include "gflags/gflags.h"
#include "rknn_api.h"
#include "Timer.hpp"
#include <tuple>
#include <vector>
#include "tabulate.hpp"

// 定义模型文件的路径
DEFINE_string(model, "path", "The file path to the rknn model.");

// 定义预热运行的次数，用于模型初始化或数据预加载
DEFINE_int32(num_warmup, 10, "The number of warmup runs before actual benchmarking.");

// 定义实际运行的次数，用于获取模型性能的平均值
DEFINE_int32(num_run, 10, "The number of runs to measure the model's performance.");

// 是否对操作进行性能分析，如果设置为true，将输出操作级别的性能数据
DEFINE_bool(enable_profiling, false, "Flag to enable profiling of individual operations within the model.");

static void dump_tensor_attr(rknn_tensor_attr *attr, bool is_input = true)
{
    std::string tensor_type = is_input ? "input tensor" : "output tensor";
    LOG(INFO) << tensor_type << ", " << "index=" << attr->index << ", name=" << attr->name << ", n_dims=" << attr->n_dims
              << ", dims=[" << attr->dims[0] << ", " << attr->dims[1] << ", " << attr->dims[2] << ", " << attr->dims[3] << "], fmt=" << get_format_string(attr->fmt) << ", n_elems=" << attr->n_elems << ", size=" << attr->size << ", type=" << get_type_string(attr->type) << ", qnt_type=" << get_qnt_type_string(attr->qnt_type) << ", zp=" << attr->zp << ", scale=" << attr->scale;
}


int batch_benchmark(const char *model, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, LatencyPerfData>> &batch_perf_results);

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
            batch_benchmark(file.string().c_str(), num_warmup, num_run, enable_profiling, batch_perf_results);
        }

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
    else
    {
        // 修改单个模型文件测试逻辑
        batch_benchmark(model_path.c_str(), num_warmup, num_run, enable_profiling, batch_perf_results);
        
        // 创建结果表格
        tabulate::Table profileTable;
        profileTable.add_row({"index", "model", "avg", "std", "min", "max"});
        
        // 添加结果到表格
        const auto &perf_result = batch_perf_results[0];
        profileTable.add_row({"0",
                            std::get<0>(perf_result),
                            std::to_string(std::get<1>(perf_result).mean),
                            std::to_string(std::get<1>(perf_result).stdev),
                            std::to_string(std::get<1>(perf_result).max),
                            std::to_string(std::get<1>(perf_result).min)});
        
        // 设置表格样式
        for (size_t i = 0; i < 5; ++i) {
            profileTable[0][i].format()
                .font_color(tabulate::Color::yellow)
                .font_align(tabulate::FontAlign::center)
                .font_style({tabulate::FontStyle::bold});
        }
        
        LOG(INFO) << "\n" << profileTable << "\n";
    }

    google::ShutdownGoogleLogging();
    return 0;
}


int batch_benchmark(const char *model, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, LatencyPerfData>> &batch_perf_results)
{
    LOG(INFO) << "Profiling model:" << model;
    rknn_context ctx;
    int flag = RKNN_FLAG_COLLECT_PERF_MASK;
    int ret = rknn_init(&ctx, (void *)model, 0, flag, nullptr);

    if (ret < 0)
    {
        LOG(ERROR) << "rknn_init fail! ret=" << ret << "\n";
        return -1;
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
    ret = rknn_query(ctx, RKNN_QUERY_PERF_DETAIL, &perf_detail, sizeof(perf_detail));
    if (enable_profiling)
    {
        LOG(INFO) << "Model OPs Performace Data:\n"
                  << perf_detail.perf_data << "\n";
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
    rknn_destroy(ctx);
    LOG(INFO) << "Profiling model:" << model << " done!";
    return 0;
}