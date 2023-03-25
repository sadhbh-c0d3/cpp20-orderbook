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
    

        bool accept_order(OrderType &order)
        {
            if (order.side == Side::Buy)
            {
                do_accept_order(order, m_ask, m_bid);
            }
            else
            {
                do_accept_order(order, m_bid, m_ask);
            }
            return false;
        }

        //template<typename ExecutionPolicy = util::AsyncNoop>
        //util::Generator<OrderQuantity<OrderType>>
        //accept_order(OrderType &order, ExecutionPolicy &execution_policy)
        //{
        //    if (order.side == Side::Buy)
        //    {
        //        auto gen = do_accept_order(order, m_ask, m_bid, execution_policy);
        //        for (auto x : gen)
        //        //while (gen)
        //        {
        //            //co_yield gen();
        //            co_yield x;
        //        }
        //    }
        //    else
        //    {
        //        auto gen = do_accept_order(order, m_bid, m_ask, execution_policy);
        //        for (auto x : gen)
        //        //while (gen)
        //        {
        //            //co_yield gen();
        //            co_yield x;
        //        }
        //    }
        //    co_return;
        //}

        const auto &bid() const { return m_bid; }
        const auto &ask() const { return m_ask; }

    private:
        OrderBookSideType<Side::Buy, OrderType> m_bid;
        OrderBookSideType<Side::Sell, OrderType> m_ask;

        template <
            OrderBookSideConcept MatchSideType,
            OrderBookSideConcept AddSideType>
            //typename ExecutionPolicy>
        //util::Generator<OrderQuantity<OrderType>>
        //std::vector<OrderQuantity<OrderType>>
        void
        do_accept_order(
            OrderType &order,
            MatchSideType &match_side,
            AddSideType &add_side)
            //ExecutionPolicy &execution_policy)
        {
            //std::vector<OrderQuantity<OrderType>> results;
            //auto executions = match_side.match_order(order, execution_policy);
            auto executions = match_side.match_order(order);
            //auto matched_quantity = match_side.match_order(order);
            typename OrderType::QuantityType matched_quantity = 0;
            for (auto executed : executions)
            //while (executions)
            {
                //auto executed = executions();
                //co_yield executed;
                //results.push_back(executed);
                matched_quantity += quantity_of(executed);
            }
            auto quantity_remaining = order.quantity - matched_quantity;

            if (quantity_remaining && (order.order_type == orderbook::OrderType::Limit))
            {
                add_side.add_order(order, quantity_remaining);
            }

            //co_return;
            //return results;
        }
    };

} // end of namespace sadhbhcraft::orderbook
#endif//INCLUDED_ORDERBOOK_HPP