#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "metrics/sliding_window.hpp"
#include "metrics/types.hpp"

namespace metrics {

class SymbolStore {
public:
    explicit SymbolStore(int64_t window_ms = 60'000) : window_ms_(window_ms) {}

    void Push(const std::string& symbol, int64_t timestamp_ms, double price, double volume) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = windows_.find(symbol);
        if (it == windows_.end()) {
            it = windows_.emplace(symbol, SlidingWindow(window_ms_)).first;
        }
        it->second.Push({.timestamp_ms = timestamp_ms, .price = price, .volume = volume});
    }

    Metrics Query(const std::string& symbol, int64_t now_ms) {
        std::lock_guard<std::mutex> lock(mutex_);

        Metrics result;
        result.symbol = symbol;

        auto it = windows_.find(symbol);
        if (it == windows_.end()) {
            result.has_data = false;
            return result;
        }

        WindowMetrics wm = it->second.Query(now_ms);
        result.has_data = wm.has_data;
        result.avg_price = wm.avg_price;
        result.sum_volume = wm.sum_volume;
        result.max_price = wm.max_price;
        result.min_price = wm.min_price;
        result.sample_count = wm.sample_count;
        return result;
    }

private:
    int64_t window_ms_;
    std::mutex mutex_;
    std::unordered_map<std::string, SlidingWindow> windows_;
};

}