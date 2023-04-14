#ifndef INCLUDED_GENERATOR_ISTREAM_HPP
#define INCLUDED_GENERATOR_ISTREAM_HPP

#include "generator.hpp"
#include <type_traits>
#include <streambuf>
#include <istream>
#include <array>


namespace sadhbhcraft::util
{

template<typename _CharT, typename _Traits, size_t _BufferSize>
struct Generator_IStreamBuffer : public std::basic_streambuf<_CharT, _Traits>
{
public:
    static constexpr size_t BufferSize = _BufferSize;
    using CharType = typename std::basic_streambuf<_CharT, _Traits>::char_type;
    using Traits = typename std::basic_streambuf<_CharT, _Traits>::traits_type;
    using GeneratorType = Generator<CharType>;

    Generator_IStreamBuffer(GeneratorType g): m_generator(std::move(g))
    {
        fill_buffer(0);
    }

    virtual int underflow() override
    {
        fill_buffer(0);
        return (m_valid > 0) ? m_buffer[0] : Traits::eof();
    }

    virtual int uflow() override
    {
        fill_buffer(0);
        this->gbump(1);
        return (m_valid > 0) ? m_buffer[0] : Traits::eof();
    }

    virtual int pbackfail(int c) override
    {
        fill_buffer(1);
        m_buffer[0] = c;
        this->gptr(); // CHECK!
        return c;
    }

private:
    GeneratorType m_generator;
    std::array<CharType, BufferSize> m_buffer;
    int m_valid;

    void fill_buffer(size_t start_offset)
    {
        auto bytes_to_read = BufferSize - start_offset;
        m_valid = 0;
        for (; m_generator && bytes_to_read; ++start_offset, --bytes_to_read)
        {
            m_buffer[start_offset] = std::forward<CharType>(m_generator());
            ++m_valid;
        }
        this->setg(m_buffer.data(), m_buffer.data(), m_buffer.data() + m_valid);
    }
};

template <
    typename _CharT = std::istream::char_type,
    typename _Traits = std::istream::traits_type,
    size_t _BufferSize = 16>
class Generator_IStream : public std::basic_istream<_CharT, _Traits>
{
public:
    static constexpr size_t BufferSize = _BufferSize;
    using GeneratorType = Generator<_CharT>;

    Generator_IStream(GeneratorType g)
        : m_streambuffer(std::move(g))
        , std::basic_istream<_CharT, _Traits>(&m_streambuffer)
    {}
    
private:
    Generator_IStreamBuffer<_CharT, _Traits, _BufferSize> m_streambuffer;
};

} // namespace sadhbhcraft::util
#endif//INCLUDED_GENERATOR_ISTREAM_HPP