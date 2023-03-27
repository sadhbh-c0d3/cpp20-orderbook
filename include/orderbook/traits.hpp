#ifndef INCLUDED_TRAITS_HPP
#define INCLUDED_TRAITS_HPP

#include "enums.hpp"

#include "util/concepts.hpp"


namespace sadhbhcraft::orderbook
{
    template<typename T> struct PriceTrait { };
    template<typename T> struct QuantityTrait { };

    template<util::NumberConcept PriceType>
    struct PriceTrait<PriceType>
    {
        static auto price(const PriceType &p) { return p; }
    };

    template<OrderConcept OrderType>
    struct PriceTrait<OrderType>
    {
        static auto price(const OrderType &o) { return o.price; }
    };

    template<OrderConcept OrderType>
    struct QuantityTrait<OrderType>
    {
        static auto quantity(const OrderType &o) { return o.quantity; }
    };

    template<typename T>
    auto price_of(const T &x)
    {
        return PriceTrait<T>::price(x);
    }

    template<typename T>
    auto quantity_of(const T &x)
    {
        return QuantityTrait<T>::quantity(x);
    }

}; // end of namespace
#endif//INCLUDED_TRAITS_HPP