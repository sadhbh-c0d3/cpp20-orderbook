#ifndef INCLUDED_MSG_HEADER_HPP
#define INCLUDED_MSG_HEADER_HPP

#include "exchange/concepts.hpp"


namespace sadhbhcraft::exchange {

    template <MsgParserConcept MsgParserType>
    struct MsgHeader
    {
        using KeyType = typename MsgParserType::KeyType;
        using StringType = typename MsgParserType::StringType;
        using IntegerType = typename MsgParserType::IntegerType;
        
        static constexpr KeyType FixVersion = 8;
        static constexpr KeyType BodyLength = 9;
        static constexpr KeyType MsgType = 35;
        static constexpr KeyType SenderCompId = 49;
        static constexpr KeyType TargetCompId = 56;
        static constexpr KeyType ApplVerID = 1128;
        static constexpr KeyType Checksum = 10;
        
        StringType fix_version;
        StringType sender_comp_id;
        StringType target_comp_id;
        IntegerType body_length;
        IntegerType msg_type;

        template<typename P>
        bool parse_message(P &&parser) requires std::is_same_v<std::remove_cvref_t<P>, MsgParserType>
        {
            // These fields must be in this exact order
            return parser.parse_field(FixVersion, fix_version) &&
                   parser.parse_field(BodyLength, body_length) &&
                   parser.parse_field(MsgType, msg_type) &&
                   parser.parse_field(SenderCompId, sender_comp_id) &&
                   parser.parse_field(TargetCompId, target_comp_id);
        }
    };

} // end of namespace sadhbhcraft::exchange
#endif//INCLUDED_MSG_HEADER_HPP