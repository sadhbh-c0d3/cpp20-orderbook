#ifndef INCLUDED_CONCEPTS_HPP
#define INCLUDED_CONCEPTS_HPP

#include "enums.hpp"

#include "util/async.hpp"
#include "util/concepts.hpp"


namespace sadhbhcraft::orderbook
{
    template <typename T, typename A>
    concept ExecutionPolicyConcept = 
        requires(T x) {
            { x(std::declval<A>()) } -> util::AwaitableConcept;
        };

    template<typename T>
    concept OrderConcept =
        requires(T &x) {
            util::NumberConcept<typename T::PriceType>;
            util::NumberConcept<typename T::QuantityType>;
            { x.side } -> std::convertible_to<Side>;
            { x.order_type } -> std::convertible_to<OrderType>;
            { x.price } -> std::convertible_to<typename T::PriceType>;
            { x.quantity } -> std::convertible_to<typename T::QuantityType>;
        };

    template <typename T, typename OrderType>
    concept OrderQuantityConcept = 
        requires(T &x) {
            { x.order() } -> std::convertible_to<OrderType>;
            { x.quantity } -> std::convertible_to<typename OrderType::QuantityType>;
        };

    template <typename T, typename OrderType>
    concept MatchGeneratorConcept =
        requires(T &x) {
            { x() } -> OrderQuantityConcept<OrderType>;
            { !x } -> std::convertible_to<bool>;
        };

    template <typename T>
    concept OrderBookSideConcept =
        requires(T &x, const T &c) {
            OrderConcept<typename T::OrderType>;
            {
                x.add_order(
                    std::declval<typename T::OrderType &>(),
                    std::declval<typename T::OrderType::QuantityType>())
                } -> std::convertible_to<void>;
            {
                x.match_order(
                    std::declval<typename T::OrderType &>(),
                    std::declval<util::AsyncNoop>())
                } -> MatchGeneratorConcept<typename T::OrderType>;
            { c.side() } -> std::convertible_to<Side>;
        };
    
    template <typename T, typename OrderType>
    concept PriceLevelConcept =
        requires(T &x, const T &c) {
            { c.price() } -> std::convertible_to<typename OrderType::PriceType>;
            { c.total_quantity() } -> std::convertible_to<typename OrderType::QuantityType>;
            { c.first() } -> OrderQuantityConcept<OrderType>;
            { *c.begin() } -> OrderQuantityConcept<OrderType>;
            { *c.end() } -> OrderQuantityConcept<OrderType>;
        };

    template <typename T>
    concept PriceLevelOrderBookSideConcept =
        requires(T &x, const T &c) {
            OrderBookSideConcept<T>;
            { c.top() } -> PriceLevelConcept<typename T::OrderType>;
            { *c.begin() } -> PriceLevelConcept<typename T::OrderType>;
            { *c.end() } -> PriceLevelConcept<typename T::OrderType>;
        };

    template <typename T>
    concept PriceLevelOrderBookConcept =
        requires(T &x, const T &c) {
            PriceLevelOrderBookSideConcept<typename T::BidBookSideType>;
            PriceLevelOrderBookSideConcept<typename T::AskBookSideType>;
            { c.bid() } -> std::convertible_to<typename T::BidBookSideType>;
            { c.ask() } -> std::convertible_to<typename T::AskBookSideType>;
        };

}; // end of namespace
#endif//INCLUDED_CONCEPTS_HPP