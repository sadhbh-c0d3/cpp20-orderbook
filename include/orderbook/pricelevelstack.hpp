#ifndef INCLUDED_PRICELEVELSTACK_HPP
#define INCLUDED_PRICELEVELSTACK_HPP

#include "enums.hpp"
#include "concepts.hpp"
#include "traits.hpp"

#include "util/concepts.hpp"
#include "util/generator.hpp"

#include<vector>
#include<deque>
#include<algorithm>
#include<set>
#include<list>


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
    struct PriceTrait<OrderQuantity<OrderType>>
    {
        static auto price(const OrderQuantity<OrderType> &o) { return price_of(o.order()); }
    };

    template<OrderConcept OrderType>
    struct QuantityTrait<OrderQuantity<OrderType>>
    {
        static auto quantity(const OrderQuantity<OrderType> &o) { return o.quantity; }
    };

    template<OrderConcept _OrderType>
    struct ExecutionGeneratorPolicy
    {
        using OrderType = _OrderType;
        using ResultType = util::Generator<OrderQuantity<_OrderType>>;
    };

    template<OrderConcept _OrderType, template <typename> class _QueueType>
    requires util::IsQueue<_QueueType, OrderQuantity<_OrderType>>::value
    struct CollectExecutionsPolicy
    {
        using OrderType = _OrderType;
        using QueueType = _QueueType<OrderQuantity<OrderType>>;
        using ResultType = typename OrderType::QuantityType;

        void operator()(ResultType &r)
        {
            r = 0;
        }

        template<typename R, typename T>
        requires (
            std::is_same_v<typename std::remove_cvref<R>::type, ResultType>
            && std::is_same_v<typename std::remove_cvref<T>::type, OrderQuantity<OrderType>>
        )
        void operator()(R &&r, T &&x)
        {
            executions.emplace_back(std::forward<T>(x));
            r += x.quantity;
        }

        QueueType executions;
    };

    template<OrderConcept _OrderType, template <typename> class _QueueType>
    requires util::IsQueue<_QueueType, OrderQuantity<_OrderType>>::value
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

        template<
            ExecutionPolicyConcept<OrderQuantity<OrderType>> ExecutionPolicy,
            MatchResultPolicyConcept<OrderQuantity<OrderType>> MatchResultPolicy,
            // Unfortunatelly we cannot have policy-based selection whether function is coroutine or not.
            // Following proposal was created:
            // "Delay the judgement for coroutine function after the instantiation of template entity."
            // https://lists.isocpp.org/std-proposals/2021/01/2291.php
            std::enable_if_t<
                !IsAsyncExecutionPolicy<ExecutionPolicy, OrderQuantity<OrderType>>::value &&
                !util::IsGenerator<typename MatchResultPolicy::ResultType, OrderQuantity<OrderType>>::value,
                bool> = true
            >
        typename MatchResultPolicy::ResultType
        match_order(
            OrderType &order,
            QuantityType quantity,
            ExecutionPolicy &&execution_policy,
            MatchResultPolicy &&match_result_policy)
        {
            typename MatchResultPolicy::ResultType result;
            match_result_policy(result);

            auto it = m_orders.begin();
            auto end = m_orders.end();

            for (; it != end; ++it)
            {
                // Quantity we should fill on this order
                QuantityType quantity_to_fill = std::min(quantity, it->quantity);

                OrderQuantity<OrderType> executed{it->order(), quantity_to_fill};
                
                execution_policy(executed);

                quantity -= executed.quantity;
                it->quantity -= executed.quantity;
                m_total_quantity -= executed.quantity;

                match_result_policy(result, executed);

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

            m_orders.erase(m_orders.begin(), it);
            
            return result;
        }

        template <
            ExecutionPolicyConcept<OrderQuantity<OrderType>> ExecutionPolicy,
            MatchResultPolicyConcept<OrderQuantity<OrderType>> MatchResultPolicy,
            // Unfortunatelly we cannot have policy-based selection whether function is coroutine or not.
            // Following proposal was created:
            // "Delay the judgement for coroutine function after the instantiation of template entity."
            // https://lists.isocpp.org/std-proposals/2021/01/2291.php
            std::enable_if_t<
                util::IsGenerator<typename MatchResultPolicy::ResultType, OrderQuantity<OrderType>>::value,
                bool> = true
            >
        typename MatchResultPolicy::ResultType
        match_order(
            OrderType &order,
            QuantityType quantity,
            ExecutionPolicy &&execution_policy,
            MatchResultPolicy &&match_result_policy)
        {
            auto it = m_orders.begin();
            auto end = m_orders.end();

            for (; it != end; ++it)
            {
                // Quantity we should fill on this order
                QuantityType quantity_to_fill = std::min(quantity, it->quantity);

                OrderQuantity<OrderType> executed{it->order(), quantity_to_fill};
                
                if constexpr (IsAsyncExecutionPolicy<ExecutionPolicy, OrderQuantity<OrderType>>::value)
                {
                    co_await execution_policy(std::ref(executed));
                }
                else
                {
                    execution_policy(executed);
                }

                quantity -= executed.quantity;
                it->quantity -= executed.quantity;
                m_total_quantity -= executed.quantity;

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

            m_orders.erase(m_orders.begin(), it);
            
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
    struct PriceTrait<OrderPriceLevel<OrderType, QueueType>>
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
    requires util::IsRandomStack<_StackType, OrderPriceLevel<_OrderType, _QueueType>>::value
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

        template<ExecutionPolicyConcept<OrderQuantity<OrderType>> ExecutionPolicy>
        util::Generator<OrderQuantity<OrderType>>
        match_order(
            OrderType &order,
            ExecutionPolicy &&execution_policy = {})
        {
            constexpr bool USE_COROUTINE = true;

            if constexpr (USE_COROUTINE)
            {
                using MatchResultPolicy = ExecutionGeneratorPolicy<OrderType>;
                MatchResultPolicy match_result_policy{};

                auto g = match_order_detail(order,
                                          std::forward<ExecutionPolicy>(execution_policy),
                                          std::forward<MatchResultPolicy>(match_result_policy));

                while (g) { co_yield g(); }
            }
            else
            {
                using MatchResultPolicy = CollectExecutionsPolicy<OrderType, std::vector>;
                MatchResultPolicy match_result_policy{};

                using ExecutionPolicy_ = typename std::remove_cvref<ExecutionPolicy>::type::SyncType;
                ExecutionPolicy_ execution_policy_{execution_policy.sync()};

                auto g = match_order_detail(order,
                                          std::forward<ExecutionPolicy_>(execution_policy_),
                                          std::forward<MatchResultPolicy>(match_result_policy));
                
                while (g) { co_yield g(); }
            }
        }

        template<
            ExecutionPolicyConcept<OrderQuantity<OrderType>> ExecutionPolicy,
            MatchResultPolicyConcept<OrderQuantity<OrderType>> MatchResultPolicy
        >
        util::Generator<OrderQuantity<OrderType>>
        match_order_detail(
            OrderType &order,
            ExecutionPolicy &&execution_policy,
            MatchResultPolicy &&match_result_policy)
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
                        std::forward<ExecutionPolicy>(execution_policy),
                        std::forward<MatchResultPolicy>(match_result_policy));

                    if constexpr (util::IsGenerator<typename MatchResultPolicy::ResultType, OrderQuantity<OrderType>>::value)
                    {
                        while (res)
                        {
                            auto executed = res();
                            co_yield executed;
                            quantity_filled += executed.quantity;
                        }
                    }
                    else
                    {
                        quantity_filled += res;

                        for (const auto &executed : match_result_policy.executions)
                        {
                            co_yield executed;
                        }

                        match_result_policy.executions.clear();
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
