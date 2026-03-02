#include "feed_simulator.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace hft {

FeedSimulator::FeedSimulator(const SimulatorConfig& config)
    : config_(config)
    , rng_(std::random_device{}())
    , price_dist_(config.min_price, config.max_price)
    , qty_dist_(config.min_quantity, config.max_quantity)
    , side_dist_(0.0, 1.0)
    , next_order_id_(1)
{}

Order FeedSimulator::generate_order() {
    OrderSide side = side_dist_(rng_) < config_.buy_ratio ? OrderSide::BUY : OrderSide::SELL;
    int64_t price = price_dist_(rng_);
    uint64_t quantity = qty_dist_(rng_);
    uint64_t timestamp = Timer::now_ns();

    return Order(
        next_order_id_++,
        side,
        OrderType::LIMIT,
        price,
        quantity,
        timestamp
    );
}

SimulationResult FeedSimulator::run(MatchingEngine& engine) {
    if (!config_.quiet) {
        std::cout << "Starting feed simulation...\n";
        std::cout << "  Mode: " << (config_.mode == SimulationMode::BURST ? "BURST" : "TIMED") << "\n";
        std::cout << "  Orders: " << config_.num_orders << "\n";
        std::cout << "  Price range: $" << std::fixed << std::setprecision(2)
                  << (config_.min_price / 100.0) << " - $" << (config_.max_price / 100.0) << "\n";
    }

    Timer timer;

    if (config_.mode == SimulationMode::BURST) {
        // Send all orders as fast as possible
        for (size_t i = 0; i < config_.num_orders; ++i) {
            Order order = generate_order();
            engine.process_order(order);
        }
    } else {
        // Timed mode: send orders at target rate
        uint64_t interval_ns = 1'000'000'000 / config_.target_rate;

        for (size_t i = 0; i < config_.num_orders; ++i) {
            auto start = std::chrono::high_resolution_clock::now();

            Order order = generate_order();
            engine.process_order(order);

            // Sleep to maintain target rate
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();

            if (elapsed_ns < static_cast<int64_t>(interval_ns)) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(interval_ns - elapsed_ns));
            }
        }
    }

    uint64_t duration_ns = timer.elapsed_ns();
    double duration_sec = duration_ns / 1e9;
    double orders_per_sec = config_.num_orders / duration_sec;

    SimulationResult result{
        config_.num_orders,
        duration_ns,
        orders_per_sec
    };

    if (!config_.quiet) {
        std::cout << "\nSimulation complete!\n";
        result.print();
    }

    return result;
}

void SimulationResult::print() const {
    std::cout << "\n=== Simulation Results ===\n";
    std::cout << "Total Orders:      " << total_orders << "\n";
    std::cout << "Duration:          " << std::fixed << std::setprecision(3)
              << (duration_ns / 1e9) << " seconds\n";
    std::cout << "Throughput:        " << std::fixed << std::setprecision(0)
              << orders_per_second << " orders/sec\n";
    std::cout << "Average Latency:   " << std::fixed << std::setprecision(0)
              << (duration_ns / total_orders) << " ns/order\n";
}

} // namespace hft
