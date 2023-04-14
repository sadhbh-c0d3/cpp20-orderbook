#ifndef INCLUDED_GENERATOR_ISTREAM_HPP
#define INCLUDED_GENERATOR_ISTREAM_HPP

#include "generator.hpp"
#include <cstring>
#include <type_traits>
#include <streambuf>
#include <istream>
#include <array>
#include <list>


namespace sadhbhcraft::util
{
template <
    typename _CharT = std::istream::char_type,
    typename _Traits = std::istream::traits_type,
    size_t _BufferSize = 16>
class IStreamGenerator
{
public:
    static constexpr size_t BufferSize = _BufferSize;
    using CharType = typename std::basic_streambuf<_CharT, _Traits>::char_type;
    using Traits = typename std::basic_streambuf<_CharT, _Traits>::traits_type;
    using SliceType = std::basic_string_view<CharType, Traits>;

    struct promise_type // required
    {
        std::list<std::array<CharType, BufferSize>> m_buffers;
        size_t m_position{BufferSize};
        size_t m_read_position{0};
        std::exception_ptr m_exception;

        IStreamGenerator get_return_object()
        {
            return {HandleType::from_promise(*this)};
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { m_exception = std::current_exception(); } // saving
                                                                               // exception

        std::suspend_always yield_value(SliceType slice)
        {
            auto next = slice.begin();
            auto count = slice.size();

            while (count)
            {
                auto buffer_remaining_space = BufferSize - m_position;
                if (!buffer_remaining_space)
                {
                    m_buffers.push_back({});
                    m_position = 0;
                    buffer_remaining_space = BufferSize;
                }

                auto dest_ptr = m_buffers.back().data() + m_position;
                auto copy_count = std::min(count, buffer_remaining_space);

                std::memcpy(dest_ptr, next, copy_count);
                next += copy_count;
                count -= copy_count;
                m_position += copy_count;
            }

            return {};
        }
        void return_void() {}

        size_t available() const
        {
            size_t num_buffers = m_buffers.size();
            if (!num_buffers)
            {
                return 0;
            }
            else if (num_buffers == 1)
            {
                return m_position - m_read_position;
            }
            else
            {
                return (num_buffers - 1) * BufferSize + m_position - m_read_position;
            }
        }

        void read(CharType *dest_ptr, size_t count)
        {
            auto begin = m_buffers.begin();
            auto end = m_buffers.end();
            auto iter = begin;
            auto last = begin;

            while (iter != end)
            {
                last = iter;
                auto current = iter++;
                auto src_ptr = current->data();
                size_t copy_count;

                if (current == begin)
                {
                    src_ptr += m_read_position;
                    if (iter == end)
                    {
                        copy_count = std::min(count, m_position - m_read_position);
                    }
                    else
                    {
                        copy_count = std::min(count, BufferSize - m_read_position);
                    }
                }
                else
                {
                    m_read_position = 0;
                    if (iter == end)
                    {
                        copy_count = std::min(count, m_position);
                    }
                    else
                    {
                        copy_count = std::min(count, BufferSize);
                    }
                }

                std::memcpy(dest_ptr, src_ptr, copy_count);
                m_read_position += copy_count;
                dest_ptr += copy_count;
                count -= copy_count;
                if (!count)
                {
                    break;
                }
            }

            m_buffers.erase(begin, last);
        }
    };

    using HandleType = std::coroutine_handle<promise_type>;

    IStreamGenerator() {}
    IStreamGenerator(HandleType h) : m_handle(h) {}

    IStreamGenerator(IStreamGenerator &&o) : m_handle(std::exchange(o.m_handle, {})) {}
    IStreamGenerator &operator=(IStreamGenerator &&o)
    {
        std::swap(m_handle, o.m_handle);
        return *this;
    }

    IStreamGenerator(const IStreamGenerator &) = delete;
    IStreamGenerator &operator=(const IStreamGenerator &) = delete;

    ~IStreamGenerator()
    {
        if (m_handle)
        {
            m_handle.destroy();
        }
    }

    size_t read(CharType *dest_ptr, size_t count)
    {
        size_t available = fill(count);
        count = std::min(count, available);
        if (count)
        {
            m_handle.promise().read(dest_ptr, count);
        }
        return count;
    }

private:
    HandleType m_handle;

    size_t fill(size_t count)
    {
        size_t available = 0;
        while (available = m_handle.promise().available(),
               available < count && !m_handle.done())
        {
            m_handle();

            if (m_handle.promise().m_exception)
            {
                std::rethrow_exception(m_handle.promise().m_exception);
            }
        }

        return available;
    }
};

template<typename _CharT, typename _Traits, size_t _BufferSize>
struct Generator_IStreamBuffer : public std::basic_streambuf<_CharT, _Traits>
{
public:
    static constexpr size_t BufferSize = _BufferSize;
    using CharType = typename std::basic_streambuf<_CharT, _Traits>::char_type;
    using Traits = typename std::basic_streambuf<_CharT, _Traits>::traits_type;
    using GeneratorType = IStreamGenerator<CharType, Traits>;

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
        m_valid = m_generator.read(m_buffer.data() + start_offset, bytes_to_read);
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
    using GeneratorType = IStreamGenerator<_CharT, _Traits>;

    Generator_IStream(GeneratorType g)
        : m_streambuffer(std::move(g))
        , std::basic_istream<_CharT, _Traits>(&m_streambuffer)
    {}

    using promise_type = GeneratorType::promise_type;
    
private:
    Generator_IStreamBuffer<_CharT, _Traits, _BufferSize> m_streambuffer;
};



} // namespace sadhbhcraft::util
#endif//INCLUDED_GENERATOR_ISTREAM_HPP