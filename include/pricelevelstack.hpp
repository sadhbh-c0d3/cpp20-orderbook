#ifndef INCLUDED_PRICELEVELSTACK_HPP
#define INCLUDED_PRICELEVELSTACK_HPP

#include "enums.hpp"
#include "concepts.hpp"
#include "traits.hpp"

#include<vector>
#include<deque>
#include<algorithm>


namespace sadhbhcraft::orderbook
{
    template<OrderConcept _OrderType>
    struct OrderQuantity
    {
        typedef _OrderType OrderType;
        typedef typename _OrderType::QuantityType QuantityType;

        OrderQuantity(OrderType &order, QuantityType quantity): order(order), quantity(quantity) {}
        OrderQuantity(OrderQuantity &&) = default;
        OrderQuantity& operator = (OrderQuantity &&x)
        {
            // Tell C++ to "steal" `order` pointer from `x`. This will actually copy that pointer.
            // This is legal, because `&order` points to some stable memory block is scope far outside.
            // Remember that `OrderType &` is stored as `OrderType *`, and we can simply copy & assign it.
            // The only reason why we can't assign it using `=` is C++ rules, which protect `&` references.
            // The rules exist generally to protect expiring objects to be referenced, and all fields are
            // treated as expiring references, so if a field is of type reference, then it becomes expiring
            // even though in fact it is a reference to outside world, that is no expiring anywhere.
            //
            // DISCUSSION
            // Is it a good programming practice? Probably not :D
            // Good question is whether this is maybe suggestion to use `shared_ptr<OrderType>` instead.
            // We don't necessarily want to manage lifetime of the `&order`, and raw reference to something
            // in the outside scope should be sufficient. Cheaper, and there is no need for it (yet).
            // NOTE: this struct is only an internal detail of the Price-Level-Stack OrderBook Side, and
            // we don't expose it anywhere outside. We require that user owns the orders they pass to us.
            // At this stage we implement OrderBook as pure matching engine and not order management component.
            return (*new (this) OrderQuantity(std::move(x)));
        } 

        OrderType &order;
        QuantityType quantity;
    };

    template<OrderConcept _OrderType, template <typename> class _QueueType>
    class OrderPriceLevel
    {
    public:
        typedef _OrderType OrderType;
        typedef typename _OrderType::PriceType PriceType;
        typedef typename _OrderType::QuantityType QuantityType;
        template<typename T> using QueueType = _QueueType<T>;

        OrderPriceLevel(PriceType price): m_price(price), m_total_quantity(0)
        {}

        void add_order(OrderType &order, QuantityType quantity)
        {
            m_orders.emplace_back(order, quantity);

            m_total_quantity += quantity;
        }
        
        QuantityType match_order(OrderType &order, QuantityType quantity)
        {
            QuantityType quantity_filled = 0;
        
            auto it = m_orders.begin();
            auto last_filled = it;
            auto last_filled_quantity = quantity;

            for (; it != m_orders.end(); ++it)
            {
                if (quantity < it->quantity)
                {
                    // Fully filled requested quantity
                    quantity_filled += quantity;
                    it->quantity -= quantity;
                    m_total_quantity -= quantity;
                    last_filled_quantity = quantity;
                    quantity = 0;
                    break;
                }

                // Fully filled order in the queue
                last_filled = it;
                last_filled_quantity = it->quantity;
                
                // update this level
                it->quantity = 0;
                m_total_quantity -= last_filled_quantity;

                // fully filled at `it`
                quantity_filled += last_filled_quantity;
                // more to go
                quantity -= last_filled_quantity;
            }

            m_orders.erase(m_orders.begin(), last_filled);

            return quantity_filled;
        }

        auto price() const { return m_price; }
        auto total_quantity() const { return m_total_quantity; }

        auto begin() const { return m_orders.begin(); }
        auto end() const { return m_orders.end(); }

        const auto &first() const { return m_orders.front(); }

        size_t size() const { return m_orders.size(); }
        bool empty() const { return m_orders.empty(); }

    private:
        QueueType<OrderQuantity<OrderType>> m_orders;
        PriceType m_price;
        QuantityType m_total_quantity;
    };

    template<OrderConcept OrderType, template <typename> class QueueType>
    struct PriceTrait<OrderPriceLevel<OrderType, QueueType>, typename OrderType::PriceType>
    {
        static auto price(const OrderPriceLevel<OrderType, QueueType> &opl) { return opl.price(); }
    };

    template<Side MySide>
    struct PriceLevelCompare
    {
        template<typename A, typename B>
        bool operator()(const A &a, const B &b)
        {
            if constexpr (MySide == Side::Buy)
            {
                return price_of(b) < price_of(a);
            }
            else
            {
                return price_of(a) < price_of(b);
            }
        }
    };
    
    template<Side MySide, OrderConcept _OrderType,
        template <typename> class _StackType,
        template <typename> class _QueueType>
    class StackOfOrderQueuesBookSide
    {
    public:
        typedef _OrderType OrderType;
        typedef typename OrderType::QuantityType QuantityType;
        template<typename T> using StackType = _StackType<T>;
        template<typename T> using QueueType = _QueueType<T>;

        void add_order(OrderType &order, QuantityType quantity)
        {
            do_add_order(order, quantity);
        }

        QuantityType match_order(OrderType &order)
        {
            QuantityType quantity_filled = 0;
            PriceLevelCompare<MySide> price_compare;

            auto it = m_levels.begin();
            auto last_filled = it; //< it's actually one after last filled
            auto last_quantity_filled = quantity_filled;
            
            if (price_compare(*it, order))
            {
                for (; it != m_levels.end(); ++it)
                {
                    if (quantity_of(order) == quantity_filled)
                    {
                        last_filled = it;
                        break; //< Order was fully filled
                    }
                    else if (price_compare(order, *it))
                    {
                        break; //< Order was partially filled
                    }
                    last_quantity_filled = it->match_order(order, quantity_of(order) - quantity_filled);
                    quantity_filled += last_quantity_filled;
                    last_filled = it;
                }

                if (last_filled != it)
                {
                    // then level at `it` was partially filled
                    // and `last_quantity_filled` tells how much was filled on that level
                    // The `it->total_quantity` will tell remaining quantity on partially
                    // filled level.
                    // TODO: (1) Collect the filled orders for purpose of firing market data events
                }

                // Remove all levels that were fully filled, but keep the one
                // that still has quantity left
                // TODO: (2) Collect the filled orders for purpose of firing market data events
                //
                // The collected orders need to be tested against user trading
                // margin using some margin policy. Matched orders and filled
                // quantity on levels will remaing matched and filled, but as a
                // result of check orders can be either executed or cancelled,
                // and if any quantity is not executed, then we need to repeat
                // whole match process again. If incoming order is of FOC type,
                // then whole repetition needs to happen in reversible
                // transaction, because if FOC order is not fully filled, then
                // quantities on levels should remain unchaged. That would
                // require perhaps two pass approach.
                //
                m_levels.erase(m_levels.begin(), last_filled);
            }

            return quantity_filled;
        }

        constexpr Side side() const { return MySide; }

        auto begin() const { return m_levels.begin(); }
        auto end() const { return m_levels.end(); }

        const auto &top() const { return m_levels.front(); }

        size_t size() const { return m_levels.size(); }
        bool empty() const { return m_levels.empty(); }

    protected:
        StackType<OrderPriceLevel<OrderType, QueueType>> m_levels;
        
        auto find_or_get_insert_iterator(typename OrderType::PriceType price)
        {
            return std::lower_bound(m_levels.begin(), m_levels.end(), price, PriceLevelCompare<MySide>());
        }

        void do_add_order(OrderType &order, QuantityType quantity)
        {
            auto level_iterator = find_or_get_insert_iterator(price_of(order));

            if (level_iterator == m_levels.end() || level_iterator->price() != order.price)
            {
                level_iterator = m_levels.emplace(level_iterator, price_of(order));
            }
            level_iterator->add_order(order, quantity);
        }
    };

    template<template <typename> class StackType = std::deque, template <typename> class QueueType = std::deque>
    struct StackOfOrderQueuesBookSidePolicy
    {
        template<Side MySide, OrderConcept OrderType>
        using OrderBookSideType = StackOfOrderQueuesBookSide<MySide, OrderType, StackType, QueueType>;
    };

} // end of namespace sadhbhcraft::orderbook
#endif//INCLUDED_PRICELEVELSTACK_HPP
