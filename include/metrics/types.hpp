#pragma once

#include <cstdint>
#include <string>

namespace metrics {

    struct Tick {
        int64_t timestamp;
        std::string symbol;
        double price;
        double volume;
    };

    struct Metrics {
        std::string symbol;
        double avg_price = 0.0;
        double sum_volume = 0.0;
        double max_price = 0.0;
        double min_price = 0.0;
        uint64_t sample_count = 0;
        bool has_data = false;
    };

}