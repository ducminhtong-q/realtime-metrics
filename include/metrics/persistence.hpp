#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "metrics/types.hpp"
#include "third_party/json.hpp"

namespace metrics {

    inline void AppendSnapshotToFile(const std::string& filepath, const std::vector<Metrics>& all_metrics,
                                      int64_t snapshot_timestamp_ms) {
        std::ofstream file(filepath, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "[persistence] FAILED to open " << filepath << "\n";
            return;
        }

        for (const Metrics& m : all_metrics) {
            nlohmann::json j;
            j["snapshot_ts_ms"] = snapshot_timestamp_ms;
            j["symbol"] = m.symbol;
            j["avg_price"] = m.avg_price;
            j["sum_volume"] = m.sum_volume;
            j["max_price"] = m.max_price;
            j["min_price"] = m.min_price;
            j["sample_count"] = m.sample_count;
            file << j.dump() << "\n";
        }
        std::cerr << "[persistence] snapshot written to: " << std::filesystem::absolute(filepath) << "\n";
    }

}