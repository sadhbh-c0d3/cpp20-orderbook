#ifndef INCLUDED_GENERATOR_HPP
#define INCLUDED_GENERATOR_HPP

//
// This source has been strongly inspired by CPPReference
// https://en.cppreference.com/w/cpp/language/coroutines
//
// I have added use of storage_trait, which adds support for 
// any data type that cannot be default constructed. Such
// type is then stored using std::optional, which does not
// requie from its value to be constructed at all times.
//
// NOTE: C++23 will have <generator> header with generator<T>
//

#include <coroutine>
#include <exception>

#include "util.hpp"

namespace sadhbhcraft::util
{
    template <typename T>
    struct Generator
    {
        // The class name 'Generator' is our choice and it is not required for coroutine
        // magic. Compiler recognizes coroutine by the presence of 'co_yield' keyword.
        // You can use name 'MyGenerator' (or any other name) instead as long as you include
        // nested struct promise_type with 'MyGenerator get_return_object()' method.
    
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type // required
        {
            typename DefaultConstructibleWrapper<T>::type value_;
            std::exception_ptr exception_;
    
            Generator get_return_object()
            {
                return Generator(handle_type::from_promise(*this));
            }
            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void unhandled_exception() { exception_ = std::current_exception(); } // saving
                                                                                // exception
    
            template <std::convertible_to<T> From> // C++20 concept
            std::suspend_always yield_value(From&& from)
            {
                value_ = std::forward<From>(from); // caching the result in promise
                return {};
            }
            void return_void() { }
        };
    
        handle_type h_;
    
        Generator(handle_type h)
            : h_(h)
        {
        }
        ~Generator() { h_.destroy(); }
        explicit operator bool()
        {
            fill(); // The only way to reliably find out whether or not we finished coroutine,
                    // whether or not there is going to be a next value generated (co_yield)
                    // in coroutine via C++ getter (operator () below) is to execute/resume
                    // coroutine until the next co_yield point (or let it fall off end).
                    // Then we store/cache result in promise to allow getter (operator() below
                    // to grab it without executing coroutine).
            return !h_.done();
        }
        T operator()()
        {
            fill();
            full_ = false; // we are going to move out previously cached
                        // result to make promise empty again

            return std::move(
                DefaultConstructibleWrapper<T>::extract_value(
                    std::forward<typename DefaultConstructibleWrapper<T>::type>(
                        h_.promise().value_)));
        }
    
    private:
        bool full_ = false;
    
        void fill()
        {
            if (!full_)
            {
                h_();
                if (h_.promise().exception_)
                    std::rethrow_exception(h_.promise().exception_);
                // propagate coroutine exception in called context
    
                full_ = true;
            }
        }
    };

} // end of namespace sadhbhcraft::util
#endif//INCLUDED_GENERATOR_HPP