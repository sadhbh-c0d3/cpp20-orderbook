#include "lib.hpp"

#include <iostream>
#include <sstream>


namespace scob = sadhbhcraft::orderbook;

struct MyOrder : scob::Order<>
{
    int userid;
};

using MyOrderBook = scob::OrderBook<MyOrder>;


int main(int argc, const char **argv)
{
    MyOrderBook book;
    MyOrder orders[] = {
        {scob::Side::Buy, scob::OrderType::Limit, 100, 5, 1},
        {scob::Side::Sell, scob::OrderType::IOC, 95, 10, 2}
    };

    for (auto &order : orders)
    {
        std::cout << "User " << order.userid << " wants to "
                << (order.side == scob::Side::Buy ? "Buy " : "Sell ")
                << (order.order_type == scob::OrderType::Limit ? "" : "immediately ")
                << scob::quantity_of(order)
                << " at price of: " << scob::price_of(order)
                << std::endl;

        for (auto next_execution = book.accept_order(order); next_execution;)
        {
            auto execution = next_execution();

            std::cout << "Matched order of user " << order.userid << " and user " << execution.order().userid
                << ", and executed " << scob::quantity_of(execution)
                << " at price of: " << scob::price_of(execution)
                << std::endl;
        }
    }

    return 0;
}
