#include "order_book.hpp"
#include "matching_engine.hpp"
#include <iostream>
#include <cassert>

using namespace hft;

void test_basic_add_order() {
    std::cout << "Testing basic add order... ";

    auto book = create_order_book(OrderBookType::MAP);
    Order order(1, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());

    auto result = book->add_order(order);
    assert(result.success);
    assert(result.order_id == 1);

    auto best_bid = book->best_bid();
    assert(best_bid.has_value());
    assert(*best_bid == 10000);

    std::cout << "PASSED\n";
}

void test_price_time_priority() {
    std::cout << "Testing price-time priority... ";

    auto book = create_order_book(OrderBookType::MAP);

    // Add orders at same price
    Order order1(1, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
    Order order2(2, OrderSide::BUY, OrderType::LIMIT, 10000, 200, Timer::now_ns());
    Order order3(3, OrderSide::BUY, OrderType::LIMIT, 10000, 150, Timer::now_ns());

    book->add_order(order1);
    book->add_order(order2);
    book->add_order(order3);

    // Add a sell order that should match with order1 first (FIFO)
    Order sell(4, OrderSide::SELL, OrderType::LIMIT, 10000, 50, Timer::now_ns());
    auto match = book->match_order(sell);

    assert(match.has_value());
    assert(match->matched_order_id == 1); // Should match order1 (first in queue)
    assert(match->quantity_filled == 50);

    std::cout << "PASSED\n";
}

void test_best_bid_ask() {
    std::cout << "Testing best bid/ask tracking... ";

    auto book = create_order_book(OrderBookType::ARRAY);

    Order bid1(1, OrderSide::BUY, OrderType::LIMIT, 9900, 100, Timer::now_ns());
    Order bid2(2, OrderSide::BUY, OrderType::LIMIT, 9950, 100, Timer::now_ns());
    Order ask1(3, OrderSide::SELL, OrderType::LIMIT, 10100, 100, Timer::now_ns());
    Order ask2(4, OrderSide::SELL, OrderType::LIMIT, 10050, 100, Timer::now_ns());

    book->add_order(bid1);
    book->add_order(bid2);
    book->add_order(ask1);
    book->add_order(ask2);

    auto best_bid = book->best_bid();
    auto best_ask = book->best_ask();

    assert(best_bid.has_value());
    assert(best_ask.has_value());
    assert(*best_bid == 9950);  // Highest bid
    assert(*best_ask == 10050); // Lowest ask

    std::cout << "PASSED\n";
}

void test_cancel_order() {
    std::cout << "Testing order cancellation... ";

    auto book = create_order_book(OrderBookType::HASH);

    Order order(1, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
    book->add_order(order);

    assert(book->best_bid().has_value());
    assert(book->cancel_order(1));
    assert(!book->best_bid().has_value()); // Book should be empty

    std::cout << "PASSED\n";
}

void test_matching() {
    std::cout << "Testing order matching... ";

    auto book = create_order_book(OrderBookType::MAP);

    // Add a sell order
    Order sell(1, OrderSide::SELL, OrderType::LIMIT, 10000, 100, Timer::now_ns());
    book->add_order(sell);

    // Add a buy order that crosses
    Order buy(2, OrderSide::BUY, OrderType::LIMIT, 10000, 50, Timer::now_ns());
    auto match = book->match_order(buy);

    assert(match.has_value());
    assert(match->matched_order_id == 1);
    assert(match->quantity_filled == 50);
    assert(match->price == 10000);

    // The sell order should still have 50 remaining
    auto best_ask = book->best_ask();
    assert(best_ask.has_value());
    assert(*best_ask == 10000);

    std::cout << "PASSED\n";
}

void test_all_implementations() {
    std::cout << "Testing all three implementations... ";

    OrderBookType types[] = {OrderBookType::MAP, OrderBookType::HASH, OrderBookType::ARRAY};

    for (auto type : types) {
        auto book = create_order_book(type);

        // Add some orders
        Order bid(1, OrderSide::BUY, OrderType::LIMIT, 9900, 100, Timer::now_ns());
        Order ask(2, OrderSide::SELL, OrderType::LIMIT, 10100, 100, Timer::now_ns());

        book->add_order(bid);
        book->add_order(ask);

        assert(book->best_bid().has_value());
        assert(book->best_ask().has_value());
        assert(*book->best_bid() == 9900);
        assert(*book->best_ask() == 10100);

        // Test matching
        Order crossing_buy(3, OrderSide::BUY, OrderType::LIMIT, 10100, 50, Timer::now_ns());
        auto match = book->match_order(crossing_buy);

        assert(match.has_value());
        assert(match->matched_order_id == 2);
        assert(match->quantity_filled == 50);
    }

    std::cout << "PASSED\n";
}

void test_matching_engine() {
    std::cout << "Testing matching engine... ";

    MatchingEngine engine(OrderBookType::MAP);

    Order order1(1, OrderSide::BUY, OrderType::LIMIT, 10000, 100, Timer::now_ns());
    Order order2(2, OrderSide::SELL, OrderType::LIMIT, 10000, 100, Timer::now_ns());

    engine.process_order(order1);
    engine.process_order(order2);

    const auto& stats = engine.stats();
    assert(stats.total_orders_added >= 1);
    assert(stats.total_orders_matched >= 1);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "Running HFT Order Book Tests\n";
    std::cout << "============================\n\n";

    test_basic_add_order();
    test_price_time_priority();
    test_best_bid_ask();
    test_cancel_order();
    test_matching();
    test_all_implementations();
    test_matching_engine();

    std::cout << "\nAll tests PASSED!\n";
    return 0;
}
