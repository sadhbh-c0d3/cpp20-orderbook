#include <iostream>
#include <cassert>
#include <memory>

#include "lib.hpp"


int main(int argc, const char** argv)
{
    namespace scob = sadhbhcraft::orderbook;
    namespace scu = sadhbhcraft::util;

    scob::OrderBook book;

    // 1. When book is empty we expect an order to be added to correct side at
    // new level
    // Add first order
    scob::Order o1{
        .side = scob::Side::Buy,
        .order_type = scob::OrderType::Limit,
        .price = 100,
        .quantity = 5
    };

    assert(!book.accept_order(o1));

    // Expect: 1-level, 1-element queue
    assert(book.bid().size() == 1);
    assert(book.bid().top().size() == 1);
    assert(std::addressof(book.bid().top().first().order()) == std::addressof(o1));

    // 2. When book contains one level with one order, new order at same price
    // will join at the end of the queue
    // Add second order, same price
    scob::Order o2{
        .side = scob::Side::Buy,
        .order_type = scob::OrderType::Limit,
        .price = 100,
        .quantity = 10
    };

    assert(!book.accept_order(o2));

    // Expect: 1-level, 2-element queue
    assert(book.bid().size() == 1);
    assert(book.bid().top().size() == 2);
    assert(std::addressof(book.bid().top().first().order()) == std::addressof(o1));
    assert(std::addressof(std::next(book.bid().top().begin())->order()) == std::addressof(o2));

    // 3. When book contains one level, new order at worse price will be added
    // to a new deeper level
    // Add third order, lower price
    scob::Order o3{
        .side = scob::Side::Buy,
        .order_type = scob::OrderType::Limit,
        .price = 90,
        .quantity = 5
    };

    assert(!book.accept_order(o3));

    // Expect: 2-levels
    assert(book.bid().size() == 2);
    // Expect: 2-el in 1st level queue
    assert(book.bid().top().size() == 2);
    assert(std::addressof(book.bid().top().first().order()) == std::addressof(o1));
    assert(std::addressof(std::next(book.bid().top().begin())->order()) == std::addressof(o2));
    // Expect: 1-el in 2nd level queue
    assert(std::next(book.bid().begin())->size() == 1);
    assert(std::addressof(std::next(book.bid().begin())->first().order()) == std::addressof(o3));

    // 4. When book contains two levels, new order with price in between the two
    // levels will be added to a new level in between
    // Add fourth order, middle price
    scob::Order o4{
        .side = scob::Side::Buy,
        .order_type = scob::OrderType::Limit,
        .price = 95,
        .quantity = 10
    };

    assert(!book.accept_order(o4));

    // Expect: 3-levels
    assert(book.bid().size() == 3);
    // Expect: 2-el in 1st level queue
    assert(book.bid().top().size() == 2);
    assert(std::addressof(book.bid().top().first().order()) == std::addressof(o1));
    assert(std::addressof(std::next(book.bid().top().begin())->order()) == std::addressof(o2));
    // Expect: 1-el in 2nd level queue
    assert(std::next(book.bid().begin())->size() == 1);
    assert(std::addressof(std::next(book.bid().begin())->first().order()) == std::addressof(o4));
    // Expect: 1-el in 3nd level queue
    assert(std::next(book.bid().begin(), 2)->size() == 1);
    assert(std::addressof(std::next(book.bid().begin(), 2)->first().order()) == std::addressof(o3));
    
    // 5. When book contains any levels, new order with new best price will be
    // added to new level at the top of the book
    // Add 5th order, best bid
    scob::Order o5{
        .side = scob::Side::Buy,
        .order_type = scob::OrderType::Limit,
        .price = 105,
        .quantity = 2
    };

    // 6. When book contains bids, and no asks, then new sell order will be
    // added to ask side, and bid side won't be altered
    // Add 6th order, best ask
    scob::Order o6{
        .side = scob::Side::Sell,
        .order_type = scob::OrderType::Limit,
        .price = 120,
        .quantity = 7
    };

    assert(!book.accept_order(o5));
    assert(!book.accept_order(o6));
    
    // Expect: 1-level at Ask
    assert(book.ask().size() == 1);
    // Expect: 1-el in 1st level queue
    assert(book.ask().top().size() == 1);
    assert(std::addressof(book.ask().top().first().order()) == std::addressof(o6));

    // Expect: 4-levels at Bid
    assert(book.bid().size() == 4);
    // Expect: 1-el in 1st level queue
    assert(book.bid().top().size() == 1);
    assert(std::addressof(book.bid().top().first().order()) == std::addressof(o5));
    // Expect: 2-el in 2nd level queue
    assert(std::next(book.bid().begin())->size() == 2);
    assert(std::addressof(std::next(book.bid().begin())->first().order()) == std::addressof(o1));
    assert(std::addressof(std::next(std::next(book.bid().begin())->begin())->order()) == std::addressof(o2));
    // Expect: 1-el in 3rd level queue
    assert(std::next(book.bid().begin(), 2)->size() == 1);
    assert(std::addressof(std::next(book.bid().begin(), 2)->first().order()) == std::addressof(o4));
    // Expect: 1-el in 4th level queue
    assert(std::next(book.bid().begin(), 3)->size() == 1);
    assert(std::addressof(std::next(book.bid().begin(), 3)->first().order()) == std::addressof(o3));
    
    // 7. When book contains bids and asks, new Sell order with worse price will
    // be added to new deeper level on the ask side, and number of bid levels won't be altered
    // NOTE: We have already checked for bids that insertion at different price
    // points works correctly, and the purpose of this test is to confirm that
    // ask side has opposite sorting direction than bid side
    // Add 7th order, 2nd level ask
    scob::Order o7{
        .side = scob::Side::Sell,
        .order_type = scob::OrderType::Limit,
        .price = 125,
        .quantity = 4
    };

    assert(!book.accept_order(o7));

    // Expect: 2-levels at Ask
    assert(book.ask().size() == 2);
    // Expect: 1-el in 1st level queue
    assert(book.ask().top().size() == 1);
    assert(std::addressof(book.ask().top().first().order()) == std::addressof(o6));
    // Expect: 1-el in 2nd level queue
    assert(std::next(book.ask().begin())->size() == 1);
    assert(std::addressof(std::next(book.ask().begin())->first().order()) == std::addressof(o7));


    // // 8. We send IOC at price 100 to swipe quantity of 8
    // // At this stage we have orders:
    // // Bid: [(105,2), (100, 5), (100, 10), (95,10), (90,5)]
    // // Ask: [(125,4), (120,7)]
    // // So we expect executions: [(105,2), (100,5), (100,1)]
    // // And then we expect Bid: [(100,9), (95,10), (90,5)]
    // scob::Order o8{
    //     .side = scob::Side::Sell,
    //     .order_type = scob::OrderType::IOC,
    //     .price = 100,
    //     .quantity = 8
    // };

    // auto ex8 = book.accept_order(o8);

    // assert(ex8);
    // // Should execute (105, 2)
    // auto ex = ex8();
    // assert(quantity_of(ex) == 2);
    // assert(std::addressof(ex.order()) == std::addressof(o5));
    // // Should execute (100, 5)
    // ex = ex8();
    // assert(quantity_of(ex) == 5);
    // assert(std::addressof(ex.order()) == std::addressof(o1));
    // // Should execute 1 from (100, 10)
    // ex = ex8();
    // assert(quantity_of(ex) == 1);
    // assert(std::addressof(ex.order()) == std::addressof(o2));
    // // No more executions
    // assert(!ex8);

    // // Expect: 3-levels at Bid
    // // Expect: 1-el in 1st level queue
    // assert(book.bid().size() == 3);
    // assert(book.bid().top().size() == 1);
    // assert(std::addressof(book.bid().top().first().order()) == std::addressof(o2));
    // assert(quantity_of(book.bid().top().first()) == 9);


    // // 9. We send IOC at price 95 to swipe quantity of 19
    // // At this stage we have orders:
    // // Bid: [(100, 9), (95,10), (90,5)]
    // // Ask: [(125,4), (120,7)]
    // // So we expect executions: [(100,9), (95,10)]
    // // And then we expect Bid: [(90,5)]
    // scob::Order o9{
    //     .side = scob::Side::Sell,
    //     .order_type = scob::OrderType::IOC,
    //     .price = 95,
    //     .quantity = 19
    // };
    
    // auto ex9 = book.accept_order(o9);

    // assert(ex9);
    // // Should execute (100, 9)
    // ex = ex9();
    // assert(quantity_of(ex) == 9);
    // assert(std::addressof(ex.order()) == std::addressof(o2));
    // // Should execute (95, 10)
    // ex = ex9();
    // assert(quantity_of(ex) == 10);
    // assert(std::addressof(ex.order()) == std::addressof(o4));
    // // No more executions
    // assert(!ex9);
    
    // assert(book.bid().size() == 1);
    // assert(book.bid().top().size() == 1);
    // assert(std::addressof(book.bid().top().first().order()) == std::addressof(o3));
    // assert(quantity_of(book.bid().top().first()) == quantity_of(o3));

    // std::cout << "Tests OK." << std::endl;
    // return 0;
    

    // // 10. We send IOC at price 125 to swipe quantity of 10
    // // with OrderSizeLimit of 5
    // // At this stage we have orders:
    // // Bid: [(90,5)]
    // // Ask: [(125,4), (120,7)]
    // // So we expect executions: [(125,4), (120,5)]
    // // And then we expect Ask: []
    // scob::Order o10{
    //     .side = scob::Side::Buy,
    //     .order_type = scob::OrderType::IOC,
    //     .price = 125,
    //     .quantity = 10
    // };
    
    // using LimitType = scu::AsyncImmediate<scob::OrderSizeLimit<scob::Order<>>>;
    // auto ex10 = book.accept_order<LimitType>(o10, {5});
    
    // assert(ex10);
    // // Should execute (125, 4)
    // ex = ex10();
    // assert(quantity_of(ex) == 4);
    // assert(std::addressof(ex.order()) == std::addressof(o7));
    // // Should execute (120, 5)
    // ex = ex10();
    // assert(quantity_of(ex) == 5);
    // assert(std::addressof(ex.order()) == std::addressof(o6));
    // // No more executions
    // assert(!ex10);
    // // Ask should be clear, as we cancel anything that cannot be executed
    // assert(book.ask().empty());
    
}
