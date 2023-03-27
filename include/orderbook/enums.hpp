#ifndef INCLUDED_ENUMS_HPP
#define INCLUDED_ENUMS_HPP


namespace sadhbhcraft::orderbook
{
    enum class Side
    {
        Buy,    // aka Bid, crosses (matches) Ask
        Sell    // aka Ask, crosses (matches) Bid
    };

    enum class OrderType
    {
        Market, // Take order(s) from opposite side to fully fill quantity requested
        Limit,  // Place order on the book, cross and execute up to requested price level
        IOC,    // Take order(s) from opposite side up to requested price level
        FOC     // Take order(s) from opposite side only up to requested price level,
                // and only if can fully fill quantity requested
    };

}; // end of namespace
#endif//INCLUDED_ENUMS_HPP