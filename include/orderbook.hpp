#ifndef INCLUDED_ORDERBOOK_HPP
#define INCLUDED_ORDERBOOK_HPP

#include "enums.hpp"
#include "pricelevelstack.hpp"


namespace sadhbhcraft::orderbook
{
    template<NumericType _PriceType = int, NumericType _QuantityType = int>
    struct Order
    {
        typedef _PriceType PriceType;
        typedef _QuantityType QuantityType;

        Side side;
        OrderType order_type;
        PriceType price;
        QuantityType quantity;
    };


    template<OrderConcept _OrderType = Order<>, typename OrderBookSidePolicy = StackOfOrderQueuesBookSidePolicy<>>
    class OrderBook
    {
    public:
        typedef _OrderType OrderType;
        template<Side MySide, OrderConcept OrderType>
        using OrderBookSideType = typename OrderBookSidePolicy::OrderBookSideType<MySide, OrderType>;
    

        void accept_order(OrderType &order)
        {
            if (order.side == Side::Buy)
            {
                do_accept_order(order, m_bid);
            }
            else
            {
                do_accept_order(order, m_ask);
            }
        }

        const auto &bid() const { return m_bid; }
        const auto &ask() const { return m_ask; }

    private:
        OrderBookSideType<Side::Buy, OrderType> m_bid;
        OrderBookSideType<Side::Sell, OrderType> m_ask;

        template<OrderBookSideConcept SideType>
        void do_accept_order(OrderType &order, SideType &side)
        {
            // ... now we can write more code handling order
            // and we won't be repeating that code
            side.add_order(order);
        }
    };

} // end of namespace sadhbhcraft::orderbook
#endif//INCLUDED_ORDERBOOK_HPP