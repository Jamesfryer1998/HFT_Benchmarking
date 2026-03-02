#pragma once

#include "order.hpp"
#include "latency_tracker.hpp"
#include <map>
#include <unordered_map>
#include <deque>
#include <vector>
#include <optional>
#include <memory>

namespace hft {

enum class OrderBookType {
    MAP,            // std::map based (baseline)
    HASH,           // std::unordered_map based
    ARRAY           // Pre-allocated array based
};

// Common result types
struct MatchResult {
    bool matched;
    uint64_t matched_order_id;
    uint64_t quantity_filled;
    int64_t price;
};

struct AddResult {
    bool success;
    uint64_t order_id;
};

// Abstract base class for order books
class IOrderBook {
public:
    virtual ~IOrderBook() = default;

    virtual AddResult add_order(const Order& order) noexcept = 0;
    virtual bool cancel_order(uint64_t order_id) noexcept = 0;
    virtual std::optional<MatchResult> match_order(const Order& order) noexcept = 0;

    virtual size_t bid_depth() const noexcept = 0;
    virtual size_t ask_depth() const noexcept = 0;

    // Get best bid/ask prices
    virtual std::optional<int64_t> best_bid() const noexcept = 0;
    virtual std::optional<int64_t> best_ask() const noexcept = 0;

    // Get top N levels for visualization
    virtual std::vector<std::pair<int64_t, uint64_t>> get_bids(size_t n) const noexcept = 0;
    virtual std::vector<std::pair<int64_t, uint64_t>> get_asks(size_t n) const noexcept = 0;
};

// Price level using FIFO queue for price-time priority
struct PriceLevel {
    int64_t price;
    std::deque<Order> orders;
    uint64_t total_quantity;

    PriceLevel() : price(0), orders(), total_quantity(0) {}
    explicit PriceLevel(int64_t p) : price(p), orders(), total_quantity(0) {}

    void add_order(const Order& order) noexcept {
        orders.push_back(order);
        total_quantity += order.quantity;
    }

    void remove_front() noexcept {
        if (!orders.empty()) {
            total_quantity -= orders.front().quantity;
            orders.pop_front();
        }
    }

    bool is_empty() const noexcept {
        return orders.empty();
    }
};

// Map-based order book implementation
class MapOrderBook : public IOrderBook {
public:
    MapOrderBook() = default;

    AddResult add_order(const Order& order) noexcept override;
    bool cancel_order(uint64_t order_id) noexcept override;
    std::optional<MatchResult> match_order(const Order& order) noexcept override;

    size_t bid_depth() const noexcept override { return bids_.size(); }
    size_t ask_depth() const noexcept override { return asks_.size(); }

    std::optional<int64_t> best_bid() const noexcept override;
    std::optional<int64_t> best_ask() const noexcept override;

    std::vector<std::pair<int64_t, uint64_t>> get_bids(size_t n) const noexcept override;
    std::vector<std::pair<int64_t, uint64_t>> get_asks(size_t n) const noexcept override;

private:
    std::map<int64_t, PriceLevel, std::greater<int64_t>> bids_;  // Descending order
    std::map<int64_t, PriceLevel, std::less<int64_t>> asks_;     // Ascending order
    std::unordered_map<uint64_t, int64_t> order_id_to_price_;
};

// Hash-based order book implementation
class HashOrderBook : public IOrderBook {
public:
    HashOrderBook() = default;

    AddResult add_order(const Order& order) noexcept override;
    bool cancel_order(uint64_t order_id) noexcept override;
    std::optional<MatchResult> match_order(const Order& order) noexcept override;

    size_t bid_depth() const noexcept override { return bids_.size(); }
    size_t ask_depth() const noexcept override { return asks_.size(); }

    std::optional<int64_t> best_bid() const noexcept override;
    std::optional<int64_t> best_ask() const noexcept override;

    std::vector<std::pair<int64_t, uint64_t>> get_bids(size_t n) const noexcept override;
    std::vector<std::pair<int64_t, uint64_t>> get_asks(size_t n) const noexcept override;

private:
    std::unordered_map<int64_t, PriceLevel> bids_;
    std::unordered_map<int64_t, PriceLevel> asks_;
    std::unordered_map<uint64_t, int64_t> order_id_to_price_;
    int64_t best_bid_price_ = 0;
    int64_t best_ask_price_ = std::numeric_limits<int64_t>::max();
};

// Array-based order book implementation (most cache-friendly)
// Uses fixed tick size of $0.01 (1 cent)
class ArrayOrderBook : public IOrderBook {
public:
    // Price range: $0 to $10,000 with $0.01 ticks = 1,000,000 levels
    static constexpr int64_t MIN_PRICE = 0;
    static constexpr int64_t MAX_PRICE = 1'000'000;  // $10,000 in ticks
    static constexpr int64_t TICK_SIZE = 1;          // $0.01
    static constexpr size_t NUM_LEVELS = MAX_PRICE - MIN_PRICE;

    ArrayOrderBook();

    AddResult add_order(const Order& order) noexcept override;
    bool cancel_order(uint64_t order_id) noexcept override;
    std::optional<MatchResult> match_order(const Order& order) noexcept override;

    size_t bid_depth() const noexcept override { return bid_count_; }
    size_t ask_depth() const noexcept override { return ask_count_; }

    std::optional<int64_t> best_bid() const noexcept override;
    std::optional<int64_t> best_ask() const noexcept override;

    std::vector<std::pair<int64_t, uint64_t>> get_bids(size_t n) const noexcept override;
    std::vector<std::pair<int64_t, uint64_t>> get_asks(size_t n) const noexcept override;

private:
    [[nodiscard]] bool is_valid_price(int64_t price) const noexcept {
        return price >= MIN_PRICE && price < MAX_PRICE;
    }

    std::vector<PriceLevel> levels_;
    std::unordered_map<uint64_t, int64_t> order_id_to_price_;
    int64_t best_bid_price_ = MIN_PRICE;
    int64_t best_ask_price_ = MAX_PRICE;
    size_t bid_count_ = 0;
    size_t ask_count_ = 0;
};

// Factory function
std::unique_ptr<IOrderBook> create_order_book(OrderBookType type);

} // namespace hft
