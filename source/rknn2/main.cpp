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

static void dump_tensor_attr(rknn_tensor_attr *attr, bool is_input = true)
{
    std::string tensor_type = is_input ? "input tensor" : "output tensor";
    LOG(INFO) << tensor_type << ", " << "index=" << attr->index << ", name=" << attr->name << ", n_dims=" << attr->n_dims
              << ", dims=[" << attr->dims[0] << ", " << attr->dims[1] << ", " << attr->dims[2] << ", " << attr->dims[3] << "], fmt=" << get_format_string(attr->fmt) << ", n_elems=" << attr->n_elems << ", size=" << attr->size << ", type=" << get_type_string(attr->type) << ", qnt_type=" << get_qnt_type_string(attr->qnt_type) << ", zp=" << attr->zp << ", scale=" << attr->scale;
}

int benchmark(const char *model, int num_warmup, int num_run, bool enable_profiling);

int batch_benchmark(const char *model, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, std::tuple<double, double, double, double>>> &batch_perf_results);

int main(int argc, char *argv[])
{
    /*
    ./rknn2_test --model=../saves/MobileNetV2-opset16.rknn
    */
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
    if (enable_batch_benchmark == false)
    {
        benchmark(model.c_str(), num_warmup, num_run, enable_profiling);
    }
    else
    {
        std::string directoryPath = model;

        // 存储找到的文件名
        std::vector<std::filesystem::path> rknnFiles;

        // 遍历目录
        for (const auto &entry : std::filesystem::recursive_directory_iterator(directoryPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".rknn")
            {
                rknnFiles.push_back(entry.path());
            }
        }
        std::vector<std::tuple<std::string, std::tuple<double, double, double, double>>> batch_perf_results;
        // 打印所有找到的文件
        for (const auto &file : rknnFiles)
        {
            // std::cout << file << std::endl;
            batch_benchmark(file.string().c_str(), num_warmup, num_run, enable_profiling, batch_perf_results);
        }
        std::ostringstream pt_oss;
        // oss << "Number: " << number << ", Value: " << value << ", Text: " << text;
        pt_oss << "\nmodel\t avg\t std\t min\t max\n";
        for (const auto &perf_result : batch_perf_results)
        {
            pt_oss << std::get<0>(perf_result) << "\t" << std::to_string(std::get<0>(std::get<1>(perf_result))) << "\t" << std::to_string(std::get<1>(std::get<1>(perf_result))) << "\t" << std::to_string(std::get<2>(std::get<1>(perf_result))) << "\t" << std::to_string(std::get<3>(std::get<1>(perf_result))) << "\n";
        }
        LOG(INFO) << pt_oss.str();
    }
    google::ShutdownGoogleLogging();
    return 0;
}

int benchmark(const char *model, int num_warmup, int num_run, bool enable_profiling)
{
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

    // Run
    // printf("rknn_run\n");

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
    return 0;
}

int batch_benchmark(const char *model, int num_warmup, int num_run, bool enable_profiling, std::vector<std::tuple<std::string, std::tuple<double, double, double, double>>> &batch_perf_results)
{
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
    // Run
    // printf("rknn_run\n");

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
    return 0;
}