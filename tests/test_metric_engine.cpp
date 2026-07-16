#include "metrics/metric_engine.hpp"

#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using metrics::Metrics;
using metrics::MetricEngine;
using metrics::Tick;

namespace {
    int64_t NowSeconds() {

        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
    }

}

TEST(MetricEngine, QueryUnknownSymbolReturnsNoData) {
    MetricEngine engine;
    Metrics m = engine.Query("UNKNOWN");
    EXPECT_FALSE(m.has_data);
}

TEST(MetricEngine, PushThenQuerySameSymbolReturnsCorrectMetrics) {
    MetricEngine engine;
    const int64_t now = NowSeconds();

    engine.Push(Tick{.timestamp = now, .symbol = "BTCUSD", .price = 30000.0, .volume = 1.0});
    engine.Push(Tick{.timestamp = now, .symbol = "BTCUSD", .price = 30200.0, .volume = 0.5});

    Metrics m = engine.Query("BTCUSD");
    EXPECT_TRUE(m.has_data);
    EXPECT_EQ(m.sample_count, 2u);
    EXPECT_DOUBLE_EQ(m.sum_volume, 1.5);
    EXPECT_DOUBLE_EQ(m.max_price, 30200.0);
    EXPECT_DOUBLE_EQ(m.min_price, 30000.0);
    EXPECT_NEAR(m.avg_price, 30100.0, 1e-6);
}

TEST(MetricEngine, DifferentSymbolsAreIndependent) {
    MetricEngine engine;
    const int64_t now = NowSeconds();

    engine.Push(Tick{.timestamp = now, .symbol = "BTCUSD", .price = 30000.0, .volume = 1.0});
    engine.Push(Tick{.timestamp = now, .symbol = "XAUUSD", .price = 2000.0, .volume = 3.0});

    Metrics btc = engine.Query("BTCUSD");
    Metrics xau = engine.Query("XAUUSD");

    EXPECT_DOUBLE_EQ(btc.max_price, 30000.0);
    EXPECT_DOUBLE_EQ(xau.max_price, 2000.0);
    EXPECT_EQ(btc.sample_count, 1u);
    EXPECT_EQ(xau.sample_count, 1u);
}

TEST(MetricEngine, ConcurrentPushFromMultipleThreadsIsSafe) {
    MetricEngine engine;
    const int64_t now = NowSeconds();
    constexpr int kThreads = 8;
    constexpr int kPushesPerThread = 500;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&engine, now]() {
            for (int i = 0; i < kPushesPerThread; ++i) {
                engine.Push(Tick{.timestamp = now, .symbol = "STRESS", .price = 100.0 + i, .volume = 1.0});
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    Metrics m = engine.Query("STRESS");
    EXPECT_TRUE(m.has_data);
    EXPECT_EQ(m.sample_count, static_cast<uint64_t>(kThreads * kPushesPerThread));
}

TEST(MetricEngine, SnapshotAllReturnsAllActiveSymbols) {
    MetricEngine engine;
    const int64_t now = NowSeconds();

    engine.Push(Tick{.timestamp = now, .symbol = "BTCUSD", .price = 30000.0, .volume = 1.0});
    engine.Push(Tick{.timestamp = now, .symbol = "XAUUSD", .price = 2000.0, .volume = 3.0});
    engine.Push(Tick{.timestamp = now, .symbol = "USOIL", .price = 70.0, .volume = 5.0});

    auto snapshot = engine.SnapshotAll();
    EXPECT_EQ(snapshot.size(), 3u);

    bool found_btc = false;
    for (const auto& m : snapshot) {
        if (m.symbol == "BTCUSD") {
            found_btc = true;
            EXPECT_DOUBLE_EQ(m.max_price, 30000.0);
        }
    }
    EXPECT_TRUE(found_btc);
}