// #include "lib.hpp"
// 
// 
// int main(int argc, const char** argv)
// {
//     namespace scob = sadhbhcraft::orderbook;
// 
//     return 0;
// }

#include "generator.hpp"
#include "async.hpp"

#include <vector>
#include <iostream>


template<typename T>
using Generator = sadhbhcraft::util::Generator<T>;
using AsyncNoop = sadhbhcraft::util::AsyncNoop;
template<typename F>
using AsyncImmediate = sadhbhcraft::util::AsyncImmediate<F>;


struct OrderQuantity
{
    int order;
    int quantity;
};

class OrderSizeLimit
{
public:
    OrderSizeLimit(int max_order_size): max_order_size_(max_order_size)
    {}

    void operator ()(OrderQuantity &oq)
    {
        if (oq.quantity > max_order_size_)
        {
            oq.quantity = max_order_size_;
        }
    }

private:
    const int max_order_size_;
};

struct Level
{
    std::vector<OrderQuantity> queue;

    template<typename ExecutionPolicy = AsyncNoop>
    Generator<OrderQuantity> match(int quantity, ExecutionPolicy &&ep = {})
    {
        const auto begin = queue.begin();
        const auto end = queue.end();
        auto iter = begin;
        
        for (begin; iter != end; ++iter)
        {
            if (!quantity)
            {
                break;
            }

            if (quantity < iter->quantity)
            {
                OrderQuantity oq{
                    .order = iter->order, 
                    .quantity = quantity
                };
                co_await ep(oq);
                iter->quantity -= oq.quantity;
                quantity -= oq.quantity;
                co_yield oq;
                break;
            }
            else
            {
                OrderQuantity oq{
                    .order = iter->order, 
                    .quantity = iter->quantity
                };
                co_await ep(oq);
                quantity -= oq.quantity;
                iter->quantity = 0; // remainder is cancelled
                co_yield oq;
            }
        }

        queue.erase(begin, iter);
    }
};


void main1()
{
    Level level{
        .queue={
            {.order=1, .quantity=10},
            {.order=2, .quantity=5},
            {.order=3, .quantity=2},
            {.order=4, .quantity=7},
            {.order=5, .quantity=8},
        }
    };

    std::cout << "Level before" << std::endl;
    for (const auto &oq : level.queue)
    {
        std::cout << oq.order << ", " << oq.quantity << std::endl;
    }
    
    auto results = level.match(12);

    std::cout << "Executions" << std::endl;
    while (results)
    {
        auto result = results();
        std::cout << result.order << ", " << result.quantity << std::endl;
    }

    std::cout << "Level after" << std::endl;
    for (const auto &oq : level.queue)
    {
        std::cout << oq.order << ", " << oq.quantity << std::endl;
    }

    std::cout << "OK" << std::endl;
}

void main2()
{
    Level level{
        .queue={
            {.order=1, .quantity=10},
            {.order=2, .quantity=5},
            {.order=3, .quantity=2},
            {.order=4, .quantity=7},
            {.order=5, .quantity=8},
        }
    };
    std::cout << "Level before" << std::endl;
    for (const auto &oq : level.queue)
    {
        std::cout << oq.order << ", " << oq.quantity << std::endl;
    }

    AsyncImmediate<OrderSizeLimit> ep{5};
    
    auto results = level.match(12, ep);

    std::cout << "Executions" << std::endl;
    while (results)
    {
        auto result = results();
        std::cout << result.order << ", " << result.quantity << std::endl;
    }

    std::cout << "Level after" << std::endl;
    for (const auto &oq : level.queue)
    {
        std::cout << oq.order << ", " << oq.quantity << std::endl;
    }

    std::cout << "OK" << std::endl;
}

int main()
{
    main1();
    main2();
    return 0;
}