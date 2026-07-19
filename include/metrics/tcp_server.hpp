#pragma once

#include "metrics/metric_engine.hpp"

namespace metrics {
    void RunServer(int port, MetricEngine& engine);
    void RequestShutdown();
}