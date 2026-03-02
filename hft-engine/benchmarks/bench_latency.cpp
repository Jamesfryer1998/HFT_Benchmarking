#include "matching_engine.hpp"
#include "feed_simulator.hpp"
#include <benchmark/benchmark.h>

using namespace hft;

// Benchmark: Throughput Test - Map (1M orders)
static void BM_Throughput_Map(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        MatchingEngine engine(OrderBookType::MAP);
        SimulatorConfig config;
        config.num_orders = state.range(0);
        config.mode = SimulationMode::BURST;
        config.quiet = true;
        FeedSimulator sim(config);
        state.ResumeTiming();

        auto result = sim.run(engine);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Throughput_Map)->Arg(100'000)->Unit(benchmark::kMillisecond);

// Benchmark: Throughput Test - Hash (1M orders)
static void BM_Throughput_Hash(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        MatchingEngine engine(OrderBookType::HASH);
        SimulatorConfig config;
        config.num_orders = state.range(0);
        config.mode = SimulationMode::BURST;
        config.quiet = true;
        FeedSimulator sim(config);
        state.ResumeTiming();

        auto result = sim.run(engine);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Throughput_Hash)->Arg(100'000)->Unit(benchmark::kMillisecond);

// Benchmark: Throughput Test - Array (1M orders)
static void BM_Throughput_Array(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        MatchingEngine engine(OrderBookType::ARRAY);
        SimulatorConfig config;
        config.num_orders = state.range(0);
        config.mode = SimulationMode::BURST;
        config.quiet = true;
        FeedSimulator sim(config);
        state.ResumeTiming();

        auto result = sim.run(engine);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Throughput_Array)->Arg(100'000)->Unit(benchmark::kMillisecond);

// Benchmark: Mixed Workload (Add + Match)
static void BM_MixedWorkload_Map(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::MAP);
    uint64_t order_id = 1;

    // Pre-populate with some orders
    for (int i = 0; i < 500; ++i) {
        Order bid(order_id++, OrderSide::BUY, OrderType::LIMIT, 9900 + i, 100, Timer::now_ns());
        book->add_order(bid);
        Order ask(order_id++, OrderSide::SELL, OrderType::LIMIT, 10100 - i, 100, Timer::now_ns());
        book->add_order(ask);
    }

    for (auto _ : state) {
        // Alternate between adding and matching
        Order buy(order_id++, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        book->add_order(buy);

        Order sell(order_id++, OrderSide::SELL, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        auto result = book->match_order(sell);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_MixedWorkload_Map);

static void BM_MixedWorkload_Hash(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::HASH);
    uint64_t order_id = 1;

    // Pre-populate
    for (int i = 0; i < 500; ++i) {
        Order bid(order_id++, OrderSide::BUY, OrderType::LIMIT, 9900 + i, 100, Timer::now_ns());
        book->add_order(bid);
        Order ask(order_id++, OrderSide::SELL, OrderType::LIMIT, 10100 - i, 100, Timer::now_ns());
        book->add_order(ask);
    }

    for (auto _ : state) {
        Order buy(order_id++, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        book->add_order(buy);

        Order sell(order_id++, OrderSide::SELL, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        auto result = book->match_order(sell);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_MixedWorkload_Hash);

static void BM_MixedWorkload_Array(benchmark::State& state) {
    auto book = create_order_book(OrderBookType::ARRAY);
    uint64_t order_id = 1;

    // Pre-populate
    for (int i = 0; i < 500; ++i) {
        Order bid(order_id++, OrderSide::BUY, OrderType::LIMIT, 9900 + i, 100, Timer::now_ns());
        book->add_order(bid);
        Order ask(order_id++, OrderSide::SELL, OrderType::LIMIT, 10100 - i, 100, Timer::now_ns());
        book->add_order(ask);
    }

    for (auto _ : state) {
        Order buy(order_id++, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        book->add_order(buy);

        Order sell(order_id++, OrderSide::SELL, OrderType::LIMIT, 10000, 100, Timer::now_ns());
        auto result = book->match_order(sell);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_MixedWorkload_Array);
