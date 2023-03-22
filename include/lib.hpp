#ifndef INCLUDED_LIB_HPP
#define INCLUDED_LIB_HPP

#include<string>
#include<vector>
#include<deque>
#include<map>
#include<algorithm>

namespace sadhbhcraft::orderbook
{
    enum class Side
    {
        Buy,    // aka Bid, crosses (matches) Ask
        Sell    // aka Ask, crosses (matches) Bid
    };

    enum class OrderType
    {
        Market, // Take order(s) from opposite side to fully fill quantity requested
        Limit,  // Place order on the book, cross and execute up to requested price level
        IOC,    // Take order(s) from opposite side up to requested price level
        FOC     // Take order(s) from opposite side only up to requested price level,
                // and only if can fully fill quantity requested
    };

    template <typename T>
    concept NumericType = std::is_arithmetic<T>::value;

    template<typename T>
    concept OrderConcept =
        requires(T &x) {
            NumericType<typename T::PriceType>;
            NumericType<typename T::QuantityType>;
            { x.side } -> std::convertible_to<Side>;
            { x.order_type } -> std::convertible_to<OrderType>;
            { x.price } -> std::convertible_to<typename T::PriceType>;
            { x.quantity } -> std::convertible_to<typename T::QuantityType>;
        };

    template<typename T>
    concept OrderBookSideConcept =
        requires(T &x) {
            OrderConcept<typename T::OrderType>;
            { x.add_order(std::declval<typename T::OrderType&>()) } -> std::convertible_to<void>;
            { x.side() } -> std::convertible_to<Side>; // C++20 doesn't support '-> Side'
        };

    template<typename T, NumericType = int> struct PriceTrait { };
    template<typename T, NumericType = int> struct QuantityTrait { };

    template<NumericType PriceType>
    struct PriceTrait<PriceType, PriceType>
    {
        static auto price(const PriceType &p) { return p; }
    };

    template<OrderConcept OrderType>
    struct PriceTrait<OrderType, typename OrderType::PriceType>
    {
        static auto price(const OrderType &o) { return o.price; }
    };

    template<OrderConcept OrderType>
    struct QuantityTrait<OrderType, typename OrderType::QuantityType>
    {
        static auto quantity(const OrderType &o) { return o.quantity; }
    };

    template<typename T, NumericType PriceType=int>
    PriceType price_of(const T &x)
    {
        return PriceTrait<T>::price(x);
    }

    template<typename T, NumericType QuantityType=int>
    QuantityType quantity_of(const T &x)
    {
        return QuantityTrait<T>::quantity(x);
    }


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
        template<typename T>
        using QueueType = _QueueType<T>;

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
    
    template<template <typename> class> struct PriceLevelLookupTraits { };

    template<Side MySide>
    struct StdLowerBoundPriceLevelLookupPolicy
    {
        static auto find_or_get_insert_iterator(auto &levels, auto price)
        {
            return std::lower_bound(levels.begin(), levels.end(), price, PriceLevelCompare<MySide>());
        }
    };

    template<>
    struct PriceLevelLookupTraits<std::vector>
    {
        template<Side MySide>
        using value = StdLowerBoundPriceLevelLookupPolicy<MySide>;
    };

    template<>
    struct PriceLevelLookupTraits<std::deque>
    {
        template<Side MySide>
        using value = StdLowerBoundPriceLevelLookupPolicy<MySide>;
    };
    
    template <Side MySide, OrderConcept OrderType, template <typename> class StackType, template <typename> class QueueType>
    auto find_or_get_insert_iterator(StackType<OrderPriceLevel<OrderType, QueueType>> &levels, typename OrderType::PriceType price)
    {
        using FindPolicy = typename PriceLevelLookupTraits<StackType>::value<MySide>;
        return FindPolicy::find_or_get_insert_iterator(levels, price);
    };

    template<Side MySide, OrderConcept _OrderType,
        template <typename> class _StackType,
        template <typename> class _QueueType>
    class StackOfOrderQueuesBookSide
    {
    public:
        typedef _OrderType OrderType;
        template<typename T>
        using StackType = _StackType<T>;
        template<typename T>
        using QueueType = _QueueType<T>;

        void add_order(OrderType &order) { do_add_order(order); }

        constexpr Side side() const { return MySide; }

        auto begin() const { return m_levels.begin(); }
        auto end() const { return m_levels.end(); }

        const auto &top() const { return m_levels.front(); }

        size_t size() const { return m_levels.size(); }
        bool empty() const { return m_levels.empty(); }

    protected:
        StackType<OrderPriceLevel<OrderType, QueueType>> m_levels;
        
        void do_add_order(OrderType &order)
        {
            auto level_iterator = find_or_get_insert_iterator<MySide>(m_levels, price_of(order));

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
#endif // INCLUDED_LIB_HPP