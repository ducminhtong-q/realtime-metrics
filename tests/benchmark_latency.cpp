#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include "metrics/metric_engine.hpp"

using namespace std::chrono;

int main() {
    metrics::MetricEngine engine(60'000);

    const int64_t now_sec = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

    const int kWarmupTicks = 1000;
    for (int i = 0; i < kWarmupTicks; ++i) {
        engine.Push({.timestamp = now_sec, .symbol = "BTCUSD", .price = 30000.0 + i, .volume = 1.0});
    }

    const int kIterations = 100'000;
    std::vector<double> latencies_us;
    latencies_us.reserve(kIterations);

    for (int i = 0; i < kIterations; ++i) {
        auto start = high_resolution_clock::now();
        auto result = engine.Query("BTCUSD");
        auto end = high_resolution_clock::now();

        volatile bool dummy = result.has_data;
        (void)dummy;

        double us = duration_cast<duration<double, std::micro>>(end - start).count();
        latencies_us.push_back(us);
    }


    std::sort(latencies_us.begin(), latencies_us.end());
    auto percentile = [&](double p) {
        size_t idx = static_cast<size_t>(p / 100.0 * static_cast<double>(latencies_us.size()));
        idx = std::min(idx, latencies_us.size() - 1);
        return latencies_us[idx];
    };

    double sum = 0.0;
    for (double v : latencies_us) sum += v;
    double avg = sum / static_cast<double>(latencies_us.size());

    std::cout << "=== Query() Latency Benchmark (" << kIterations << " iterations) ===\n";
    std::cout << "avg: " << avg << " us\n";
    std::cout << "p50: " << percentile(50) << " us\n";
    std::cout << "p95: " << percentile(95) << " us\n";
    std::cout << "p99: " << percentile(99) << " us\n";
    std::cout << "max: " << latencies_us.back() << " us\n";

    return 0;
}