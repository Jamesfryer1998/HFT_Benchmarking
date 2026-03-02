#pragma once

#include <cstdint>

namespace hft {

enum class OrderSide : uint8_t {
    BUY = 0,
    SELL = 1
};

enum class OrderType : uint8_t {
    LIMIT = 0,
    MARKET = 1
};

// Cache-friendly Order struct (56 bytes, fits in cache line)
struct alignas(64) Order {
    uint64_t order_id;      // 8 bytes
    uint64_t timestamp;     // 8 bytes (nanoseconds since epoch)
    int64_t price;          // 8 bytes (price in ticks, e.g., 100 = $1.00 for $0.01 tick)
    uint64_t quantity;      // 8 bytes
    OrderSide side;         // 1 byte
    OrderType type;         // 1 byte
    uint8_t padding[22];    // 22 bytes padding to reach 64 bytes

    Order() noexcept = default;

    Order(uint64_t id, OrderSide s, OrderType t, int64_t p, uint64_t q, uint64_t ts) noexcept
        : order_id(id)
        , timestamp(ts)
        , price(p)
        , quantity(q)
        , side(s)
        , type(t)
        , padding{}
    {}

    [[nodiscard]] bool is_buy() const noexcept { return side == OrderSide::BUY; }
    [[nodiscard]] bool is_sell() const noexcept { return side == OrderSide::SELL; }
    [[nodiscard]] bool is_limit() const noexcept { return type == OrderType::LIMIT; }
    [[nodiscard]] bool is_market() const noexcept { return type == OrderType::MARKET; }
};

static_assert(sizeof(Order) == 64, "Order struct must be exactly 64 bytes");

} // namespace hft
