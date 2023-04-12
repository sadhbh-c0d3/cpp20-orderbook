#ifndef INCLUDED_SIMPLE_PARSER_HPP
#define INCLUDED_SIMPLE_PARSER_HPP

#include "util/concepts.hpp"

#include <istream>


namespace sadhbhcraft::exchange {

    template<util::NumberConcept _KeyType>
    class IStreamMsgParser
    {
    public:
        using KeyType = _KeyType;
        using StringType = std::string;
        using IntegerType = int;
        using DecimalType = double;

        IStreamMsgParser(std::istream &is): m_is(is) {}
    
        bool parse_key(KeyType &out_key)
        {
            if (m_is.eof())
            {
                return false;
            }

            m_is >> out_key;
            
            if (auto c = m_is.get(); c != '=')
            {
                return false;
            }

            return true;
        }
        
        bool parse_value(util::NumberConcept auto &out_value)
        {
            if (m_is.eof())
            {
                return false;
            }

            m_is >> out_value;
            
            if (auto c = m_is.get(); c != '\1' && !m_is.eof())
            {
                return false;
            }

            return true;
        }
        
        bool parse_value(std::string &out_value)
        {
            if (m_is.eof())
            {
                return false;
            }

            for (;;)
            {
                auto c = m_is.get();

                if (c == '\1' || m_is.eof())
                {
                    break;
                }
                out_value += c;
            }

            return true;
        }

        bool parse_field(KeyType in_key, auto &out_value)
        {
            if (KeyType key; parse_key(key), key != in_key)
            { 
                return false;
            }

            return parse_value(out_value);
        }

    private:
        std::istream &m_is;
    };


} // end of namespace sadhbhcraft::exchange
#endif//INCLUDED_SIMPLE_PARSER_HPP