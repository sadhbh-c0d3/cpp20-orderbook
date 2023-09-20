# C++20 Order Book

### ***Order Book implementation in C++20 (Concepts & Co-Routines)***

[![Watch My Video!](https://img.youtube.com/vi/3k2O3KiE_xY/0.jpg)](https://www.youtube.com/watch?v=3k2O3KiE_xY)

## About

Order Book implementation in C++20 and a long journey from C++98.

This work can be seen as an attepmt to make paractical use of new features of C++ language.

Project created using *Dockerized C++ Code Template* from my GitHub repository [cpp-template](https://github.com/sadhbh-c0d3/cpp-template).

## Experimental Branches

- `coro-policy` - Policy, which controls whether we use co-routines or regular functions.
- `fix-order` - FIX message parser using C++20 concepts
- `generator-istream` - IStream powered by Generator co-routine

## Journey from C++98 to C++20

I wanted to show, how do I see as the user of C++, the evolution of the programming techniques from C++98 up to C++20.

I have organised commits in a specific way so that first commits show how this would be implemented in C++98, and with every
next commit we move towards C++20 concepts and co-routines.


## Architecture

The architecture is very simple and intuitive.
There is an `OrderBook` template that takes `OrderType` and `OrderBookSidePolicy` template parameters.
 
The `OrderType` needs to conform to `OrderConcept`, i.e. needs to have a price, quantity, side and type.
 
The `OrderBookSide` needs to be some implementation of the order book side, which provides two methods `add_order()` and `match_order()`.
As a result of calling `add_order()`, an order should be stored somewhere on that side of the book.

The `match_order()` is a co-routine, which should match incoming order against orders on the side of the book, and 
for every match yield an order execution information, which is a pair of matched order and quantity executed.
Additionally `match_order()` receives `ExecutionPolicy`, which may control order executions, i.e.
it may reject or reduce executed quantity. In theory user may provide asynchronous `ExecutionPolicy`.

Provided is an implementation of Price Level book side, which conforms to `PriceLevelOrderBookSideConcept`.
This means that each price levels are accessible via range between `begin()` and `end()`, and top level by `top()`.
Then each level conforms to `PriceLevelConcept`, which then allows you to iterate over orders on that level
within the range between `begin()` and `end()`, and also first order by `first()`.

We also provide `Order` template that takes `PriceType` and `QuantityType` template parameters,
which control the numeric types used for price and quantity.  There are also `PriceTraits` and `QuantityTraits`,
which provide additional flexibility. We test that solution works for `int`, `long`, and `double` as
type of each price or quantity.

## No Smart Pointers

At this stage the order book implementation acts purely as matching engine, and not as order manager,
and because of that I am not using smart pointers like `shared_ptr<>` in the `OrderQuantity`.
I decided to leave the responsibility of order management to the user. However I was considering to
add order pointer policy class that would control whether we use smart pointers or not.


## Building

1. Launch Docker container
```
    docker-compose up -d
```

2. Enter development environment within Docker container
```
    ./enter-app.sh
```

3. Configure
```
    mkdir /home/build
    cd /home/build
    cmake /home/app
```

4. Build
```
    make
```

5. Test
```
    ctest
```

6. Run App
```
    ./bin/run_app
```
