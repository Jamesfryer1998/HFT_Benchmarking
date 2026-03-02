# HFT Order Book Matching Engine

A high-performance limit order book matching engine built in C++23 for learning low-latency systems and HFT concepts. Features nanosecond latency tracking, swappable data structures, and comprehensive benchmarking tools.

## Features

- **Three Order Book Implementations** for performance comparison:
  - `std::map` based (baseline, O(log n) operations)
  - `std::unordered_map` based (average O(1) operations)
  - Pre-allocated array based (cache-friendly, best performance)

- **Latency Measurement**:
  - Chrono high-resolution clock timing
  - RDTSC (CPU timestamp counter) support on x86/x64
  - Percentile tracking (P50, P95, P99)

- **Price-Time Priority Matching**:
  - FIFO queue at each price level
  - Proper matching engine semantics

- **Memory Management**:
  - Pre-allocated memory pool (no malloc/free in hot path)
  - Cache-friendly Order struct (64 bytes, cache-aligned)

- **Market Feed Simulator**:
  - Burst mode (maximum throughput testing)
  - Timed mode (configurable order rate)
  - Realistic order generation

## Project Structure

```
hft-engine/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp              # CLI entry point
│   ├── order.hpp             # Order struct (64 bytes, cache-aligned)
│   ├── order_book.hpp        # Order book interface + 3 implementations
│   ├── order_book.cpp        # Matching logic
│   ├── matching_engine.hpp   # Engine wrapper
│   ├── matching_engine.cpp
│   ├── feed_simulator.hpp    # Market feed generator
│   ├── feed_simulator.cpp
│   ├── memory_pool.hpp       # Pre-allocated memory pool
│   └── latency_tracker.hpp   # Latency measurement utilities
├── benchmarks/
│   ├── bench_order_book.cpp  # Google Benchmark suite
│   └── bench_latency.cpp     # Throughput tests
└── tests/
    └── test_matching.cpp     # Correctness tests
```

## Building

### Requirements

- C++23 compatible compiler (GCC 11+, Clang 14+, Apple Clang 14+)
- CMake 3.20+
- Internet connection (for fetching Google Benchmark)

### Build Commands

```bash
cd hft-engine
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Build Targets

- `hft_engine` - Main CLI executable
- `hft_benchmarks` - Google Benchmark suite
- `hft_tests` - Correctness tests

## Usage

### CLI Modes

```bash
# Compare all three data structure implementations (default)
./build/hft_engine --compare

# Run latency benchmarks
./build/hft_engine --benchmark

# Run feed simulator
./build/hft_engine --simulate

# Interactive REPL mode
./build/hft_engine --interactive

# Show help
./build/hft_engine --help
```

### Running Tests

```bash
./build/hft_tests
```

### Running Google Benchmarks

```bash
./build/hft_benchmarks
```

## Performance Comparison

The `--compare` mode runs all three implementations and produces a side-by-side comparison:

```
┌─────────────────────┬──────────────┬──────────────┬──────────────┐
│ Operation           │ std::map     │ unordered    │ Array Pool   │
├─────────────────────┼──────────────┼──────────────┼──────────────┤
│ Add Order (avg)     │ ~200ns       │ ~80ns        │ ~15ns        │
│ Match Order (avg)   │ ~180ns       │ ~70ns        │ ~10ns        │
│ Cancel Order (avg)  │ ~190ns       │ ~75ns        │ ~12ns        │
│ Throughput          │ ~1.2M/s      │ ~4.5M/s      │ ~15M/s       │
│ p99 Latency         │ ~800ns       │ ~250ns       │ ~35ns        │
└─────────────────────┴──────────────┴──────────────┴──────────────┘
```

**Note**: Actual numbers will vary based on your CPU and system configuration.

## Interactive Mode

The interactive mode provides a REPL for manual testing:

```
> add buy 100.00 500
Order 1 added

> add sell 101.00 300
Order 2 added

> book
=== Order Book ===

Asks (descending):
  $ 101.00  |      300
  -------------------------------

Bids (descending):
  $ 100.00  |      500

> stats
=== Engine Statistics ===
Total Orders Added:    2
...

> quit
```

## Key Design Decisions

### Cache-Friendly Design

- **Order struct**: 64 bytes, cache-aligned
- **Array-based book**: Pre-allocated contiguous memory
- **Memory pool**: Eliminates heap fragmentation

### Price Representation

- Prices stored as `int64_t` in ticks (1 tick = $0.01)
- Example: 10000 ticks = $100.00
- Array implementation supports $0 - $10,000 range

### No Exceptions in Hot Path

- All critical operations use error codes or `std::optional`
- `[[nodiscard]]` attributes ensure error handling

### Single-Threaded Design

- Simpler, no locks needed
- Matches real HFT design (one thread per symbol)

## Learning Outcomes

This project demonstrates key HFT concepts:

1. **Cache-line awareness** - Order struct fits in single cache line
2. **Memory pre-allocation** - Avoid malloc/free in hot path
3. **Data structure trade-offs** - Map vs Hash vs Array performance
4. **Latency measurement** - Nanosecond precision timing
5. **Price-time priority** - Proper matching semantics
6. **Hot path optimization** - Inlining, no exceptions, no allocations

## Extending the Project

### TODO: Stretch Features

- [ ] Lock-free SPSC ring buffer for feed → engine communication
- [ ] CPU core pinning (Linux: `pthread_setaffinity_np`)
- [ ] Order book depth visualization (top 5 levels)
- [ ] Multi-symbol support
- [ ] Market order support (currently limit orders only)
- [ ] Iceberg order support
- [ ] IOC (Immediate-or-Cancel) and FOK (Fill-or-Kill) order types

## Benchmarking Tips

1. **CPU Frequency Scaling**: Disable for consistent results
   ```bash
   # macOS: Use built-in energy saver settings
   # Linux: sudo cpupower frequency-set --governor performance
   ```

2. **Background Processes**: Close unnecessary applications

3. **Multiple Runs**: Run benchmarks multiple times and take the median

4. **Compiler Flags**: Already set to `-O3 -march=native`

## Technical Notes

### RDTSC Support

RDTSC (Read Time-Stamp Counter) is available on x86/x64 architectures only. The code automatically detects support at compile time.

### Tick Size

The array-based implementation uses a fixed tick size of $0.01. For different asset classes (e.g., crypto with fractional cents), adjust `ArrayOrderBook::TICK_SIZE`.

### Memory Usage

Array-based order book pre-allocates ~76MB for 1M price levels. Adjust `ArrayOrderBook::MAX_PRICE` if needed.

## License

This is an educational project. Use freely for learning purposes.

## Contributing

This is a personal learning project, but suggestions and improvements are welcome via issues or pull requests.

## Acknowledgments

- Google Benchmark for benchmarking framework
- Modern C++ best practices from various HFT engineering blogs
- Inspired by real-world matching engine designs

## Performance Notes

Results will vary significantly based on: 

- CPU architecture (cache sizes, frequency)
- Compiler and optimization flags
- System load and background processes
- Memory speed and configuration

Always benchmark on your target hardware for production use cases.


# TODO:
- Add my own custom Orderbook type to try be faster than array
- Compare this with real world examples
