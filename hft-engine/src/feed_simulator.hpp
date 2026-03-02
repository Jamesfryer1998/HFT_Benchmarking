#pragma once

#include "order.hpp"
#include "matching_engine.hpp"
#include <random>
#include <vector>

namespace hft {

enum class SimulationMode {
    BURST,      // Send all orders as fast as possible
    TIMED       // Send orders at a configurable rate
};

struct SimulatorConfig {
    size_t num_orders = 100000;
    int64_t min_price = 9900;       // $99.00
    int64_t max_price = 10100;      // $101.00
    uint64_t min_quantity = 100;
    uint64_t max_quantity = 1000;
    double buy_ratio = 0.5;         // 50% buy, 50% sell
    SimulationMode mode = SimulationMode::BURST;
    uint64_t target_rate = 1000000; // Orders per second (for TIMED mode)
    bool quiet = false;             // Suppress output when true
};

struct SimulationResult {
    size_t total_orders;
    uint64_t duration_ns;
    double orders_per_second;

    void print() const;
};

class FeedSimulator {
public:
    explicit FeedSimulator(const SimulatorConfig& config = SimulatorConfig());

    // Run simulation on a matching engine
    SimulationResult run(MatchingEngine& engine);

    // Generate a random order
    Order generate_order();

private:
    SimulatorConfig config_;
    std::mt19937_64 rng_;
    std::uniform_int_distribution<int64_t> price_dist_;
    std::uniform_int_distribution<uint64_t> qty_dist_;
    std::uniform_real_distribution<double> side_dist_;
    uint64_t next_order_id_ = 1;
};

} // namespace hft
