#ifndef INCLUDED_ORDERBOOK_HPP
#define INCLUDED_ORDERBOOK_HPP

#include "async.hpp"
#include "concepts.hpp"
#include "enums.hpp"
#include "generator.hpp"
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


    template<
        OrderConcept _OrderType = Order<>,
        typename OrderBookSidePolicy = PriceLevelStackBookSidePolicy<>
        >
    class OrderBook
    {
    public:
        typedef _OrderType OrderType;
        template<Side MySide, OrderConcept OrderType>
        using OrderBookSideType = typename OrderBookSidePolicy::OrderBookSideType<MySide, OrderType>;
    

        template<typename ExecutionPolicy = util::AsyncNoop>
        util::Generator<OrderQuantity<OrderType>>
        accept_order(OrderType &order, ExecutionPolicy &&execution_policy = {})
        {
            if (order.side == Side::Buy)
            {
                return std::move(
                    do_accept_order(order, m_ask, m_bid,
                                    std::forward<ExecutionPolicy>(execution_policy)));
            }
            else
            {
                return std::move(
                    do_accept_order(order, m_bid, m_ask,
                                    std::forward<ExecutionPolicy>(execution_policy)));
            }
        }

        const auto &bid() const { return m_bid; }
        const auto &ask() const { return m_ask; }

    private:
        OrderBookSideType<Side::Buy, OrderType> m_bid;
        OrderBookSideType<Side::Sell, OrderType> m_ask;

        template <
            OrderBookSideConcept MatchSideType,
            OrderBookSideConcept AddSideType,
            typename ExecutionPolicy
        >
        util::Generator<OrderQuantity<OrderType>>
        do_accept_order(
            OrderType &order,
            MatchSideType &match_side,
            AddSideType &add_side,
            ExecutionPolicy &&execution_policy)
        {
            typename OrderType::QuantityType matched_quantity = 0;

            auto executions = match_side.match_order(
                order, std::forward<ExecutionPolicy>(execution_policy));

            while (executions)
            {
                auto executed = executions();
                co_yield executed;
                matched_quantity += quantity_of(executed);
            }

            if (order.order_type == orderbook::OrderType::Limit)
            {
                auto quantity_remaining = (order.quantity - matched_quantity);
                if (quantity_remaining)
                {
                    add_side.add_order(order, quantity_remaining);
                }
            }

            co_return;
        }
    };

} // end of namespace sadhbhcraft::orderbook
#endif//INCLUDED_ORDERBOOK_HPP