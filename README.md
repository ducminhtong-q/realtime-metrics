# Real-time Streaming Metrics Engine

TCP server nhận luồng tick data (price/volume) theo real-time, tính các chỉ số thống kê (avg, sum volume, max, min) trên sliding window 1 phút cho mỗi symbol, hỗ trợ query với latency < 5ms.

## Features

- Nhận tick data từ nhiều client đồng thời qua TCP (NDJSON protocol)
- Tính metrics real-time trên sliding window 1 phút: average price, total volume, max/min price
- Query latency < 5ms
- Thread-safe, hỗ trợ nhiều client đọc/ghi đồng thời
- Persistence: snapshot ra file mỗi 60 giây

## Requirements

- Linux, CMake ≥ 3.16, compiler hỗ trợ C++20 (GCC ≥ 10)
- Kết nối mạng khi build lần đầu (tải GoogleTest qua FetchContent)

## Quick Start

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

./metrics_server 9000
```

Trong terminal khác:

```bash
echo '{"timestamp": '$(date +%s)', "symbol": "BTCUSD", "price": 30123.45, "volume": 0.5}' | nc localhost 9000
echo '{"action": "query", "symbol": "BTCUSD"}' | nc localhost 9000
```

## API

**Push tick:**
```json
-> {"timestamp": 1689000000, "symbol": "BTCUSD", "price": 30123.45, "volume": 0.5}
<- {"status": "ok"}
```

**Query metrics:**
```json
-> {"action": "query", "symbol": "BTCUSD"}
<- {"symbol": "BTCUSD", "has_data": true, "avg_price": 30150.2, "sum_volume": 12.5, "max_price": 30200.0, "min_price": 30100.0, "sample_count": 42}
```

## Testing

```bash
cd build
./test_sliding_window
./benchmark_latency
```

Load test (2 terminal):
```bash
./metrics_server 9000
./load_test_client 127.0.0.1 9000 <num_clients> <requests_per_client>
```

## Project Structure

```
include/metrics/    # public headers (types, sliding_window, symbol_store, metric_engine, protocol, persistence, tcp_server)
src/                # tcp_server.cpp, main.cpp
tests/              # unit test + benchmark
tools/              # load test client
third_party/        # nlohmann/json (vendored, MIT license)
```