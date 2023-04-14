#include "test_util.hpp"

#include "util/generator_istream.hpp"
#include <iostream>


sadhbhcraft::util::Generator<char> generate_text()
{
    co_yield 'H';
    co_yield 'e';
    co_yield 'l';
    co_yield 'l';
    co_yield 'o';
    co_return;
}


void test_generator_istream()
{
    namespace scu = sadhbhcraft::util;

    scu::Generator_IStream is{generate_text()};

    std::string text;
    std::cout << (is >> text, text) << std::endl;
}

int main(int argc, char **argv)
{
    test_generator_istream();
    return 0;
}
