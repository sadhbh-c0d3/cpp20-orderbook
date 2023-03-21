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

        const int price() const { return m_price; }

        std::deque<OrderQuantity>::const_iterator begin() const { return m_orders.begin(); }
        std::deque<OrderQuantity>::const_iterator end() const { return m_orders.end(); }

        OrderQuantity &first() { return m_orders.front(); }
        const OrderQuantity &first() const { return m_orders.front(); }

        size_t size() const { return m_orders.size(); }
        bool empty() const { return m_orders.empty(); }

    private:
        std::deque<OrderQuantity> m_orders;
        int m_price;
    };

    struct OrderPriceLevelCompare
    {
        OrderPriceLevelCompare(Side side): m_side(side){}

        bool operator()(const OrderPriceLevel &a, const OrderPriceLevel &b) const
        {
            return (m_side == Side::Buy) ? b.price() < a.price() : a.price() < b.price();
        }

        bool operator()(int price, const OrderPriceLevel &b) const
        {
            return (m_side == Side::Buy) ? b.price() < price : price < b.price();
        }

        bool operator()(const OrderPriceLevel &a, int price) const
        {
            return (m_side == Side::Buy) ? price < a.price() : a.price() < price;
        }

    private:
        Side m_side;
    };


    class OrderBookSide
    {
    public:
        OrderBookSide(Side side): m_side(side)
        {}

        void add_order(Order &order)
        {
            std::deque<OrderPriceLevel>::iterator level_iterator = find_or_get_insert_iterator(order.price);
            if (level_iterator == m_levels.end() || level_iterator->price() != order.price)
            {
                level_iterator = m_levels.insert(level_iterator, OrderPriceLevel(order.price));
            }
            level_iterator->add_order(order);
        }

        Side side() const { return m_side ;}

        std::deque<OrderPriceLevel>::const_iterator begin() const { return m_levels.begin(); }
        std::deque<OrderPriceLevel>::const_iterator end() const { return m_levels.end(); }

        OrderPriceLevel &top() { return m_levels.front(); }
        const OrderPriceLevel &top() const { return m_levels.front(); }

        size_t size() const { return m_levels.size(); }
        bool empty() const { return m_levels.empty(); }

    private:
        std::deque<OrderPriceLevel> m_levels;
        Side m_side;

        std::deque<OrderPriceLevel>::iterator find_or_get_insert_iterator(int price)
        {
            return std::lower_bound(m_levels.begin(), m_levels.end(), price, OrderPriceLevelCompare(m_side));
        }
    };

    class OrderBook
    {
    public:
        OrderBook(): m_bid(Side::Buy), m_ask(Side::Sell)
        {}

        void accept_order(Order &order)
        {
            if (order.side == Side::Buy)
            {
                m_bid.add_order(order);
            }
            else
            {
                m_ask.add_order(order);
            }
        }

        const OrderBookSide &bid() const { return m_bid; }
        const OrderBookSide &ask() const { return m_ask; }

    private:
        OrderBookSide m_bid;
        OrderBookSide m_ask;
    };

} // end of namespace sadhbhcraft::orderbook
#endif // INCLUDED_LIB_HPP