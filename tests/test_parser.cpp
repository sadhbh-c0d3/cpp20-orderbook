#include "test_util.hpp"

#include "exchange/istream_msg_parser.hpp"

#include "exchange/msg_header.hpp"
#include "exchange/msg_new_order_single.hpp"

#include <sstream>
#include <iostream>
#include <cmath>
#include <limits>


void test_parse_msg_header()
{
    namespace sce = sadhbhcraft::exchange;

    std::stringstream ss{"8=FIX.4.2" "\1" "9=178" "\1" "35=8" "\1" "49=PHLX" "\1" "56=PERS" };

    sce::IStreamMsgParser<int> parser{ss};

    sce::MsgHeader<std::remove_cvref_t<decltype(parser)>> header;

    assert(header.parse_message(parser));

    std::cout
        << "FixVersion: " << header.fix_version << ", "
        << "Sender ID: " << header.sender_comp_id << ", "
        << "Target ID: " << header.target_comp_id << ", "
        << "BodyLen: " << header.body_length << ", "
        << "MsgType: " << header.msg_type
        << std::endl;

    assert(header.fix_version == "FIX.4.2");
    assert(header.sender_comp_id == "PHLX");
    assert(header.target_comp_id == "PERS");
    assert(header.body_length == 178);
    assert(header.msg_type == 8);
}

void test_parse_new_order_single()
{
    namespace sce = sadhbhcraft::exchange;

    std::stringstream ss{
        "11=000-001-001" "\1"
        "40=2" "\1"
        "44=75.250" "\1"
        "38=50.500" "\1"
        "54=2" "\1"
        "60=20230412-18:10:00.000" "\1"
        "55=BTCETH" "\1"
        "59=1"
    };

    sce::IStreamMsgParser<int> parser{ss};

    sce::NewOrderSingle<std::remove_cvref_t<decltype(parser)>> new_order;

    assert(new_order.parse_message(parser));

    std::cout << "ClOrdID: " << new_order.cl_ord_id
        << ", TransactTime: " << new_order.transact_time
        << ", Symbol: " << new_order.symbol
        << ", OrdType: " << new_order.ord_type
        << ", Side: " << new_order.side
        << ", TimeInForce: " << new_order.time_in_force
        << ", Price: " << new_order.price
        << ", OrderQty: " << new_order.order_qty
        << std::endl;

    assert(new_order.cl_ord_id == "000-001-001");
    assert(new_order.transact_time == "20230412-18:10:00.000");
    assert(new_order.symbol == "BTCETH");
    assert(new_order.ord_type == 2);
    assert(new_order.side == 2);
    assert(new_order.time_in_force == 1);
    assert(std::fabs(new_order.price - 75.25) < std::numeric_limits<decltype(new_order.price)>::epsilon());
    assert(std::fabs(new_order.order_qty - 50.5) < std::numeric_limits<decltype(new_order.order_qty)>::epsilon());
}

int main(int argc, char **argv)
{
    test_parse_msg_header();
    test_parse_new_order_single();
    return 0;
}