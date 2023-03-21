#include <iostream>
#include <cassert>

#include "lib.hpp"


int main(int argc, const char** argv)
{
    namespace scob = sadhbhcraft::orderbook;

    scob::Instrument i1{.symbol = "AAA"};
    scob::Instrument i2{.symbol = "ABB"};

    scob::Market m1{.main=i1, .traded=i2};
    scob::OrderBook book;

    // 1. When book is empty we expect an order to be added to correct side at
    // new level
    // Add first order
    scob::Order o1{
        .market = m1,
        .side = scob::Side::Buy,
        .order_type = scob::OrderType::Limit,
        .price = 100,
        .quantity = 5
    };

    book.accept_order(o1);

    // Expect: 1-level, 1-element queue
    assert(book.bid().size() == 1);
    assert(book.bid().top().size() == 1);
    assert(&(book.bid().top().first().order) == &o1);

    // 2. When book contains one level with one order, new order at same price
    // will join at the end of the queue
    // Add second order, same price
    scob::Order o2{
        .market = m1,
        .side = scob::Side::Buy,
        .order_type = scob::OrderType::Limit,
        .price = 100,
        .quantity = 10
    };

    book.accept_order(o2);

    // Expect: 1-level, 2-element queue
    assert(book.bid().size() == 1);
    assert(book.bid().top().size() == 2);
    assert(&(book.bid().top().first().order) == &o1);
    assert(&(std::next(book.bid().top().begin())->order) == &o2);

    // 3. When book contains one level, new order at worse price will be added
    // to a new deeper level
    // Add third order, lower price
    scob::Order o3{
        .market = m1,
        .side = scob::Side::Buy,
        .order_type = scob::OrderType::Limit,
        .price = 90,
        .quantity = 5
    };

    book.accept_order(o3);

    // Expect: 2-levels
    assert(book.bid().size() == 2);
    // Expect: 2-el in 1st level queue
    assert(book.bid().top().size() == 2);
    assert(&(book.bid().top().first().order) == &o1);
    assert(&(std::next(book.bid().top().begin())->order) == &o2);
    // Expect: 1-el in 2nd level queue
    assert(std::next(book.bid().begin())->size() == 1);
    assert(&(std::next(book.bid().begin())->first().order) == &o3);

    // 4. When book contains two levels, new order with price in between the two
    // levels will be added to a new level in between
    // Add fourth order, middle price
    scob::Order o4{
        .market = m1,
        .side = scob::Side::Buy,
        .order_type = scob::OrderType::Limit,
        .price = 95,
        .quantity = 10
    };

    book.accept_order(o4);

    // Expect: 3-levels
    assert(book.bid().size() == 3);
    // Expect: 2-el in 1st level queue
    assert(book.bid().top().size() == 2);
    assert(&(book.bid().top().first().order) == &o1);
    assert(&(std::next(book.bid().top().begin())->order) == &o2);
    // Expect: 1-el in 2nd level queue
    assert(std::next(book.bid().begin())->size() == 1);
    assert(&(std::next(book.bid().begin())->first().order) == &o4);
    // Expect: 1-el in 3nd level queue
    assert(std::next(book.bid().begin(), 2)->size() == 1);
    assert(&(std::next(book.bid().begin(), 2)->first().order) == &o3);
    
    // 5. When book contains any levels, new order with new best price will be
    // added to new level at the top of the book
    // Add 5th order, best bid
    scob::Order o5{
        .market = m1,
        .side = scob::Side::Buy,
        .order_type = scob::OrderType::Limit,
        .price = 105,
        .quantity = 2
    };

    // 6. When book contains bids, and no asks, then new sell order will be
    // added to ask side, and bid side won't be altered
    // Add 6th order, best ask
    scob::Order o6{
        .market = m1,
        .side = scob::Side::Sell,
        .order_type = scob::OrderType::Limit,
        .price = 120,
        .quantity = 7
    };

    book.accept_order(o5);
    book.accept_order(o6);
    
    // Expect: 1-level at Ask
    assert(book.ask().size() == 1);
    // Expect: 1-el in 1st level queue
    assert(book.ask().top().size() == 1);
    assert(&(book.ask().top().first().order) == &o6);

    // Expect: 4-levels at Bid
    assert(book.bid().size() == 4);
    // Expect: 1-el in 1st level queue
    assert(book.bid().top().size() == 1);
    assert(&(book.bid().top().first().order) == &o5);
    // Expect: 2-el in 2nd level queue
    assert(std::next(book.bid().begin())->size() == 2);
    assert(&(std::next(book.bid().begin())->first().order) == &o1);
    assert(&(std::next(std::next(book.bid().begin())->begin())->order) == &o2);
    // Expect: 1-el in 3rd level queue
    assert(std::next(book.bid().begin(), 2)->size() == 1);
    assert(&(std::next(book.bid().begin(), 2)->first().order) == &o4);
    // Expect: 1-el in 4th level queue
    assert(std::next(book.bid().begin(), 3)->size() == 1);
    assert(&(std::next(book.bid().begin(), 3)->first().order) == &o3);
    
    // 7. When book contains bids and asks, new Sell order with worse price will
    // be added to new deeper level on the ask side, and number of bid levels won't be altered
    // NOTE: We have already checked for bids that insertion at different price
    // points works correctly, and the purpose of this test is to confirm that
    // ask side has opposite sorting direction than bid side
    // Add 7th order, 2nd level ask
    scob::Order o7{
        .market = m1,
        .side = scob::Side::Sell,
        .order_type = scob::OrderType::Limit,
        .price = 125,
        .quantity = 4
    };

    book.accept_order(o7);

    // Expect: 2-levels at Ask
    assert(book.ask().size() == 2);
    // Expect: 1-el in 1st level queue
    assert(book.ask().top().size() == 1);
    assert(&(book.ask().top().first().order) == &o6);
    // Expect: 1-el in 2nd level queue
    assert(std::next(book.ask().begin())->size() == 1);
    assert(&(std::next(book.ask().begin())->first().order) == &o7);


    std::cout << "Tests OK." << std::endl;
    return 0;
}
