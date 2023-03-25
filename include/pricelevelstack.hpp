#ifndef INCLUDED_PRICELEVELSTACK_HPP
#define INCLUDED_PRICELEVELSTACK_HPP

#include "enums.hpp"
#include "concepts.hpp"
#include "traits.hpp"
#include "generator.hpp"
#include "print.hpp"

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

        OrderQuantity(OrderType &order, QuantityType quantity) noexcept
            : order_ref(order), quantity(quantity)
        {}

        // This class is fully copyable and assignable - it's essentially poco.

        OrderType &order() noexcept { return order_ref; }
        const OrderType &order() const noexcept { return order_ref; }
    
        QuantityType quantity;
        // ^ This can be remaining quantity if this instance lays on OrderPriceLevel, or
        // it can be executed quantity if this is instance holds result of applying `ExecutionPolicy`.
        // The sole purpose of `OrderQuantity` structure is to bind `OrderType` with some quantity.
    
    private:
        std::reference_wrapper<OrderType> order_ref;
        // ^ We don't necessarily want to manage lifetime of the order, and raw
        // reference to something in the outside scope should be sufficient.
        // TODO: Add OrderPointerPolicy so that used can choose whether OrderBook
        // does or does not manage lifetime of the orders.
    };
    
    template<OrderConcept OrderType>
    struct PriceTrait<OrderQuantity<OrderType>, typename OrderType::PriceType>
    {
        static auto price(const OrderQuantity<OrderType> &o) { return price_of(o.order()); }
    };

    template<OrderConcept OrderType>
    struct QuantityTrait<OrderQuantity<OrderType>, typename OrderType::QuantityType>
    {
        static auto quantity(const OrderQuantity<OrderType> &o) { return o.quantity; }
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

        template<typename ExecutionPolicy>
        util::Generator<OrderQuantity<OrderType>>
        match_order(OrderType &order, QuantityType quantity, ExecutionPolicy &execution_policy)
        {
            QuantityType quantity_filled = 0;
        
            std::cout << "match() - starting quantity = " << quantity << std::endl;
            auto it = m_orders.begin();
            auto end = m_orders.end();

            for (; it != end; ++it)
            {
                std::cout << "match() - quantity remaining = " << quantity << " --> ";
                print(*it);
                // Quantity we should fill on this order
                QuantityType quantity_to_fill = std::min(quantity, it->quantity);

                // Execution policy will tell if order could be filled
                // and resulting executed quantity will be adjusted
                OrderQuantity<OrderType> executed{it->order(), quantity_to_fill};
                co_await execution_policy(executed);
                std::cout << "match() - executed --> ";
                print(*it);

                // Update variables
                quantity -= executed.quantity;
                quantity_filled += executed.quantity;
                it->quantity -= executed.quantity;
                m_total_quantity -= executed.quantity;
                
                // Send excuted quantity to the caller
                co_yield executed;

                // Check if we fully filled incomming order
                if (!quantity)
                {
                    // Either there is no quantity left on the order,
                    // or execution policy has trimmed the order.
                    // DISCUSSION:
                    // We could introduce new type for execution instead of using OrderQuantity
                    // and perhaps have executed_quantity and cancelled_quantity there.
                    if (!it->quantity || executed.quantity != quantity_to_fill)
                    {
                        ++it;
                    }
                    break;
                }
            }

            std::cout << "match() - erase " << std::distance(m_orders.begin(), it) << std::endl;
            m_orders.erase(m_orders.begin(), it);
            
            std::cout << "match() - final quantity = " << quantity << std::endl;
            co_return;
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
    class PriceLevelStack
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

        template <typename ExecutionPolicy>
        util::Generator<OrderQuantity<OrderType>>
        match_order(OrderType &order, ExecutionPolicy &execution_policy)
        {
            QuantityType quantity_filled = 0;
            PriceLevelCompare<MySide> price_compare;

            auto it = m_levels.begin();
            
            if (price_compare(*it, order))
            {
                for (; it != m_levels.end(); ++it)
                {
                    if (quantity_of(order) == quantity_filled)
                    {
                        break; //< Order was fully filled
                    }
                    else if (price_compare(order, *it))
                    {
                        break; //< Order was partially filled
                    }

                    auto res = it->match_order(
                        order,
                        quantity_of(order) - quantity_filled,
                        execution_policy);

                    while (res)
                    {
                        auto executed = res();
                        co_yield executed;
                        quantity_filled += executed.quantity;
                    }

                    if (!it->empty())
                    {
                        // Level wasn't fully filled
                        break;
                    }
                }

                // Remove all levels that were fully filled, but keep the one
                // that still has quantity left
                m_levels.erase(m_levels.begin(), it);
            }

            co_return;
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

            if (level_iterator == m_levels.end() || price_of(*level_iterator) != price_of(order))
            {
                level_iterator = m_levels.emplace(level_iterator, price_of(order));
            }
            level_iterator->add_order(order, quantity);
        }
    };

    template<template <typename> class StackType = std::deque, template <typename> class QueueType = std::deque>
    struct PriceLevelStackBookSidePolicy
    {
        template<Side MySide, OrderConcept OrderType>
        using OrderBookSideType = PriceLevelStack<MySide, OrderType, StackType, QueueType>;
    };

    template<OrderConcept _OrderType>
    class OrderSizeLimit
    {
    public:
        using OrderType = _OrderType;
        using QuantityType = typename OrderType::QuantityType;

        OrderSizeLimit(QuantityType max_order_size) : max_order_size_(max_order_size)
        {
        }

        void operator()(OrderQuantity<OrderType> &executed)
        {
            if (executed.quantity > max_order_size_)
            {
                executed.quantity = max_order_size_;
            }
        }

    private:
        const QuantityType max_order_size_;
    };

} // end of namespace sadhbhcraft::orderbook
#endif//INCLUDED_PRICELEVELSTACK_HPP
