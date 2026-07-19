#include "metrics/tcp_server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <iostream>
#include <string>
#include <thread>

#include "metrics/protocol.hpp"
#include "third_party/json.hpp"

namespace metrics {

    namespace {
        std::atomic<bool> g_shutdown_requested{false};

        void HandleClient(int client_fd, MetricEngine& engine) {
            std::string buffer;
            char chunk[4096];

            while (true) {
                ssize_t n = recv(client_fd, chunk, sizeof(chunk), 0);
                if (n <= 0) {
                    break;
                }
                buffer.append(chunk, static_cast<size_t>(n));

                size_t newline_pos;
                while ((newline_pos = buffer.find('\n')) != std::string::npos) {
                    std::string line = buffer.substr(0, newline_pos);
                    buffer.erase(0, newline_pos + 1);
                    if (line.empty()) {
                        continue;
                    }

                    std::string response;
                    try {
                        nlohmann::json j = nlohmann::json::parse(line);
                        if (j.contains("action") && j["action"] == "query") {
                            std::string symbol = j.at("symbol").get<std::string>();
                            Metrics m = engine.Query(symbol);
                            response = SerializeMetrics(m);
                        } else {
                            auto tick = ParseTick(line);
                            if (tick.has_value()) {
                                engine.Push(*tick);
                                response = R"({"status":"ok"})";
                            } else {
                                response = R"({"status":"error","message":"invalid tick"})";
                            }
                        }
                    } catch (const nlohmann::json::exception&) {
                        response = R"({"status":"error","message":"invalid json"})";
                    }

                    response += "\n";
                    send(client_fd, response.data(), response.size(), 0);
                }
            }
            close(client_fd);
        }

    }

    void RunServer(int port, MetricEngine& engine) {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "Failed to create socket\n";
            return;
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(static_cast<uint16_t>(port));

        if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
            std::cerr << "Failed to bind to port " << port << "\n";
            close(server_fd);
            return;
        }

        if (listen(server_fd, 128) < 0) {
            std::cerr << "Failed to listen\n";
            close(server_fd);
            return;
        }

        std::cout << "Server listening on port " << port << "\n";

        while (!g_shutdown_requested.load()) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            if (client_fd < 0) {
                if (errno == EINTR) {
                    break;
                }
                continue;
            }
            std::thread(HandleClient, client_fd, std::ref(engine)).detach();
        }

        close(server_fd);
        std::cout << "Server shut down cleanly\n";
    }
    void RequestShutdown() {
        g_shutdown_requested.store(true);
    }

}