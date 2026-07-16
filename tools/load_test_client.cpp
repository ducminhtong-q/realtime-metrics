#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono;

namespace {

    int Connect(const std::string& host, int port) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return -1;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            close(fd);
            return -1;
        }
        return fd;
    }

    std::string SendAndRecv(int fd, const std::string& line) {
        std::string msg = line + "\n";
        send(fd, msg.data(), msg.size(), 0);

        char buf[4096];
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) return "";
        return std::string(buf, static_cast<size_t>(n));
    }

    struct ClientStats {
        int push_count = 0;
        int query_count = 0;
        std::vector<double> query_latencies_us;
    };


    ClientStats RunClient(const std::string& host, int port, int requests_per_client, int client_id) {
        ClientStats stats;
        int fd = Connect(host, port);
        if (fd < 0) {
            std::cerr << "Client " << client_id << " failed to connect\n";
            return stats;
        }

        std::mt19937 rng(client_id);
        std::uniform_real_distribution<double> price_dist(29000.0, 31000.0);
        std::uniform_int_distribution<int> action_dist(0, 4);

        const int64_t now_sec = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();

        for (int i = 0; i < requests_per_client; ++i) {
            if (action_dist(rng) < 4) {
                std::ostringstream oss;
                oss << R"({"timestamp":)" << now_sec << R"(,"symbol":"BTCUSD","price":)" << price_dist(rng)
                    << R"(,"volume":1.0})";
                SendAndRecv(fd, oss.str());
                stats.push_count++;
            } else {
                auto start = high_resolution_clock::now();
                SendAndRecv(fd, R"({"action":"query","symbol":"BTCUSD"})");
                auto end = high_resolution_clock::now();
                stats.query_latencies_us.push_back(duration_cast<duration<double, std::micro>>(end - start).count());
                stats.query_count++;
            }
        }

        close(fd);
        return stats;
    }

    double Percentile(std::vector<double>& sorted_data, double p) {
        if (sorted_data.empty()) return 0.0;
        size_t idx = static_cast<size_t>(p / 100.0 * static_cast<double>(sorted_data.size()));
        idx = std::min(idx, sorted_data.size() - 1);
        return sorted_data[idx];
    }

}

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = 9000;
    int num_clients = 20;
    int requests_per_client = 500;

    if (argc > 1) host = argv[1];
    if (argc > 2) port = std::atoi(argv[2]);
    if (argc > 3) num_clients = std::atoi(argv[3]);
    if (argc > 4) requests_per_client = std::atoi(argv[4]);

    std::cout << "Load test: " << num_clients << " concurrent clients x " << requests_per_client
              << " requests each, target " << host << ":" << port << "\n";

    std::vector<std::thread> threads;
    std::mutex results_mutex;
    std::vector<ClientStats> all_stats;

    auto wall_start = high_resolution_clock::now();

    for (int c = 0; c < num_clients; ++c) {
        threads.emplace_back([&, c]() {
            ClientStats s = RunClient(host, port, requests_per_client, c);
            std::lock_guard<std::mutex> lock(results_mutex);
            all_stats.push_back(std::move(s));
        });
    }
    for (auto& t : threads) t.join();

    auto wall_end = high_resolution_clock::now();
    double wall_seconds = duration_cast<duration<double>>(wall_end - wall_start).count();

    int total_push = 0, total_query = 0;
    std::vector<double> all_query_latencies;
    for (auto& s : all_stats) {
        total_push += s.push_count;
        total_query += s.query_count;
        all_query_latencies.insert(all_query_latencies.end(), s.query_latencies_us.begin(),
                                    s.query_latencies_us.end());
    }
    std::sort(all_query_latencies.begin(), all_query_latencies.end());

    int total_requests = total_push + total_query;
    std::cout << "Tong request: " << total_requests << " (push=" << total_push << ", query=" << total_query
              << ")\n";
    std::cout << "Thoi gian chay: " << wall_seconds << "s\n";
    std::cout << "Throughput: " << (static_cast<double>(total_requests) / wall_seconds) << " req/s\n";
    std::cout << "\nQuery round-trip latency:\n";
    std::cout << "  p50: " << Percentile(all_query_latencies, 50) << " us\n";
    std::cout << "  p95: " << Percentile(all_query_latencies, 95) << " us\n";
    std::cout << "  p99: " << Percentile(all_query_latencies, 99) << " us\n";
    if (!all_query_latencies.empty()) {
        std::cout << "  max: " << all_query_latencies.back() << " us\n";
    }

    return 0;
}