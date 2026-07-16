#pragma once

#include <chrono>

#include "metrics/symbol_store.hpp"
#include "metrics/types.hpp"

namespace metrics {

    class MetricEngine {
    public:
        explicit MetricEngine(int64_t window_ms = 60'000) : store_(window_ms) {}

        void Push(const Tick& tick) {
            store_.Push(tick.symbol, tick.timestamp * 1000, tick.price, tick.volume);
        }

        Metrics Query(const std::string& symbol) {
            return store_.Query(symbol, NowMillis());
        }

        std::vector<Metrics> SnapshotAll() {
            return store_.SnapshotAll(NowMillis());
        }

    private:
        static int64_t NowMillis() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                .count();
        }

        SymbolStore store_;
    };

}