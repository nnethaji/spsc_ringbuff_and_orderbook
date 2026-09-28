Implementation of a UDP byte stream parser -> updates SPSC ring buffer (lockfree version and locked: mutex and cv) 

macOS does not allow pinning threads to cores. The p99 and p99.9 tails therefore include scheduler preemption, interrupts and latency due to swithcing between cores. On Linux with isolated, pinned cores the tail would be expected to shrink. 

# Low-Latency Market Feed Handler (C++23)

A producer thread decodes fixed-size binary messages from a feed file and hands them to a consumer thread through a lock-free single-producer/single-consumer (SPSC) ring buffer. The project measures per-message handoff latency as p50/p99/p99.9.


## Message format

Each message is 32 bytes with fixed-width fields in big-endian. The parser reads into the struct and converts each field with `std::byteswap` (to little endian).  `static_assert(sizeof(msg) == 32)` to check if msg is 32 bytes

| offset | field          | type       |
|-------:|----------------|------------|
| 0      | `sequence_no`  | `uint64_t` |
| 8      | `timestamp_ns` | `uint64_t` |
| 16     | `price`        | `int64_t`  |
| 24     | `quantity`     | `uint32_t` |
| 28     | `symbol_id`    | `uint16_t` |
| 30     | `buy_sell`     | `char`     |
| 31     | `msg_type`     | `uint8_t`  |

## SPSC ring buffer (`spsc_lockfree.hpp`)

- **Power-of-two capacity**, checked with a `static_assert`. The slot index is `i & (N-1)`, so there's no modulo.
- **Monotonic `head`/`tail` counters.** Full is `head - tail == N` and empty is `head == tail` with unsigned wraparound 
- This is done so that we don't need to use modulo unnecessarily 

- **Memory ordering:**
  - Each side loads its own index `relaxed`, because it's the only writer. (P load head as relaxed)
  - Each side loads the *other* index with `acquire`. (C loads tail as relaxed)
  - Each side writes then stores with a `release` 
  - So a slot's contents are visible before the index that signals it.
- **Wait-free `push`/`pop`.** Each index has exactly one writer, so no CAS is needed. Every call finishes in a bounded number of steps and returns `false` on full or empty. The caller spins.
- **False-sharing.** `tail` (written by the consumer) and `head` plus the shutdown flag (written by the producer) sit on separate cache lines via `alignas(std::hardware_destructive_interference_size)`. 
- **Shutdown.** The producer sets a `release` flag after its last push. `done()` is true only when the flag is set **and** the queue is drained, so no message is lost at the end.

Correctness was checked with `-fsanitize=thread`: all 10,000 messages arrive, and reports no races.

## Latency measurement

The producer stamps each message with `std::chrono::steady_clock` just before pushing. The consumer takes the difference on a successful pop. The samples are sorted and read at integer percentile indices (`n/2`, `n*99/100`, `n*999/1000`).


1. The TSan and `-O0` builds reported p50 of about 840 µs and 283 µs. That was stack call overhead and queueing delay, not queue latency.
2. At `-O0` the consumer was slower than the producer, so the queue stayed full. Every message waited behind about 1,024 others, and the "latency" was capacity × consumer time per message. At `-O2` the consumer outpaces the producer, and the measurement becomes true handoff latency.
3. The producer waits on an atomic flag until the consumer thread is running. Without it, the first messages queued during thread startup, which inflated p99 from about 4 µs to about 18 µs.

## Results

These are handoff latencies: 10,000 messages, capacity 1,024, `clang++ -O2`, Apple M4 (MacBook Air), macOS. Each figure is the **median of 20 runs**.

| p50     | p99    | p99.9  |
|--------:|-------:|-------:|
| ~167 ns | ~7.1 µs | ~8.1 µs |



## Build and run

```bash
# to test for race conditions
clang++ -std=c++23 -g -fsanitize=thread main.cpp generator.cpp -o main_tsan && ./main_tsan

# measuring time taken for one message from producer to consumer
clang++ -std=c++23 -O2 main.cpp generator.cpp -o main && ./main
```




