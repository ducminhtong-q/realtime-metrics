#include "metrics/sliding_window.hpp"

#include <gtest/gtest.h>

using metrics::DataPoint;
using metrics::SlidingWindow;

namespace {
    constexpr int64_t kMinute = 60'000;
}

TEST(SlidingWindow, EmptyWindowHasNoData) {
    SlidingWindow w(kMinute);
    auto m = w.Query(0);
    EXPECT_FALSE(m.has_data);
    EXPECT_EQ(m.sample_count, 0u);
}

TEST(SlidingWindow, SinglePointBasicMetrics) {
    SlidingWindow w(kMinute);
    w.Push({.timestamp_ms = 1000, .price = 100.0, .volume = 2.0});

    auto m = w.Query(1000);
    EXPECT_TRUE(m.has_data);
    EXPECT_DOUBLE_EQ(m.avg_price, 100.0);
    EXPECT_DOUBLE_EQ(m.sum_volume, 2.0);
    EXPECT_DOUBLE_EQ(m.max_price, 100.0);
    EXPECT_DOUBLE_EQ(m.min_price, 100.0);
    EXPECT_EQ(m.sample_count, 1u);
}

TEST(SlidingWindow, MultiplePointsWithinWindow) {
    SlidingWindow w(kMinute);
    w.Push({.timestamp_ms = 0, .price = 100.0, .volume = 1.0});
    w.Push({.timestamp_ms = 10'000, .price = 110.0, .volume = 2.0});
    w.Push({.timestamp_ms = 20'000, .price = 90.0, .volume = 3.0});

    auto m = w.Query(20'000);
    EXPECT_TRUE(m.has_data);
    EXPECT_EQ(m.sample_count, 3u);
    EXPECT_DOUBLE_EQ(m.sum_volume, 6.0);
    EXPECT_DOUBLE_EQ(m.max_price, 110.0);
    EXPECT_DOUBLE_EQ(m.min_price, 90.0);
    EXPECT_NEAR(m.avg_price, 100.0, 1e-9);
}

TEST(SlidingWindow, OldPointsExpireOutOfWindow) {
    SlidingWindow w(kMinute);
    w.Push({.timestamp_ms = 0, .price = 50.0, .volume = 1.0});
    w.Push({.timestamp_ms = 30'000, .price = 200.0, .volume = 1.0});

    auto m = w.Query(61'000);
    EXPECT_TRUE(m.has_data);
    EXPECT_EQ(m.sample_count, 1u);
    EXPECT_DOUBLE_EQ(m.max_price, 200.0);
    EXPECT_DOUBLE_EQ(m.min_price, 200.0);
}

TEST(SlidingWindow, ExpiresEvenWithoutNewPush) {
    SlidingWindow w(kMinute);
    w.Push({.timestamp_ms = 0, .price = 100.0, .volume = 1.0});

    auto m1 = w.Query(30'000);
    EXPECT_EQ(m1.sample_count, 1u);

    auto m2 = w.Query(61'000);
    EXPECT_FALSE(m2.has_data);
    EXPECT_EQ(m2.sample_count, 0u);
}

TEST(SlidingWindow, MaxMinCorrectAfterPartialExpiry) {
    SlidingWindow w(kMinute);
    w.Push({.timestamp_ms = 0, .price = 500.0, .volume = 1.0});
    w.Push({.timestamp_ms = 5'000, .price = 300.0, .volume = 1.0});
    w.Push({.timestamp_ms = 10'000, .price = 400.0, .volume = 1.0});

    auto m1 = w.Query(10'000);
    EXPECT_DOUBLE_EQ(m1.max_price, 500.0);

    auto m2 = w.Query(61'000);
    EXPECT_DOUBLE_EQ(m2.max_price, 400.0);
    EXPECT_DOUBLE_EQ(m2.min_price, 300.0);
}

TEST(SlidingWindow, DuplicatePricesHandledCorrectly) {
    SlidingWindow w(kMinute);
    w.Push({.timestamp_ms = 0, .price = 100.0, .volume = 1.0});
    w.Push({.timestamp_ms = 1000, .price = 100.0, .volume = 1.0});
    w.Push({.timestamp_ms = 2000, .price = 100.0, .volume = 1.0});

    auto m1 = w.Query(2000);
    EXPECT_DOUBLE_EQ(m1.max_price, 100.0);
    EXPECT_EQ(m1.sample_count, 3u);

    auto m2 = w.Query(60'001);
    EXPECT_EQ(m2.sample_count, 2u);
    EXPECT_DOUBLE_EQ(m2.max_price, 100.0);

    auto m3 = w.Query(63'000);
    EXPECT_FALSE(m3.has_data);
}