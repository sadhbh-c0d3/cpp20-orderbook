#ifndef INCLUDED_CONCEPTS_HPP
#define INCLUDED_CONCEPTS_HPP

#include "enums.hpp"

#include <concepts>


namespace sadhbhcraft::orderbook
{
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
                x.match_order(std::declval<typename T::OrderType &>())
                } -> std::convertible_to<typename T::OrderType::QuantityType>;
            {
                x.side()
                } -> std::convertible_to<Side>; // C++20 doesn't support '-> Side'
        };

}; // end of namespace
#endif//INCLUDED_CONCEPTS_HPP