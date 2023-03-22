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

        void add_order(OrderType &order)
        {
            m_orders.emplace_back(order, quantity_of(order));

            m_total_quantity += quantity_of(order);
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
        template<typename T> using StackType = _StackType<T>;
        template<typename T> using QueueType = _QueueType<T>;

        void add_order(OrderType &order) { do_add_order(order); }

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

        void do_add_order(OrderType &order)
        {
            auto level_iterator = find_or_get_insert_iterator(price_of(order));

            if (level_iterator == m_levels.end() || level_iterator->price() != order.price)
            {
                level_iterator = m_levels.emplace(level_iterator, price_of(order));
            }
            level_iterator->add_order(order);
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
