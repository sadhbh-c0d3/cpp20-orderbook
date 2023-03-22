#ifndef INCLUDED_LIB_HPP
#define INCLUDED_LIB_HPP

#include<string>
#include<deque>
#include<algorithm>

namespace sadhbhcraft::orderbook
{
    enum class Side
    {
        Buy,
        Sell
    };

    enum class OrderType
    {
        Market,
        Limit,
        IOC,
        FOC
    };

    template <typename T>
    concept NumericType = std::is_arithmetic<T>::value;

    template<typename T>
    concept OrderConcept =
        requires(T &x) {
            NumericType<typename T::PriceType>;
            NumericType<typename T::QuantityType>;
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

    template<typename T, NumericType = int>
    struct PriceTrait
    {
    };

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

    template<typename T, NumericType PriceType=int>
    PriceType price_of(const T &x)
    {
        return PriceTrait<T>::price(x);
    }

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


    struct Instrument
    {
        std::string symbol;
    };

    struct Market
    {
        Instrument &main;
        Instrument &traded;
    };

    template<NumericType _PriceType = int, NumericType _QuantityType = int>
    struct Order
    {
        typedef _PriceType PriceType;
        typedef _QuantityType QuantityType;

        Market &market;
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

        OrderQuantity(OrderType &order): order(order)
        {}

        OrderType &order;
        QuantityType quantity;
    };


    template<OrderConcept _OrderType, template <typename> class _QueueType>
    class OrderPriceLevel
    {
    public:
        typedef _OrderType OrderType;
        typedef typename _OrderType::PriceType PriceType;
        template<typename T>
        using QueueType = _QueueType<T>;

        OrderPriceLevel(PriceType price): m_price(price)
        {}

        void add_order(OrderType &order)
        {
            m_orders.emplace_back(order);
        }

        auto price() const { return m_price; }

        auto begin() const { return m_orders.begin(); }
        auto end() const { return m_orders.end(); }

        auto &first() { return m_orders.front(); }
        const auto &first() const { return m_orders.front(); }

        size_t size() const { return m_orders.size(); }
        bool empty() const { return m_orders.empty(); }

    private:
        QueueType<OrderQuantity<OrderType>> m_orders;
        PriceType m_price;
    };

    template<OrderConcept OrderType, template <typename> class QueueType>
    struct PriceTrait<OrderPriceLevel<OrderType, QueueType>, typename OrderType::PriceType>
    {
        static auto price(const OrderPriceLevel<OrderType, QueueType> &opl) { return opl.price(); }
    };

    template<Side MySide, OrderConcept _OrderType,
        template <typename> class _StackType,
        template <typename> class _QueueType>
    class OrderBookSide
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

        auto &top() { return m_levels.front(); }
        const auto &top() const { return m_levels.front(); }

        size_t size() const { return m_levels.size(); }
        bool empty() const { return m_levels.empty(); }

    protected:
        StackType<OrderPriceLevel<OrderType, QueueType>> m_levels;
        
        auto find_or_get_insert_iterator(OrderType::PriceType price)
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

    template<OrderConcept _OrderType = Order<>,
        template <typename> class _StackType = std::deque,
        template <typename> class _QueueType = std::deque>
    class OrderBook
    {
    public:
        typedef _OrderType OrderType;
        template<typename T>
        using StackType = _StackType<T>;
        template<typename T>
        using QueueType = _QueueType<T>;

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
        OrderBookSide<Side::Buy, OrderType, StackType, QueueType> m_bid;
        OrderBookSide<Side::Sell, OrderType, StackType, QueueType> m_ask;

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