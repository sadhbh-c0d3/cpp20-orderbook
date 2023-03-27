#ifndef INCLUDED_ORDERBOOK_HPP
#define INCLUDED_ORDERBOOK_HPP

#include "enums.hpp"
#include "concepts.hpp"
#include "pricelevelstack.hpp"
#include "util/async.hpp"
#include "util/generator.hpp"


namespace sadhbhcraft::orderbook
{
    template<util::NumberConcept _PriceType = int, util::NumberConcept _QuantityType = int>
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
        using BidBookSideType = OrderBookSideType<Side::Buy, OrderType>;
        using AskBookSideType = OrderBookSideType<Side::Sell, OrderType>;
    

        template<ExecutionPolicyConcept<OrderQuantity<OrderType>> ExecutionPolicy = util::AsyncNoop>
        util::Generator<OrderQuantity<OrderType>>
        accept_order(OrderType &order, ExecutionPolicy &&execution_policy = {})
        {
            if (order.side == Side::Buy)
            {
                return do_accept_order(order, m_ask, m_bid, std::forward<ExecutionPolicy>(execution_policy));
            }
            else
            {
                return do_accept_order(order, m_bid, m_ask, std::forward<ExecutionPolicy>(execution_policy));
            }
        }

        const auto &bid() const { return m_bid; }
        const auto &ask() const { return m_ask; }

    private:
        BidBookSideType m_bid;
        AskBookSideType m_ask;

        template <
            OrderBookSideConcept MatchSideType,
            OrderBookSideConcept AddSideType,
            ExecutionPolicyConcept<OrderQuantity<OrderType>> ExecutionPolicy>
        util::Generator<OrderQuantity<OrderType>>
        do_accept_order(
            OrderType &order,
            MatchSideType &match_side,
            AddSideType &add_side,
            ExecutionPolicy &&execution_policy)
        {
            auto executions = match_side.match_order(order, std::forward<ExecutionPolicy>(execution_policy));
            typename OrderType::QuantityType matched_quantity = 0;
            while (executions)
            {
                auto executed = executions();
                co_yield executed;
                matched_quantity += quantity_of(executed);
            }
            auto quantity_remaining = order.quantity - matched_quantity;

            if (quantity_remaining && (order.order_type == orderbook::OrderType::Limit))
            {
                add_side.add_order(order, quantity_remaining);
            }

            co_return;
        }
    };

} // end of namespace sadhbhcraft::orderbook
#endif//INCLUDED_ORDERBOOK_HPP