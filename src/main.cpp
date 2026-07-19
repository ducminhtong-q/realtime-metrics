#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <thread>

#include "metrics/metric_engine.hpp"
#include "metrics/persistence.hpp"
#include "metrics/tcp_server.hpp"

namespace {
    void HandleSignal(int /*signum*/)
    {
        metrics::RequestShutdown();
    }
    void InstallSignalHandler()
    {
        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = HandleSignal;
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);
    }
}

int main(int argc, char** argv) {
    InstallSignalHandler();

    int port = 9000;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    metrics::MetricEngine engine(60'000);

    std::thread snapshot_thread([&engine]() {
        const std::string filepath = "snapshots.jsonl";
        while (true) {
            std::this_thread::sleep_for(std::chrono::minutes(1));
            auto all = engine.SnapshotAll();
            int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count();
            metrics::AppendSnapshotToFile(filepath, all, now_ms);
            std::cout << "[persistence] wrote snapshot for " << all.size() << " symbol(s)\n";
        }
    });
    snapshot_thread.detach();

    metrics::RunServer(port, engine);

    return 0;
}