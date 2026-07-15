#include <iostream>

#include "metrics/metric_engine.hpp"
#include "metrics/tcp_server.hpp"

int main(int argc, char** argv) {
    int port = 9000;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    metrics::MetricEngine engine(60'000);
    metrics::RunServer(port, engine);

    return 0;
}