#ifndef TIMER_HPP
#define TIMER_HPP
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <functional>
#include "glog/logging.h"
using namespace std;
using namespace std::chrono;

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

    void report()
    {
        LOG(INFO) << "Warmup Statistics: " << "\n";
        // cout << "Warmup Statistics:" << endl;
        report_statistics(durations_warmup_);
        LOG(INFO) << "Normal Statistics: " << "\n";
        // cout << "Normal Statistics:" << endl;
        report_statistics(durations_normal_);
    }

private:
    int warmup_iters_;
    int normal_iters_;
    function<void()> func_;
    vector<long long> durations_warmup_;
    vector<long long> durations_normal_;

    void report_statistics(const vector<long long> &durations)
    {
        if (durations.empty())
            return;

        double sum = accumulate(durations.begin(), durations.end(), 0.0);
        double mean = sum / durations.size();

        auto minmax = minmax_element(durations.begin(), durations.end());
        long long min_val = *minmax.first;
        long long max_val = *minmax.second;

        double sq_sum = inner_product(durations.begin(), durations.end(), durations.begin(), 0.0);
        double stdev = sqrt(sq_sum / durations.size() - mean * mean);

        cout << "Count:" << durations.size() << ", avg: " << mean << " us, std: " << stdev << " us, " << ", min: " << min_val << " us, " << "max: " << max_val << " us" << endl;

        // cout << "Count: " << durations.size() << endl;
    }
};

#endif