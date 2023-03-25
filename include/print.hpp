#ifndef INCLUDED_PRINT_HPP
#define INCLUDED_PRINT_HPP

#include "concepts.hpp"
#include "traits.hpp"

#include <iostream>


namespace sadhbhcraft::orderbook
{
    template<OrderLikeConcept OrderLikeType>
    void print(const OrderLikeType &o)
    {
        std::cerr << "{ .price=" << price_of(o) << ", .quantity=" << quantity_of(o) << " }" << std::endl;
    }

} // end of namespace sadhbhcraft::orderbook
#endif // INCLUDED_PRINT_HPP