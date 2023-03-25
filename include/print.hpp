#ifndef INCLUDED_PRINT_HPP
#define INCLUDED_PRINT_HPP

#include "concepts.hpp"
#include "traits.hpp"

#include <iostream>


namespace sadhbhcraft::orderbook
{
    template<OrderLikeConcept OrderLikeType>
    OrderLikeType &&print(OrderLikeType &&o, std::ostream &os = std::cout)
    {
        os << "{ .price=" << price_of(o) << ", .quantity=" << quantity_of(o) << " }" << std::endl;

        return std::forward<OrderLikeType>(o);
    }

} // end of namespace sadhbhcraft::orderbook
#endif // INCLUDED_PRINT_HPP