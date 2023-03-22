#ifndef INCLUDED_ORDERBOOK_HPP
#define INCLUDED_ORDERBOOK_HPP

#include "enums.hpp"
#include "concepts.hpp"
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


    template<OrderConcept _OrderType = Order<>, typename OrderBookSidePolicy = PriceLevelStackBookSidePolicy<>>
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
                do_accept_order(order, m_ask, m_bid);
            }
            else
            {
                do_accept_order(order,m_bid, m_ask);
            }
        }

        const auto &bid() const { return m_bid; }
        const auto &ask() const { return m_ask; }

    private:
        OrderBookSideType<Side::Buy, OrderType> m_bid;
        OrderBookSideType<Side::Sell, OrderType> m_ask;

        template <
            OrderBookSideConcept MatchSideType,
            OrderBookSideConcept AddSideType>
        void do_accept_order(OrderType &order, MatchSideType &match_side, AddSideType &add_side)
        {
            auto matched_quantity = match_side.match_order(order);
            auto quantity_remaining = order.quantity - matched_quantity;

            if (quantity_remaining && (order.order_type == sadhbhcraft::orderbook::OrderType::Limit))
            {
                add_side.add_order(order, quantity_remaining);
            }
        }
    };

} // end of namespace sadhbhcraft::orderbook
#endif//INCLUDED_ORDERBOOK_HPP