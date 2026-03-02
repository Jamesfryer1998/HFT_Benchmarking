#include "order_book.hpp"
#include "matching_engine.hpp"
#include <benchmark/benchmark.h>

using namespace hft;

// Benchmark: Add Order - Map Implementation
static void BM_AddOrder_Map(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::MAP);
    uint64_t order_id = 1;

    for (auto _ : state) {
        Order order(order_id++, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        auto result = book->add_order(order);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddOrder_Map);

// Benchmark: Add Order - Hash Implementation
static void BM_AddOrder_Hash(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::HASH);
    uint64_t order_id = 1;

    for (auto _ : state) {
        Order order(order_id++, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        auto result = book->add_order(order);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddOrder_Hash);

// Benchmark: Add Order - Array Implementation
static void BM_AddOrder_Array(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::ARRAY);
    uint64_t order_id = 1;

    for (auto _ : state) {
        Order order(order_id++, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        auto result = book->add_order(order);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddOrder_Array);

// Benchmark: Match Order - Map Implementation
static void BM_MatchOrder_Map(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::MAP);

    // Pre-populate with some asks
    for (int i = 0; i < 1000; ++i) {
        Order ask(i, OrderSide::SELL, OrderType::LIMIT, 10000 + i, 100, Timer::now_ns());
        book->add_order(ask);
    }

    uint64_t order_id = 100000;
    for (auto _ : state) {
        Order buy(order_id++, OrderSide::BUY, OrderType::LIMIT, 10500, 100, Timer::now_ns());
        auto result = book->match_order(buy);
        benchmark::DoNotOptimize(result);

        // Re-add the matched order to keep book populated
        if (result) {
            Order ask(order_id, OrderSide::SELL, OrderType::LIMIT, result->price, 100, Timer::now_ns());
            book->add_order(ask);
        }
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MatchOrder_Map);

// Benchmark: Match Order - Hash Implementation
static void BM_MatchOrder_Hash(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::HASH);

    // Pre-populate with some asks
    for (int i = 0; i < 1000; ++i) {
        Order ask(i, OrderSide::SELL, OrderType::LIMIT, 10000 + i, 100, Timer::now_ns());
        book->add_order(ask);
    }

    uint64_t order_id = 100000;
    for (auto _ : state) {
        Order buy(order_id++, OrderSide::BUY, OrderType::LIMIT, 10500, 100, Timer::now_ns());
        auto result = book->match_order(buy);
        benchmark::DoNotOptimize(result);

        // Re-add the matched order
        if (result) {
            Order ask(order_id, OrderSide::SELL, OrderType::LIMIT, result->price, 100, Timer::now_ns());
            book->add_order(ask);
        }
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MatchOrder_Hash);

// Benchmark: Match Order - Array Implementation
static void BM_MatchOrder_Array(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::ARRAY);

    // Pre-populate with some asks
    for (int i = 0; i < 1000; ++i) {
        Order ask(i, OrderSide::SELL, OrderType::LIMIT, 10000 + i, 100, Timer::now_ns());
        book->add_order(ask);
    }

    uint64_t order_id = 100000;
    for (auto _ : state) {
        Order buy(order_id++, OrderSide::BUY, OrderType::LIMIT, 10500, 100, Timer::now_ns());
        auto result = book->match_order(buy);
        benchmark::DoNotOptimize(result);

        // Re-add the matched order
        if (result) {
            Order ask(order_id, OrderSide::SELL, OrderType::LIMIT, result->price, 100, Timer::now_ns());
            book->add_order(ask);
        }
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MatchOrder_Array);

// Benchmark: Cancel Order - Map Implementation
static void BM_CancelOrder_Map(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::MAP);
    uint64_t order_id = 1;

    for (auto _ : state) {
        state.PauseTiming();
        // Add an order
        Order order(order_id, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        book->add_order(order);
        state.ResumeTiming();

        // Cancel it
        bool result = book->cancel_order(order_id);
        benchmark::DoNotOptimize(result);

        order_id++;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CancelOrder_Map);

// Benchmark: Cancel Order - Hash Implementation
static void BM_CancelOrder_Hash(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::HASH);
    uint64_t order_id = 1;

    for (auto _ : state) {
        state.PauseTiming();
        Order order(order_id, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        book->add_order(order);
        state.ResumeTiming();

        bool result = book->cancel_order(order_id);
        benchmark::DoNotOptimize(result);

        order_id++;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CancelOrder_Hash);

// Benchmark: Cancel Order - Array Implementation
static void BM_CancelOrder_Array(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::ARRAY);
    uint64_t order_id = 1;

    for (auto _ : state) {
        state.PauseTiming();
        Order order(order_id, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        book->add_order(order);
        state.ResumeTiming();

        bool result = book->cancel_order(order_id);
        benchmark::DoNotOptimize(result);

        order_id++;
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CancelOrder_Array);

BENCHMARK_MAIN();
