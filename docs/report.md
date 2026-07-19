# Resource Usage Report — CPU & RAM

Đo bằng các công cụ: **Valgrind** (`memcheck` cho leak, `massif` cho heap theo thời gian) và **`/usr/bin/time -v`** cho CPU/RAM tổng quan.


## 1. Leak check — `memcheck` ([raw log](memcheck_report.txt))

```
definitely lost: 0 bytes in 0 blocks
indirectly lost: 0 bytes in 0 blocks
possibly lost:   304 bytes in 1 blocks
still reachable: 16 bytes in 1 blocks
```

**Kết luận: không bị leak** (`definitely lost = 0`).

## 2. Heap theo thời gian — `massif` ([raw log](massif_report.txt))

Graph tự sinh bởi `ms_print`
```
Peak: ~121KB.
Phân bổ:
- 59.5%: Khởi tạo runtime C++
- 30.1%: SlidingWindow::Push
- 2.5%: Stack cấp phát cho mỗi thread mới (thread-per-connection)
```

## 3. CPU & RAM tổng quan — `/usr/bin/time -v` ([raw log](time_report2.txt))

Đo dưới tải 50 client × 3000 request (150.000 request):
```
Percent of CPU this job got: 396%
Maximum resident set size: 16,480 KB (~16MB)
Elapsed: 1.62s   User: 5.72s   System: 0.71s
```