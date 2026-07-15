#pragma once

#include <optional>
#include <string>

#include "metrics/types.hpp"
#include "third_party/json.hpp"

namespace metrics {

    inline std::optional<Tick> ParseTick(const std::string& json_line) {
        try {
            nlohmann::json j = nlohmann::json::parse(json_line);
            Tick tick;
            tick.timestamp = j.at("timestamp").get<int64_t>();
            tick.symbol = j.at("symbol").get<std::string>();
            tick.price = j.at("price").get<double>();
            tick.volume = j.at("volume").get<double>();
            return tick;
        } catch (const nlohmann::json::exception&) {
            return std::nullopt;
        }
    }

    inline std::string SerializeMetrics(const Metrics& m) {
        nlohmann::json j;
        j["symbol"] = m.symbol;
        j["has_data"] = m.has_data;
        if (m.has_data) {
            j["avg_price"] = m.avg_price;
            j["sum_volume"] = m.sum_volume;
            j["max_price"] = m.max_price;
            j["min_price"] = m.min_price;
            j["sample_count"] = m.sample_count;
        }
        return j.dump();
    }

}