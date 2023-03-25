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
    

        util::Generator<OrderQuantity<OrderType>>
        accept_order(OrderType &order)
        {
            util::AsyncNoop execution_policy;
            if (order.side == Side::Buy)
            {
                auto gen = do_accept_order(order, m_ask, m_bid, execution_policy);
                while (gen)
                {
                    co_yield gen();
                }
            }
            else
            {
                auto gen = do_accept_order(order, m_bid, m_ask, execution_policy);
                while (gen)
                {
                    co_yield gen();
                }
            }
            co_return;
        }

        template<typename ExecutionPolicy = util::AsyncNoop>
        util::Generator<OrderQuantity<OrderType>>
        accept_order(OrderType &order, ExecutionPolicy &execution_policy)
        {
            if (order.side == Side::Buy)
            {
                auto gen = do_accept_order(order, m_ask, m_bid, execution_policy);
                while (gen)
                {
                    co_yield gen();
                }
            }
            else
            {
                auto gen = do_accept_order(order, m_bid, m_ask, execution_policy);
                while (gen)
                {
                    co_yield gen();
                }
            }
            co_return;
        }

        const auto &bid() const { return m_bid; }
        const auto &ask() const { return m_ask; }

    private:
        OrderBookSideType<Side::Buy, OrderType> m_bid;
        OrderBookSideType<Side::Sell, OrderType> m_ask;

        template <
            OrderBookSideConcept MatchSideType,
            OrderBookSideConcept AddSideType,
            typename ExecutionPolicy>
        util::Generator<OrderQuantity<OrderType>>
        do_accept_order(
            OrderType &order,
            MatchSideType &match_side,
            AddSideType &add_side,
            ExecutionPolicy &execution_policy)
        {
            auto executions = match_side.match_order(order, execution_policy);
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