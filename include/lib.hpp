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

    inline int price_of(int x)
    {
        return x;
    }

    inline int price_of(const Order &x)
    {
        return x.price;
    }

    inline int price_of(const OrderPriceLevel &x)
    {
        return x.price();
    }

    class OrderBookSide
    {
    public:
        void add_order(Order &order) { do_add_order(order); }

        virtual Side side() const = 0;

        auto begin() const { return m_levels.begin(); }
        auto end() const { return m_levels.end(); }

        auto &top() { return m_levels.front(); }
        const auto &top() const { return m_levels.front(); }

        size_t size() const { return m_levels.size(); }
        bool empty() const { return m_levels.empty(); }

    protected:
        std::deque<OrderPriceLevel> m_levels;
        
        // NOTE: you cannot have virtual function with auto return value
        virtual std::deque<OrderPriceLevel>::iterator find_or_get_insert_iterator(int price) = 0;
        
        // NOTE: If we use find_or_get_insert_iterator() before its declaration we'll get this error:
        // error: use of 'auto sadhbhcraft::orderbook::OrderBookSide::find_or_get_insert_iterator(int)' before deduction of 'auto'
        void do_add_order(Order &order)
        {
            auto level_iterator = find_or_get_insert_iterator(price_of(order));
            if (level_iterator == m_levels.end() || level_iterator->price() != order.price)
            {
                level_iterator = m_levels.insert(level_iterator, OrderPriceLevel(price_of(order)));
            }
            level_iterator->add_order(order);
        }
    };

    class OrderBookBidSide : public OrderBookSide
    {
    public:
        Side side() const override { return Side::Buy; }

    protected:
        std::deque<OrderPriceLevel>::iterator find_or_get_insert_iterator(int price) override
        {
            return std::lower_bound(m_levels.begin(), m_levels.end(), price, [this](auto a, auto b){
                return price_of(b) < price_of(a);
            });
        }
    };

    class OrderBookAskSide : public OrderBookSide
    {
    public:
        Side side() const override { return Side::Sell; }

    protected:
        std::deque<OrderPriceLevel>::iterator find_or_get_insert_iterator(int price) override
        {
            return std::lower_bound(m_levels.begin(), m_levels.end(), price, [this](auto a, auto b){
                return price_of(a) < price_of(b);
            });
        }
    };

    class OrderBook
    {
    public:
        void accept_order(Order &order)
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
        OrderBookBidSide m_bid;
        OrderBookAskSide m_ask;

        void do_accept_order(Order &order, OrderBookSide &side)
        {
            // ... now we can write more code handling order
            // and we won't be repeating that code
            side.add_order(order);
        }

    };

} // end of namespace sadhbhcraft::orderbook
#endif // INCLUDED_LIB_HPP