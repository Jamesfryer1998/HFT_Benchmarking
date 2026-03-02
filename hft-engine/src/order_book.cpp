#include "order_book.hpp"
#include <algorithm>

namespace hft {

// ============================================================================
// MapOrderBook Implementation
// ============================================================================

AddResult MapOrderBook::add_order(const Order& order) noexcept {
    if (order.is_buy()) {
        auto& level = bids_[order.price];
        level.price = order.price;
        level.add_order(order);
    } else {
        auto& level = asks_[order.price];
        level.price = order.price;
        level.add_order(order);
    }
    order_id_to_price_[order.order_id] = order.price;
    return {true, order.order_id};
}

bool MapOrderBook::cancel_order(uint64_t order_id) noexcept {
    auto it = order_id_to_price_.find(order_id);
    if (it == order_id_to_price_.end()) {
        return false;
    }

    int64_t price = it->second;
    order_id_to_price_.erase(it);

    // Try to find in bids
    auto bid_it = bids_.find(price);
    if (bid_it != bids_.end()) {
        auto& orders = bid_it->second.orders;
        auto order_it = std::find_if(orders.begin(), orders.end(),
            [order_id](const Order& o) { return o.order_id == order_id; });

        if (order_it != orders.end()) {
            bid_it->second.total_quantity -= order_it->quantity;
            orders.erase(order_it);
            if (orders.empty()) {
                bids_.erase(bid_it);
            }
            return true;
        }
    }

    // Try to find in asks
    auto ask_it = asks_.find(price);
    if (ask_it != asks_.end()) {
        auto& orders = ask_it->second.orders;
        auto order_it = std::find_if(orders.begin(), orders.end(),
            [order_id](const Order& o) { return o.order_id == order_id; });

        if (order_it != orders.end()) {
            ask_it->second.total_quantity -= order_it->quantity;
            orders.erase(order_it);
            if (orders.empty()) {
                asks_.erase(ask_it);
            }
            return true;
        }
    }

    return false;
}

std::optional<MatchResult> MapOrderBook::match_order(const Order& order) noexcept {
    if (order.is_buy()) {
        // Buy order: match against best ask
        if (asks_.empty()) {
            return std::nullopt;
        }

        auto& best_ask_level = asks_.begin()->second;
        if (order.is_market() || order.price >= best_ask_level.price) {
            auto& matched_order = best_ask_level.orders.front();
            uint64_t fill_qty = std::min(order.quantity, matched_order.quantity);

            MatchResult result{
                true,
                matched_order.order_id,
                fill_qty,
                best_ask_level.price
            };

            // Update or remove matched order
            matched_order.quantity -= fill_qty;
            best_ask_level.total_quantity -= fill_qty;

            if (matched_order.quantity == 0) {
                order_id_to_price_.erase(matched_order.order_id);
                best_ask_level.remove_front();
                if (best_ask_level.is_empty()) {
                    asks_.erase(asks_.begin());
                }
            }

            return result;
        }
    } else {
        // Sell order: match against best bid
        if (bids_.empty()) {
            return std::nullopt;
        }

        auto& best_bid_level = bids_.begin()->second;
        if (order.is_market() || order.price <= best_bid_level.price) {
            auto& matched_order = best_bid_level.orders.front();
            uint64_t fill_qty = std::min(order.quantity, matched_order.quantity);

            MatchResult result{
                true,
                matched_order.order_id,
                fill_qty,
                best_bid_level.price
            };

            // Update or remove matched order
            matched_order.quantity -= fill_qty;
            best_bid_level.total_quantity -= fill_qty;

            if (matched_order.quantity == 0) {
                order_id_to_price_.erase(matched_order.order_id);
                best_bid_level.remove_front();
                if (best_bid_level.is_empty()) {
                    bids_.erase(bids_.begin());
                }
            }

            return result;
        }
    }

    return std::nullopt;
}

std::optional<int64_t> MapOrderBook::best_bid() const noexcept {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<int64_t> MapOrderBook::best_ask() const noexcept {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::vector<std::pair<int64_t, uint64_t>> MapOrderBook::get_bids(size_t n) const noexcept {
    std::vector<std::pair<int64_t, uint64_t>> result;
    result.reserve(n);

    size_t count = 0;
    for (const auto& [price, level] : bids_) {
        if (count >= n) break;
        result.emplace_back(price, level.total_quantity);
        ++count;
    }

    return result;
}

std::vector<std::pair<int64_t, uint64_t>> MapOrderBook::get_asks(size_t n) const noexcept {
    std::vector<std::pair<int64_t, uint64_t>> result;
    result.reserve(n);

    size_t count = 0;
    for (const auto& [price, level] : asks_) {
        if (count >= n) break;
        result.emplace_back(price, level.total_quantity);
        ++count;
    }

    return result;
}

// ============================================================================
// HashOrderBook Implementation
// ============================================================================

AddResult HashOrderBook::add_order(const Order& order) noexcept {
    if (order.is_buy()) {
        auto& level = bids_[order.price];
        level.price = order.price;
        level.add_order(order);

        if (order.price > best_bid_price_ || best_bid_price_ == 0) {
            best_bid_price_ = order.price;
        }
    } else {
        auto& level = asks_[order.price];
        level.price = order.price;
        level.add_order(order);

        if (order.price < best_ask_price_) {
            best_ask_price_ = order.price;
        }
    }

    order_id_to_price_[order.order_id] = order.price;
    return {true, order.order_id};
}

bool HashOrderBook::cancel_order(uint64_t order_id) noexcept {
    auto it = order_id_to_price_.find(order_id);
    if (it == order_id_to_price_.end()) {
        return false;
    }

    int64_t price = it->second;
    order_id_to_price_.erase(it);

    // Try bids
    auto bid_it = bids_.find(price);
    if (bid_it != bids_.end()) {
        auto& orders = bid_it->second.orders;
        auto order_it = std::find_if(orders.begin(), orders.end(),
            [order_id](const Order& o) { return o.order_id == order_id; });

        if (order_it != orders.end()) {
            bid_it->second.total_quantity -= order_it->quantity;
            orders.erase(order_it);

            if (orders.empty()) {
                bids_.erase(bid_it);
                // Recalculate best bid if necessary
                if (price == best_bid_price_) {
                    best_bid_price_ = 0;
                    for (const auto& [p, _] : bids_) {
                        if (p > best_bid_price_) best_bid_price_ = p;
                    }
                }
            }
            return true;
        }
    }

    // Try asks
    auto ask_it = asks_.find(price);
    if (ask_it != asks_.end()) {
        auto& orders = ask_it->second.orders;
        auto order_it = std::find_if(orders.begin(), orders.end(),
            [order_id](const Order& o) { return o.order_id == order_id; });

        if (order_it != orders.end()) {
            ask_it->second.total_quantity -= order_it->quantity;
            orders.erase(order_it);

            if (orders.empty()) {
                asks_.erase(ask_it);
                // Recalculate best ask if necessary
                if (price == best_ask_price_) {
                    best_ask_price_ = std::numeric_limits<int64_t>::max();
                    for (const auto& [p, _] : asks_) {
                        if (p < best_ask_price_) best_ask_price_ = p;
                    }
                }
            }
            return true;
        }
    }

    return false;
}

std::optional<MatchResult> HashOrderBook::match_order(const Order& order) noexcept {
    if (order.is_buy()) {
        auto it = asks_.find(best_ask_price_);
        if (it == asks_.end() || best_ask_price_ == std::numeric_limits<int64_t>::max()) {
            return std::nullopt;
        }

        auto& best_ask_level = it->second;
        if (order.is_market() || order.price >= best_ask_level.price) {
            auto& matched_order = best_ask_level.orders.front();
            uint64_t fill_qty = std::min(order.quantity, matched_order.quantity);

            MatchResult result{
                true,
                matched_order.order_id,
                fill_qty,
                best_ask_level.price
            };

            matched_order.quantity -= fill_qty;
            best_ask_level.total_quantity -= fill_qty;

            if (matched_order.quantity == 0) {
                order_id_to_price_.erase(matched_order.order_id);
                best_ask_level.remove_front();

                if (best_ask_level.is_empty()) {
                    asks_.erase(it);
                    // Recalculate best ask
                    best_ask_price_ = std::numeric_limits<int64_t>::max();
                    for (const auto& [p, _] : asks_) {
                        if (p < best_ask_price_) best_ask_price_ = p;
                    }
                }
            }

            return result;
        }
    } else {
        auto it = bids_.find(best_bid_price_);
        if (it == bids_.end() || best_bid_price_ == 0) {
            return std::nullopt;
        }

        auto& best_bid_level = it->second;
        if (order.is_market() || order.price <= best_bid_level.price) {
            auto& matched_order = best_bid_level.orders.front();
            uint64_t fill_qty = std::min(order.quantity, matched_order.quantity);

            MatchResult result{
                true,
                matched_order.order_id,
                fill_qty,
                best_bid_level.price
            };

            matched_order.quantity -= fill_qty;
            best_bid_level.total_quantity -= fill_qty;

            if (matched_order.quantity == 0) {
                order_id_to_price_.erase(matched_order.order_id);
                best_bid_level.remove_front();

                if (best_bid_level.is_empty()) {
                    bids_.erase(it);
                    // Recalculate best bid
                    best_bid_price_ = 0;
                    for (const auto& [p, _] : bids_) {
                        if (p > best_bid_price_) best_bid_price_ = p;
                    }
                }
            }

            return result;
        }
    }

    return std::nullopt;
}

std::optional<int64_t> HashOrderBook::best_bid() const noexcept {
    if (bids_.empty() || best_bid_price_ == 0) return std::nullopt;
    return best_bid_price_;
}

std::optional<int64_t> HashOrderBook::best_ask() const noexcept {
    if (asks_.empty() || best_ask_price_ == std::numeric_limits<int64_t>::max()) {
        return std::nullopt;
    }
    return best_ask_price_;
}

std::vector<std::pair<int64_t, uint64_t>> HashOrderBook::get_bids(size_t n) const noexcept {
    std::vector<std::pair<int64_t, uint64_t>> result;
    for (const auto& [price, level] : bids_) {
        result.emplace_back(price, level.total_quantity);
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    if (result.size() > n) {
        result.resize(n);
    }

    return result;
}

std::vector<std::pair<int64_t, uint64_t>> HashOrderBook::get_asks(size_t n) const noexcept {
    std::vector<std::pair<int64_t, uint64_t>> result;
    for (const auto& [price, level] : asks_) {
        result.emplace_back(price, level.total_quantity);
    }

    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    if (result.size() > n) {
        result.resize(n);
    }

    return result;
}

// ============================================================================
// ArrayOrderBook Implementation
// ============================================================================

ArrayOrderBook::ArrayOrderBook()
    : levels_(NUM_LEVELS)
    , order_id_to_price_()
    , best_bid_price_(MIN_PRICE)
    , best_ask_price_(MAX_PRICE)
    , bid_count_(0)
    , ask_count_(0)
{
    // Initialize price levels
    for (size_t i = 0; i < NUM_LEVELS; ++i) {
        levels_[i].price = static_cast<int64_t>(i);
    }
}

AddResult ArrayOrderBook::add_order(const Order& order) noexcept {
    if (!is_valid_price(order.price)) {
        return {false, 0};
    }

    auto& level = levels_[order.price];
    level.add_order(order);

    if (order.is_buy()) {
        if (order.price > best_bid_price_) {
            best_bid_price_ = order.price;
        }
        ++bid_count_;
    } else {
        if (order.price < best_ask_price_) {
            best_ask_price_ = order.price;
        }
        ++ask_count_;
    }

    order_id_to_price_[order.order_id] = order.price;
    return {true, order.order_id};
}

bool ArrayOrderBook::cancel_order(uint64_t order_id) noexcept {
    auto it = order_id_to_price_.find(order_id);
    if (it == order_id_to_price_.end()) {
        return false;
    }

    int64_t price = it->second;
    if (!is_valid_price(price)) {
        return false;
    }

    auto& level = levels_[price];
    auto& orders = level.orders;

    auto order_it = std::find_if(orders.begin(), orders.end(),
        [order_id](const Order& o) { return o.order_id == order_id; });

    if (order_it != orders.end()) {
        bool is_buy = order_it->is_buy();
        level.total_quantity -= order_it->quantity;
        orders.erase(order_it);
        order_id_to_price_.erase(it);

        if (is_buy) {
            --bid_count_;
            // Recalculate best bid if necessary
            if (price == best_bid_price_ && level.is_empty()) {
                best_bid_price_ = MIN_PRICE;
                for (int64_t p = price - 1; p >= MIN_PRICE; --p) {
                    if (!levels_[p].is_empty() && !levels_[p].orders.empty() &&
                        levels_[p].orders.front().is_buy()) {
                        best_bid_price_ = p;
                        break;
                    }
                }
            }
        } else {
            --ask_count_;
            // Recalculate best ask if necessary
            if (price == best_ask_price_ && level.is_empty()) {
                best_ask_price_ = MAX_PRICE;
                for (int64_t p = price + 1; p < MAX_PRICE; ++p) {
                    if (!levels_[p].is_empty() && !levels_[p].orders.empty() &&
                        levels_[p].orders.front().is_sell()) {
                        best_ask_price_ = p;
                        break;
                    }
                }
            }
        }

        return true;
    }

    return false;
}

std::optional<MatchResult> ArrayOrderBook::match_order(const Order& order) noexcept {
    if (order.is_buy()) {
        if (!is_valid_price(best_ask_price_) || best_ask_price_ == MAX_PRICE) {
            return std::nullopt;
        }

        auto& best_ask_level = levels_[best_ask_price_];
        if (best_ask_level.is_empty()) {
            return std::nullopt;
        }

        if (order.is_market() || order.price >= best_ask_level.price) {
            auto& matched_order = best_ask_level.orders.front();
            uint64_t fill_qty = std::min(order.quantity, matched_order.quantity);

            MatchResult result{
                true,
                matched_order.order_id,
                fill_qty,
                best_ask_level.price
            };

            matched_order.quantity -= fill_qty;
            best_ask_level.total_quantity -= fill_qty;

            if (matched_order.quantity == 0) {
                order_id_to_price_.erase(matched_order.order_id);
                best_ask_level.remove_front();
                --ask_count_;

                if (best_ask_level.is_empty()) {
                    // Find next best ask
                    best_ask_price_ = MAX_PRICE;
                    for (int64_t p = best_ask_level.price + 1; p < MAX_PRICE; ++p) {
                        if (!levels_[p].is_empty() && !levels_[p].orders.empty() &&
                            levels_[p].orders.front().is_sell()) {
                            best_ask_price_ = p;
                            break;
                        }
                    }
                }
            }

            return result;
        }
    } else {
        if (!is_valid_price(best_bid_price_) || best_bid_price_ == MIN_PRICE) {
            return std::nullopt;
        }

        auto& best_bid_level = levels_[best_bid_price_];
        if (best_bid_level.is_empty()) {
            return std::nullopt;
        }

        if (order.is_market() || order.price <= best_bid_level.price) {
            auto& matched_order = best_bid_level.orders.front();
            uint64_t fill_qty = std::min(order.quantity, matched_order.quantity);

            MatchResult result{
                true,
                matched_order.order_id,
                fill_qty,
                best_bid_level.price
            };

            matched_order.quantity -= fill_qty;
            best_bid_level.total_quantity -= fill_qty;

            if (matched_order.quantity == 0) {
                order_id_to_price_.erase(matched_order.order_id);
                best_bid_level.remove_front();
                --bid_count_;

                if (best_bid_level.is_empty()) {
                    // Find next best bid
                    best_bid_price_ = MIN_PRICE;
                    for (int64_t p = best_bid_level.price - 1; p >= MIN_PRICE; --p) {
                        if (!levels_[p].is_empty() && !levels_[p].orders.empty() &&
                            levels_[p].orders.front().is_buy()) {
                            best_bid_price_ = p;
                            break;
                        }
                    }
                }
            }

            return result;
        }
    }

    return std::nullopt;
}

std::optional<int64_t> ArrayOrderBook::best_bid() const noexcept {
    if (bid_count_ == 0 || best_bid_price_ == MIN_PRICE) {
        return std::nullopt;
    }
    return best_bid_price_;
}

std::optional<int64_t> ArrayOrderBook::best_ask() const noexcept {
    if (ask_count_ == 0 || best_ask_price_ == MAX_PRICE) {
        return std::nullopt;
    }
    return best_ask_price_;
}

std::vector<std::pair<int64_t, uint64_t>> ArrayOrderBook::get_bids(size_t n) const noexcept {
    std::vector<std::pair<int64_t, uint64_t>> result;
    result.reserve(n);

    size_t count = 0;
    for (int64_t p = best_bid_price_; p >= MIN_PRICE && count < n; --p) {
        const auto& level = levels_[p];
        if (!level.is_empty() && !level.orders.empty() && level.orders.front().is_buy()) {
            result.emplace_back(p, level.total_quantity);
            ++count;
        }
    }

    return result;
}

std::vector<std::pair<int64_t, uint64_t>> ArrayOrderBook::get_asks(size_t n) const noexcept {
    std::vector<std::pair<int64_t, uint64_t>> result;
    result.reserve(n);

    size_t count = 0;
    for (int64_t p = best_ask_price_; p < MAX_PRICE && count < n; ++p) {
        const auto& level = levels_[p];
        if (!level.is_empty() && !level.orders.empty() && level.orders.front().is_sell()) {
            result.emplace_back(p, level.total_quantity);
            ++count;
        }
    }

    return result;
}

// ============================================================================
// Factory Function
// ============================================================================

std::unique_ptr<IOrderBook> create_order_book(OrderBookType type) {
    switch (type) {
        case OrderBookType::MAP:
            return std::make_unique<MapOrderBook>();
        case OrderBookType::HASH:
            return std::make_unique<HashOrderBook>();
        case OrderBookType::ARRAY:
            return std::make_unique<ArrayOrderBook>();
        default:
            return std::make_unique<MapOrderBook>();
    }
}

} // namespace hft
