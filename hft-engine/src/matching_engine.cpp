#include "matching_engine.hpp"
#include <iostream>
#include <iomanip>

namespace hft {

MatchingEngine::MatchingEngine(OrderBookType book_type)
    : order_book_(create_order_book(book_type))
    , stats_()
    , next_order_id_(1)
{}

void MatchingEngine::process_order(const Order& order) {
    // Try to match first if it's a marketable order
    Timer timer;
    auto match_result = order_book_->match_order(order);

    if (match_result) {
        stats_.match_latency.record(timer.elapsed_ns());
        stats_.total_orders_matched++;
        stats_.total_volume_matched += match_result->quantity_filled;
    } else {
        // No match, add to book
        timer.reset();
        auto add_result = order_book_->add_order(order);
        stats_.add_latency.record(timer.elapsed_ns());

        if (add_result.success) {
            stats_.total_orders_added++;
        }
    }
}

bool MatchingEngine::cancel_order(uint64_t order_id) {
    Timer timer;
    bool result = order_book_->cancel_order(order_id);
    stats_.cancel_latency.record(timer.elapsed_ns());

    if (result) {
        stats_.total_orders_cancelled++;
    }

    return result;
}

void MatchingEngine::reset_stats() {
    stats_ = EngineStats();
}

void EngineStats::print_summary() const {
    std::cout << "\n=== Engine Statistics ===\n";
    std::cout << "Total Orders Added:    " << total_orders_added << "\n";
    std::cout << "Total Orders Matched:  " << total_orders_matched << "\n";
    std::cout << "Total Orders Cancelled: " << total_orders_cancelled << "\n";
    std::cout << "Total Volume Matched:  " << total_volume_matched << "\n";

    std::cout << "\n--- Add Latency ---\n";
    std::cout << "  Count: " << add_latency.count() << "\n";
    std::cout << "  Mean:  " << add_latency.mean() << " ns\n";
    std::cout << "  Min:   " << add_latency.min() << " ns\n";
    std::cout << "  Max:   " << add_latency.max() << " ns\n";
    std::cout << "  P50:   " << add_latency.p50() << " ns\n";
    std::cout << "  P95:   " << add_latency.p95() << " ns\n";
    std::cout << "  P99:   " << add_latency.p99() << " ns\n";

    if (match_latency.count() > 0) {
        std::cout << "\n--- Match Latency ---\n";
        std::cout << "  Count: " << match_latency.count() << "\n";
        std::cout << "  Mean:  " << match_latency.mean() << " ns\n";
        std::cout << "  Min:   " << match_latency.min() << " ns\n";
        std::cout << "  Max:   " << match_latency.max() << " ns\n";
        std::cout << "  P50:   " << match_latency.p50() << " ns\n";
        std::cout << "  P95:   " << match_latency.p95() << " ns\n";
        std::cout << "  P99:   " << match_latency.p99() << " ns\n";
    }

    if (cancel_latency.count() > 0) {
        std::cout << "\n--- Cancel Latency ---\n";
        std::cout << "  Count: " << cancel_latency.count() << "\n";
        std::cout << "  Mean:  " << cancel_latency.mean() << " ns\n";
        std::cout << "  Min:   " << cancel_latency.min() << " ns\n";
        std::cout << "  Max:   " << cancel_latency.max() << " ns\n";
        std::cout << "  P50:   " << cancel_latency.p50() << " ns\n";
        std::cout << "  P95:   " << cancel_latency.p95() << " ns\n";
        std::cout << "  P99:   " << cancel_latency.p99() << " ns\n";
    }
}

} // namespace hft
