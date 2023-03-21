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

    struct Instrument
    {
        std::string symbol;
    };

    struct Market
    {
        Instrument &main;
        Instrument &traded;
    };

    struct Order
    {
        typedef int PriceType;
        typedef int QuantityType;

        Market &market;
        Side side;
        OrderType order_type;
        int price;
        int quantity;
    };

    struct OrderQuantity
    {
        OrderQuantity(Order &order): order(order)
        {}

        Order &order;
        int quantity;
    };

    class OrderPriceLevel
    {
    public:
        OrderPriceLevel(int price): m_price(price)
        {}

        void add_order(Order &order)
        {
            m_orders.push_back(order);
        }

        int price() const { return m_price; }

        auto begin() const { return m_orders.begin(); }
        auto end() const { return m_orders.end(); }

        auto &first() { return m_orders.front(); }
        const auto &first() const { return m_orders.front(); }

        size_t size() const { return m_orders.size(); }
        bool empty() const { return m_orders.empty(); }

    private:
        std::deque<OrderQuantity> m_orders;
        int m_price;
    };

    template<typename T, NumericType = int>
    struct PriceTrait
    {
    };

    template<NumericType PriceType>
    struct PriceTrait<PriceType, PriceType>
    {
        static PriceType price(const PriceType &p) { return p; }
    };

    template<NumericType PriceType>
    struct PriceTrait<Order, PriceType>
    {
        static PriceType price(const Order &o) { return o.price; }
    };

    template<NumericType PriceType>
    struct PriceTrait<OrderPriceLevel, PriceType>
    {
        static PriceType price(const OrderPriceLevel &opl) { return opl.price(); }
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

    template<Side MySide, OrderConcept _OrderType = Order>
    class OrderBookSide
    {
    public:
        typedef _OrderType OrderType;

        void add_order(OrderType &order) { do_add_order(order); }

        constexpr Side side() const { return MySide; }

        auto begin() const { return m_levels.begin(); }
        auto end() const { return m_levels.end(); }

        auto &top() { return m_levels.front(); }
        const auto &top() const { return m_levels.front(); }

        size_t size() const { return m_levels.size(); }
        bool empty() const { return m_levels.empty(); }

    protected:
        std::deque<OrderPriceLevel> m_levels;
        
        auto find_or_get_insert_iterator(OrderType::PriceType price)
        {
            return std::lower_bound(m_levels.begin(), m_levels.end(), price, PriceLevelCompare<MySide>());
        }
        
        // NOTE: If we use find_or_get_insert_iterator() before its declaration we'll get this error:
        // error: use of 'auto sadhbhcraft::orderbook::OrderBookSide::find_or_get_insert_iterator(int)' before deduction of 'auto'
        void do_add_order(OrderType &order)
        {
            auto level_iterator = find_or_get_insert_iterator(price_of(order));
            if (level_iterator == m_levels.end() || level_iterator->price() != order.price)
            {
                level_iterator = m_levels.insert(level_iterator, OrderPriceLevel(price_of(order)));
            }
            level_iterator->add_order(order);
        }
    };

    template<OrderConcept _OrderType = Order>
    class OrderBook
    {
    public:
        typedef _OrderType OrderType;

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
        OrderBookSide<Side::Buy, OrderType> m_bid;
        OrderBookSide<Side::Sell, OrderType> m_ask;

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