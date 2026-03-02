#include "matching_engine.hpp"
#include "feed_simulator.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <map>

using namespace hft;

// ASCII table formatter
class TableFormatter {
public:
    void add_row(const std::vector<std::string>& row) {
        rows_.push_back(row);
        if (!row.empty() && row.size() > col_widths_.size()) {
            col_widths_.resize(row.size(), 0);
        }
        for (size_t i = 0; i < row.size(); ++i) {
            if (row[i].size() > col_widths_[i]) {
                col_widths_[i] = row[i].size();
            }
        }
    }

    void print() const {
        if (rows_.empty()) return;

        print_separator();
        print_row(rows_[0]); // Header
        print_separator();

        for (size_t i = 1; i < rows_.size(); ++i) {
            print_row(rows_[i]);
        }

        print_separator();
    }

private:
    void print_separator() const {
        std::cout << "+";
        for (size_t i = 0; i < col_widths_.size(); ++i) {
            std::cout << std::string(col_widths_[i] + 2, '-');
            if (i < col_widths_.size() - 1) {
                std::cout << "+";
            }
        }
        std::cout << "+\n";
    }

    void print_row(const std::vector<std::string>& row) const {
        std::cout << "|";
        for (size_t i = 0; i < col_widths_.size(); ++i) {
            std::string cell = i < row.size() ? row[i] : "";
            std::cout << " " << std::left << std::setw(col_widths_[i]) << cell << " |";
        }
        std::cout << "\n";
    }

    std::vector<std::vector<std::string>> rows_;
    std::vector<size_t> col_widths_;
};

// Format nanoseconds as string
std::string format_ns(uint64_t ns) {
    std::ostringstream oss;
    oss << ns << "ns";
    return oss.str();
}

// Format throughput as string
std::string format_throughput(double ops_per_sec) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);

    if (ops_per_sec >= 1'000'000) {
        oss << (ops_per_sec / 1'000'000) << "M/s";
    } else if (ops_per_sec >= 1'000) {
        oss << (ops_per_sec / 1'000) << "K/s";
    } else {
        oss << ops_per_sec << "/s";
    }

    return oss.str();
}

// Visualize order book (top N levels)
void visualize_order_book(const IOrderBook& book, size_t levels = 5) {
    auto bids = book.get_bids(levels);
    auto asks = book.get_asks(levels);

    std::cout << "\n=== Order Book ===\n";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "\nAsks (descending):\n";
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        std::cout << "  $" << std::setw(8) << (it->first / 100.0)
                  << "  |  " << std::setw(8) << it->second << "\n";
    }

    std::cout << "  " << std::string(30, '-') << "\n";

    std::cout << "\nBids (descending):\n";
    for (const auto& [price, qty] : bids) {
        std::cout << "  $" << std::setw(8) << (price / 100.0)
                  << "  |  " << std::setw(8) << qty << "\n";
    }

    std::cout << "\n";
}

// Run comparison benchmark
void run_comparison() {
    std::cout << "\n=== Running Performance Comparison ===\n";
    std::cout << "Testing all three order book implementations...\n\n";

    const size_t num_orders = 50'000;
    OrderBookType types[] = {OrderBookType::MAP, OrderBookType::HASH, OrderBookType::ARRAY};
    std::string names[] = {"std::map", "unordered_map", "Array Pool"};

    std::map<std::string, EngineStats> results;

    for (size_t i = 0; i < 3; ++i) {
        std::cout << "Benchmarking " << names[i] << "...\n";

        MatchingEngine engine(types[i]);
        SimulatorConfig config;
        config.num_orders = num_orders;
        config.mode = SimulationMode::BURST;
        config.quiet = true;

        FeedSimulator sim(config);
        sim.run(engine);

        // Benchmark cancellations: add orders then cancel them
        const size_t cancel_test_count = 5000;
        std::vector<uint64_t> order_ids;
        order_ids.reserve(cancel_test_count);

        // Add orders to cancel
        for (size_t j = 0; j < cancel_test_count; ++j) {
            uint64_t order_id = num_orders + j + 1;
            Order order(order_id, OrderSide::BUY, OrderType::LIMIT, 9000 + (j % 100), 100, Timer::now_ns());
            engine.order_book().add_order(order);
            order_ids.push_back(order_id);
        }

        // Cancel all orders
        for (uint64_t order_id : order_ids) {
            engine.cancel_order(order_id);
        }

        results[names[i]] = engine.stats();
    }

    // Print comparison table
    std::cout << "\n";
    TableFormatter table;

    table.add_row({"Operation", "std::map", "unordered_map", "Array Pool"});

    // Add order average
    {
        std::vector<std::string> row = {"Add Order (avg)"};
        for (const auto& name : names) {
            row.push_back(format_ns(results[name].add_latency.mean()));
        }
        table.add_row(row);
    }

    // Match order average
    {
        std::vector<std::string> row = {"Match Order (avg)"};
        for (const auto& name : names) {
            auto mean = results[name].match_latency.count() > 0
                ? results[name].match_latency.mean() : 0;
            row.push_back(format_ns(mean));
        }
        table.add_row(row);
    }

    // Cancel order average
    {
        std::vector<std::string> row = {"Cancel Order (avg)"};
        for (const auto& name : names) {
            auto mean = results[name].cancel_latency.count() > 0
                ? results[name].cancel_latency.mean() : 0;
            row.push_back(format_ns(mean));
        }
        table.add_row(row);
    }

    // Throughput
    {
        std::vector<std::string> row = {"Throughput"};
        for (const auto& name : names) {
            double avg_latency_ns = results[name].add_latency.mean();
            double ops_per_sec = avg_latency_ns > 0 ? (1e9 / avg_latency_ns) : 0;
            row.push_back(format_throughput(ops_per_sec));
        }
        table.add_row(row);
    }

    // P99 latency
    {
        std::vector<std::string> row = {"p99 Latency"};
        for (const auto& name : names) {
            row.push_back(format_ns(results[name].add_latency.p99()));
        }
        table.add_row(row);
    }

    table.print();

    std::cout << "\nComparison complete!\n";
}

// Run simulation mode
void run_simulation() {
    std::cout << "\n=== Running Feed Simulator ===\n";

    SimulatorConfig config;
    config.num_orders = 100'000;
    config.mode = SimulationMode::BURST;

    MatchingEngine engine(OrderBookType::ARRAY); // Use fastest implementation
    FeedSimulator sim(config);

    sim.run(engine);
    engine.stats().print_summary();
}

// Run benchmark mode
void run_benchmark() {
    std::cout << "\n=== Running Latency Benchmarks ===\n";

    // Quick benchmark on each operation
    MatchingEngine engine(OrderBookType::ARRAY);

    std::cout << "\nRunning add order benchmark (10,000 iterations)...\n";
    for (int i = 0; i < 10'000; ++i) {
        Order order(i, OrderSide::BUY, OrderType::LIMIT, 10000 + (i % 100), 100, Timer::now_ns());
        engine.process_order(order);
    }

    engine.stats().print_summary();
}

// Interactive REPL mode
void run_interactive() {
    std::cout << "\n=== Interactive Mode ===\n";
    std::cout << "Commands:\n";
    std::cout << "  add <side> <price> <qty>  - Add order (e.g., 'add buy 100.00 500')\n";
    std::cout << "  cancel <id>               - Cancel order by ID\n";
    std::cout << "  book                      - Show order book\n";
    std::cout << "  stats                     - Show statistics\n";
    std::cout << "  quit                      - Exit\n\n";

    MatchingEngine engine(OrderBookType::ARRAY);
    uint64_t next_id = 1;

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "quit" || cmd == "exit") {
            break;
        } else if (cmd == "add") {
            std::string side_str;
            double price_dollars;
            uint64_t qty;

            if (iss >> side_str >> price_dollars >> qty) {
                OrderSide side = (side_str == "buy" || side_str == "BUY")
                    ? OrderSide::BUY : OrderSide::SELL;

                int64_t price_ticks = static_cast<int64_t>(price_dollars * 100);
                Order order(next_id++, side, OrderType::LIMIT, price_ticks, qty, Timer::now_ns());

                engine.process_order(order);
                std::cout << "Order " << (next_id - 1) << " added\n";
            } else {
                std::cout << "Usage: add <buy|sell> <price> <qty>\n";
            }
        } else if (cmd == "cancel") {
            uint64_t id;
            if (iss >> id) {
                bool result = engine.cancel_order(id);
                if (result) {
                    std::cout << "Order " << id << " cancelled\n";
                } else {
                    std::cout << "Order " << id << " not found\n";
                }
            } else {
                std::cout << "Usage: cancel <id>\n";
            }
        } else if (cmd == "book") {
            visualize_order_book(engine.order_book());
        } else if (cmd == "stats") {
            engine.stats().print_summary();
        } else if (!cmd.empty()) {
            std::cout << "Unknown command: " << cmd << "\n";
        }
    }

    std::cout << "Goodbye!\n";
}

void print_help() {
    std::cout << "HFT Order Book Matching Engine\n";
    std::cout << "===============================\n\n";
    std::cout << "Usage: hft_engine [MODE]\n\n";
    std::cout << "Modes:\n";
    std::cout << "  --benchmark     Run latency benchmarks and print summary\n";
    std::cout << "  --simulate      Run feed simulator with default settings\n";
    std::cout << "  --interactive   Interactive REPL for manual testing\n";
    std::cout << "  --compare       Compare all three data structure implementations\n";
    std::cout << "  --help          Show this help message\n\n";
    std::cout << "Default: --compare\n\n";
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "--compare";

    if (mode == "--help" || mode == "-h") {
        print_help();
    } else if (mode == "--benchmark") {
        run_benchmark();
    } else if (mode == "--simulate") {
        run_simulation();
    } else if (mode == "--interactive") {
        run_interactive();
    } else if (mode == "--compare") {
        run_comparison();
    } else {
        std::cerr << "Unknown mode: " << mode << "\n";
        std::cerr << "Use --help for usage information\n";
        return 1;
    }

    return 0;
}
