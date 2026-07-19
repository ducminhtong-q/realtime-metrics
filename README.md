# Real-time Streaming Metrics Engine

TCP server nhận luồng tick data (price/volume) theo real-time, tính các chỉ số thống kê (avg, sum volume, max, min) trên sliding window 1 phút cho mỗi symbol, hỗ trợ query với latency < 5ms.

## Features

- Nhận tick data từ nhiều client đồng thời qua TCP (NDJSON protocol)
- Tính metrics real-time trên sliding window 1 phút: average price, total volume, max/min price
- Query latency < 5ms
- Thread-safe, hỗ trợ nhiều client đọc/ghi đồng thời
- Snapshot ra file mỗi 60 giây

## Kỹ thuật & thuật toán sử dụng

- **Sliding window bằng monotonic deque** — max/min trong cửa sổ trượt tính O(1) amortized (kỹ thuật "Sliding Window Maximum").
- **Thread-per-connection** cho networking, đồng bộ hóa qua 1 `std::mutex` duy nhất bảo vệ toàn bộ symbol store.
- **NDJSON qua TCP thô** (không dùng HTTP).
- **RAII** (`std::lock_guard`, `std::ofstream`) — không quản lý tài nguyên thủ công.
- **Graceful shutdown** qua signal handler (SIGINT/SIGTERM) + `std::atomic<bool>` — server dừng, cần thiết để chạy qua `valgrind`

**Nếu có thêm thời gian để nghiên cứu/nâng cấp, hướng đi tiếp theo:**
- Chuyển từ thread-per-connection sang **epoll** (event-driven I/O) nếu cần hàng nghìn connection đồng thời
- **Sharded locking theo symbol** (mutex riêng mỗi symbol + `shared_mutex` cho cấu trúc map) thay vì 1 mutex toàn cục
- Đo và tối ưu **memory usage** định lượng — hiện mới tối ưu ở mức thuật toán, chưa có số liệu cụ thể.
- Refactor persistence sang **interface** nếu cần hỗ trợ database, cloud storage...

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
```
-> {"timestamp": 1689000000, "symbol": "BTCUSD", "price": 30123.45, "volume": 0.5}
<- {"status": "ok"}
```

**Query metrics:**
```
-> {"action": "query", "symbol": "BTCUSD"}
<- {"symbol": "BTCUSD", "has_data": true, "avg_price": 30150.2, "sum_volume": 12.5, "max_price": 30200.0, "min_price": 30100.0, "sample_count": 42}
```

## Testing

### 1. Nhận luồng dữ liệu, nhiều client đồng thời

```bash
./metrics_server 9000 &

echo '{"action":"query","symbol":"BTCUSD"}' | nc -q 1 localhost 9000 &
echo '{"action":"query","symbol":"BTCUSD"}' | nc -q 1 localhost 9000 &
echo '{"action":"query","symbol":"BTCUSD"}' | nc -q 1 localhost 9000 &
wait
```

### 2. Metrics real-time (avg, sum volume, max, min)

```bash
./test_sliding_window
```

Test thủ công qua server thật:
```bash
./metrics_server 9000 &
NOW=$(date +%s)
echo "{\"timestamp\":$NOW,\"symbol\":\"BTCUSD\",\"price\":30000,\"volume\":1.0}" | nc -q 1 localhost 9000
echo "{\"timestamp\":$NOW,\"symbol\":\"BTCUSD\",\"price\":30200,\"volume\":0.5}" | nc -q 1 localhost 9000
echo '{"action":"query","symbol":"BTCUSD"}' | nc -q 1 localhost 9000
```

### 3. Query bất kỳ lúc nào, latency < 5ms

Latency không qua network:
```bash
./benchmark_latency
```

Latency qua socket thật:
```bash
./metrics_server 9000 &
./load_test_client 127.0.0.1 9000 50 1000
```

### 4. Concurrency thread-safe

Unit test:
```bash
./test_metric_engine
```

Load test:
```bash
./metrics_server 9000 &
./load_test_client 127.0.0.1 9000 50 1000
```

**CPU/RAM dưới tải**:
```bash
mkdir -p ../build_valgrind && cd ../build_valgrind
cmake -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Debug .. && make -j$(nproc)
 
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=definite,indirect \
    ./metrics_server 9000 &
VPID=$!
sleep 2 && ./load_test_client 127.0.0.1 9000 5 100
kill -INT $VPID
 
/usr/bin/time -v ./metrics_server 9000 &
sleep 1 && ./load_test_client 127.0.0.1 9000 50 3000 && kill -INT $(pgrep -x metrics_server)
```

## Project Structure

```
include/metrics/   # public headers (types, sliding_window, symbol_store, metric_engine, protocol, persistence, tcp_server)
src/                # tcp_server.cpp, main.cpp
tests/              # unit test + benchmark
tools/              # load test client
third_party/        # nlohmann/json (vendored, MIT license)
```