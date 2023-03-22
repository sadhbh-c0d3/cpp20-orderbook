#ifndef INCLUDED_TRAITS_HPP
#define INCLUDED_TRAITS_HPP

#include "enums.hpp"
#include "concepts.hpp"


namespace sadhbhcraft::orderbook
{
    template<typename T, NumericType = int> struct PriceTrait { };
    template<typename T, NumericType = int> struct QuantityTrait { };

    template<NumericType PriceType>
    struct PriceTrait<PriceType, PriceType>
    {
        static auto price(const PriceType &p) { return p; }
    };

    template<OrderConcept OrderType>
    struct PriceTrait<OrderType, typename OrderType::PriceType>
    {
        static auto price(const OrderType &o) { return o.price; }
    };

    template<OrderConcept OrderType>
    struct QuantityTrait<OrderType, typename OrderType::QuantityType>
    {
        static auto quantity(const OrderType &o) { return o.quantity; }
    };

    template<typename T, NumericType PriceType=int>
    PriceType price_of(const T &x)
    {
        return PriceTrait<T>::price(x);
    }

    template<typename T, NumericType QuantityType=int>
    QuantityType quantity_of(const T &x)
    {
        return QuantityTrait<T>::quantity(x);
    }

}; // end of namespace
#endif//INCLUDED_TRAITS_HPP