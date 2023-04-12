#ifndef INCLUDED_MSG_NEW_ORDER_SINGLE_HPP
#define INCLUDED_MSG_NEW_ORDER_SINGLE_HPP

#include "exchange/concepts.hpp"


namespace sadhbhcraft::exchange {

    template <MsgParserConcept MsgParserType>
    struct NewOrderSingle
    {
        using KeyType = typename MsgParserType::KeyType;
        using StringType = typename MsgParserType::StringType;
        using IntegerType = typename MsgParserType::IntegerType;
        using DecimalType = typename MsgParserType::DecimalType;
        
        static constexpr KeyType ClOrdID = 11;
        static constexpr KeyType OrdType = 40; // 1 MKT, 2 LMT
        static constexpr KeyType Price = 44;
        static constexpr KeyType OrderQty = 38;
        static constexpr KeyType Side = 54; // 1 BUY, 2 SELL
        static constexpr KeyType TransactTime = 60;
        static constexpr KeyType Symbol = 55;
        static constexpr KeyType TimeInForce = 59; //1 GTC, 3 IOC

        unsigned
            cl_ord_id_bit : 1 = {0},
            transact_time_bit : 1 = {0},
            symbol_bit : 1 = {0},
            ord_type_bit : 1 = {0},
            side_bit : 1 = {0},
            time_in_force_bit : 1 = {0},
            price_bit : 1 = {0},
            order_qty_bit : 1 = {0};

        StringType cl_ord_id;
        StringType transact_time;
        StringType symbol;
        IntegerType ord_type;
        IntegerType side;
        IntegerType time_in_force;
        DecimalType price;
        DecimalType order_qty;


        template<typename P>
        bool parse_message(P &&parser) requires std::is_same_v<std::remove_cvref_t<P>, MsgParserType>
        {
            for (;;)
            {
                KeyType key;
                if (!parser.parse_key(key))
                {
                    break;
                }

                switch (key)
                {
                case ClOrdID:
                    cl_ord_id_bit = parser.parse_value(cl_ord_id);
                    break;
                case OrdType:
                    ord_type_bit = parser.parse_value(ord_type);
                    break;
                case Price:
                    price_bit = parser.parse_value(price);
                    break;
                case OrderQty:
                    order_qty_bit = parser.parse_value(order_qty);
                    break;
                case Side:
                    side_bit = parser.parse_value(side);
                    break;
                case TransactTime:
                    transact_time_bit = parser.parse_value(transact_time);
                    break;
                case Symbol:
                    symbol_bit = parser.parse_value(symbol);
                    break;
                case TimeInForce:
                    time_in_force_bit = parser.parse_value(time_in_force);
                    break;
                }
            }

            return cl_ord_id_bit &&
                   transact_time_bit &&
                   symbol_bit &&
                   ord_type_bit &&
                   side_bit &&
                   time_in_force_bit &&
                   price_bit &&
                   order_qty_bit;
        }
    };

} // end of namespace sadhbhcraft::exchange
#endif//INCLUDED_MSG_NEW_ORDER_SINGLE_HPP