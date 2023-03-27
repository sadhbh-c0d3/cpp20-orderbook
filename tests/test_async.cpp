#include "test_util.hpp"

#include "util/async.hpp"
#include "util/generator.hpp"

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

bool assert_front_equal_and_pop(std::vector<OrderQuantity> &expected, const OrderQuantity &result)
{
    assert(!expected.empty());
    auto expected_result = expected.front();
    bool error = (result.order != expected_result.order || result.quantity != expected_result.quantity);
    if (error)
    {
        std::cerr << "Assertion error: "
                  << "{" << result.order << ", " << result.quantity 
                  << "} != {" << expected_result.order << ", " << expected_result.quantity 
                  << "}" << std::endl;
    }
    expected.erase(expected.begin());
    return error;
}

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

    std::vector<OrderQuantity> expected_executions{
        {1, 10},
        {2, 2}
    };

    std::vector<OrderQuantity> expected_rest{
        {2, 3},
        {3 ,2},
        {4, 7},
        {5, 8}
    };

    std::cout << "Level before" << std::endl;
    for (const auto &oq : level.queue)
    {
        std::cout << oq.order << ", " << oq.quantity << std::endl;
    }
    
    auto results = level.match(12);
    auto error = false;

    std::cout << "Executions" << std::endl;
    while (results)
    {
        auto result = results();
        std::cout << result.order << ", " << result.quantity << std::endl;
        error |= assert_front_equal_and_pop(expected_executions, result);
    }

    std::cout << "Level after" << std::endl;
    for (const auto &oq : level.queue)
    {
        std::cout << oq.order << ", " << oq.quantity << std::endl;
        error |= assert_front_equal_and_pop(expected_rest, oq);
    }

    assert(!error);
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
    
    std::vector<OrderQuantity> expected_executions{
        {1, 5},
        {2, 5},
        {3, 2}
    };

    std::vector<OrderQuantity> expected_rest{
        {4, 7},
        {5, 8}
    };

    std::cout << "Level before" << std::endl;
    for (const auto &oq : level.queue)
    {
        std::cout << oq.order << ", " << oq.quantity << std::endl;
    }

    AsyncImmediate<OrderSizeLimit> ep{5};
    
    auto results = level.match(12, ep);
    auto error = false;

    std::cout << "Executions" << std::endl;
    while (results)
    {
        auto result = results();
        std::cout << result.order << ", " << result.quantity << std::endl;
        error |= assert_front_equal_and_pop(expected_executions, result);
    }

    std::cout << "Level after" << std::endl;
    for (const auto &oq : level.queue)
    {
        std::cout << oq.order << ", " << oq.quantity << std::endl;
        error |= assert_front_equal_and_pop(expected_rest, oq);
    }

    assert(!error);
    std::cout << "OK" << std::endl;
}

int main()
{
    main1();
    main2();
    return 0;
}