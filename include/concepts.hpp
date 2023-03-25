#ifndef INCLUDED_CONCEPTS_HPP
#define INCLUDED_CONCEPTS_HPP

#include "enums.hpp"

#include <concepts>


namespace sadhbhcraft::orderbook
{
    struct UnconstrainedType {};
    
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
        requires(T &x) {
            OrderConcept<typename T::OrderType>;
            {
                x.add_order(
                    std::declval<typename T::OrderType &>(),
                    std::declval<typename T::OrderType::QuantityType>())
                } -> std::convertible_to<void>;
            {
                x.match_order(std::declval<typename T::OrderType &>(), std::declval<UnconstrainedType&>())
                } -> MatchGeneratorConcept<typename T::OrderType>;
            {
                x.side()
                } -> std::convertible_to<Side>;
        };

}; // end of namespace
#endif//INCLUDED_CONCEPTS_HPP