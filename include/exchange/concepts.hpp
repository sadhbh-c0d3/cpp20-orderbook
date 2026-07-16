#ifndef INCLUDED_EXCHANGE_CONCEPTS_HPP
#define INCLUDED_EXCHANGE_CONCEPTS_HPP

#include "util/concepts.hpp"


namespace sadhbhcraft::exchange {

    template <typename T>
    concept MsgParserConcept =
        requires(T &x) {
            util::NumberConcept<typename T::KeyType>;
            util::NumberConcept<typename T::IntegerType>;
            util::NumberConcept<typename T::DecimalType>;
            typename T::StringType;
            {
                x.parse_key(
                    std::declval<typename T::KeyType &>())
                } -> std::convertible_to<bool>;
            {
                x.parse_value(
                    std::declval<typename T::IntegerType &>())
                } -> std::convertible_to<bool>;
            {
                x.parse_value(
                    std::declval<typename T::DecimalType &>())
                } -> std::convertible_to<bool>;
            {
                x.parse_value(
                    std::declval<typename T::StringType &>())
                } -> std::convertible_to<bool>;
            {
                x.parse_field(
                    std::declval<typename T::KeyType>(),
                    std::declval<typename T::IntegerType &>())
                } -> std::convertible_to<bool>;
            {
                x.parse_field(
                    std::declval<typename T::KeyType>(),
                    std::declval<typename T::DecimalType &>())
                } -> std::convertible_to<bool>;
            {
                x.parse_field(
                    std::declval<typename T::KeyType>(),
                    std::declval<typename T::StringType &>())
                } -> std::convertible_to<bool>;
        };


} // end of namespace sadhbhcraft::exchange
#endif//INCLUDED_EXCHANGE_CONCEPTS_HPP