#ifndef TIMER_HPP
#define TIMER_HPP
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <functional>
#include <tuple>
#include "glog/logging.h"
using namespace std;
using namespace std::chrono;
typedef struct LatencyPerformanceData
{
    double mean;
    double stdev;
    double min;
    double max;
} LatencyPerfData;

class Timer
{
public:
    template <typename Func, typename... Args>
    Timer(int warmup_iters, int normal_iters, Func &&func, Args &&...args)
        : warmup_iters_(warmup_iters), normal_iters_(normal_iters)
    {
        func_ = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    }

    void run()
    {
        for (int i = 0; i < warmup_iters_; ++i)
        {
            auto start = high_resolution_clock::now();
            func_();
            auto end = high_resolution_clock::now();
            durations_warmup_.push_back(duration_cast<microseconds>(end - start).count());
        }

        for (int i = 0; i < normal_iters_; ++i)
        {
            auto start = high_resolution_clock::now();
            func_();
            auto end = high_resolution_clock::now();
            durations_normal_.push_back(duration_cast<microseconds>(end - start).count());
        }
    }

    std::tuple<LatencyPerfData, LatencyPerfData> report()
    {
        LOG(INFO) << "Warmup Statistics: " << "\n";
        // cout << "Warmup Statistics:" << endl;
        auto warmup_data = report_statistics(durations_warmup_);
        LOG(INFO) << "Normal Statistics: " << "\n";
        // cout << "Normal Statistics:" << endl;
        auto normal_data = report_statistics(durations_normal_);
        return std::make_tuple(warmup_data, normal_data);
    }

private:
    int warmup_iters_;
    int normal_iters_;
    function<void()> func_;
    vector<long long> durations_warmup_;
    vector<long long> durations_normal_;

    LatencyPerfData report_statistics(const vector<long long> &durations)
    {
        if (durations.empty())
            return {0, 0, 0, 0};

        double sum = accumulate(durations.begin(), durations.end(), 0.0);
        double mean = sum / durations.size();

        auto minmax = minmax_element(durations.begin(), durations.end());
        long long min_val = *minmax.first;
        long long max_val = *minmax.second;

        double sq_sum = inner_product(durations.begin(), durations.end(), durations.begin(), 0.0);
        double stdev = sqrt(sq_sum / durations.size() - mean * mean);

        cout << "Count:" << durations.size() << ", avg: " << mean << " us, std: " << stdev << " us, " << ", min: " << min_val << " us, " << "max: " << max_val << " us" << endl;
        return {
            mean,
            stdev,
            (double)min_val,
            (double)max_val};
        // return std::make_tuple(mean, stdev, (double)min_val, (double)max_val);
        // cout << "Count: " << durations.size() << endl;
    }
};

#endif