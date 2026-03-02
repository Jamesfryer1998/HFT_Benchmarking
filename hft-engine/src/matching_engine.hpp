#pragma once

#include "order.hpp"
#include "order_book.hpp"
#include "latency_tracker.hpp"
#include "memory_pool.hpp"
#include <memory>

namespace hft {

struct EngineStats {
    uint64_t total_orders_added = 0;
    uint64_t total_orders_matched = 0;
    uint64_t total_orders_cancelled = 0;
    uint64_t total_volume_matched = 0;

    LatencyTracker add_latency;
    LatencyTracker match_latency;
    LatencyTracker cancel_latency;

    EngineStats()
        : add_latency(1'000'000)
        , match_latency(1'000'000)
        , cancel_latency(1'000'000)
    {}

    void print_summary() const;
};

class MatchingEngine {
public:
    explicit MatchingEngine(OrderBookType book_type = OrderBookType::MAP);

    // Process an order (add or match)
    void process_order(const Order& order);

    // Cancel an order by ID
    bool cancel_order(uint64_t order_id);

    // Get statistics
    const EngineStats& stats() const { return stats_; }
    EngineStats& stats() { return stats_; }

    // Reset statistics
    void reset_stats();

    // Get order book for visualization
    const IOrderBook& order_book() const { return *order_book_; }
    IOrderBook& order_book() { return *order_book_; }

private:
    std::unique_ptr<IOrderBook> order_book_;
    EngineStats stats_;
    uint64_t next_order_id_ = 1;
};

} // namespace hft
