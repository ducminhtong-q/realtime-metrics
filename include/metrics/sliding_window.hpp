#pragma once

#include <cstdint>
#include <deque>

namespace metrics {

struct DataPoint {
    int64_t timestamp_ms;
    double price;
    double volume;
};

struct WindowMetrics {
    double avg_price = 0.0;
    double sum_volume = 0.0;
    double max_price = 0.0;
    double min_price = 0.0;
    uint64_t sample_count = 0;
    bool has_data = false;
};

class SlidingWindow {
public:
    explicit SlidingWindow(int64_t window_ms = 60'000) : window_ms_(window_ms) {}

    void Push(const DataPoint& dp) {
        main_.push_back(dp);

        while (!max_deque_.empty() && max_deque_.back()->price <= dp.price) {
            max_deque_.pop_back();
        }
        max_deque_.push_back(&main_.back());

        while (!min_deque_.empty() && min_deque_.back()->price >= dp.price) {
            min_deque_.pop_back();
        }
        min_deque_.push_back(&main_.back());

        sum_price_ += dp.price;
        sum_volume_ += dp.volume;

        Expire(dp.timestamp_ms);
    }

    WindowMetrics Query(int64_t now_ms) {
        Expire(now_ms);

        WindowMetrics result;
        result.sample_count = main_.size();
        if (main_.empty()) {
            result.has_data = false;
            return result;
        }

        result.has_data = true;
        result.avg_price = sum_price_ / static_cast<double>(main_.size());
        result.sum_volume = sum_volume_;
        result.max_price = max_deque_.front()->price;
        result.min_price = min_deque_.front()->price;
        return result;
    }

private:
    void Expire(int64_t now_ms) {
        const int64_t cutoff = now_ms - window_ms_;
        while (!main_.empty() && main_.front().timestamp_ms < cutoff) {
            if (!max_deque_.empty() && max_deque_.front() == &main_.front()) {
                max_deque_.pop_front();
            }
            if (!min_deque_.empty() && min_deque_.front() == &main_.front()) {
                min_deque_.pop_front();
            }
            sum_price_ -= main_.front().price;
            sum_volume_ -= main_.front().volume;
            main_.pop_front();
        }
    }

    int64_t window_ms_;
    std::deque<DataPoint> main_;
    std::deque<const DataPoint*> max_deque_;
    std::deque<const DataPoint*> min_deque_;
    double sum_price_ = 0.0;
    double sum_volume_ = 0.0;
};

}