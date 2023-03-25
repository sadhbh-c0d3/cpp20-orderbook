# C++20 Order Book

## About

An order book in C++20 with concepts and coroutines.

This work can be seen as an attepmt to make paractical use of new features of C++ language.

### Architecture
The architecture is very simple and intuitive.
There is an `OrderBook` template that takes `OrderType` and `OrderBookSide` template parameters.
 
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
