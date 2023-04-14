#include "test_util.hpp"

#include "util/generator_istream.hpp"
#include <iostream>


sadhbhcraft::util::IStreamGenerator<char> generate_text()
{
    co_yield "Hello";
    co_yield "World ";
    co_yield "Beautiful";
    co_yield "Day Today! ";
    co_yield "Tomorrow another day";
    co_return;
}

sadhbhcraft::util::Generator_IStream<char> generate_text_stream()
{
    co_yield "Hello ";
    co_yield "World\n";
    co_yield "Beautiful ";
    co_yield "Day Today!\n";
    co_yield "Tomorrow another day";
    co_return;
}

void test_generate_text_buffer()
{
    auto g = generate_text();
    std::array<char, 5> buf;
    std::string expected[] = {
        "Hello",
        "World",
        " Beau",
        "tiful",
        "Day T",
        "oday!",
        " Tomo",
        "rrow ",
        "anoth",
        "er da",
        "y...."};

    for(auto s : expected)
    {
        buf.fill('.');
        
        assert(g.read(buf.data(), buf.size()));

        std::string_view sv{buf.data(), buf.size()};

        std::cout << sv << std::endl;
        assert(s == sv);
    }
        
    assert(!g.read(buf.data(), buf.size()));
}

void test_generate_text_stream()
{
    auto is = generate_text_stream();
    std::string s;
    std::string expected[] = {
        "Hello",
        "World",
        "Beautiful",
        "Day",
        "Today!",
        "Tomorrow",
        "another",
        "day"};

    for (auto z : expected)
    {
        assert(!is.eof());
        is >> s;
        std::cout << s << std::endl;
        assert(s == z);
    }

    assert(is.eof());
}

int main(int argc, char **argv)
{
    test_generate_text_buffer();
    test_generate_text_stream();
    return 0;
}
