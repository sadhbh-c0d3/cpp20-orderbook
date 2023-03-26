#ifndef INCLUDED_ASYNC_HPP
#define INCLUDED_ASYNC_HPP

#include <coroutine>
#include <utility>

// These are some basic things we can co_await on
//
//      SomePolicy policy;
//      SomeArg arg;
//
//      co_await policy(arg);
//
// Where SomePolicy could be:
// - AsyncNoop      -- which does nothing, and co_await immediately returns
// - AsyncImmediate -- which applies function to argument and returns immediately
//
// ...something else user-defined could be true async, e.g. send request to a
// micro-service and await response (this is not in the scope)


#include "util.hpp"

namespace sadhbhcraft::util
{
    struct AsyncNoop
    {
        template<typename T>
        std::suspend_never operator()(T &&oq) { return {}; }
    };

    template<typename F>
    struct AsyncImmediate
    {
        AsyncImmediate(F f): f_(std::move(f))
        {}

        template<ArgumentToCallable<F> T>
        auto operator()(T &&x)
        {
            return apply(std::forward<T>(x));
        }

    private:
        F f_;

        template<typename T>
        struct awaitable
        {
            awaitable(F f, T x): f_(std::move(f)), x_(std::move(x))
            {}

            bool await_ready() { return false; }
            void await_suspend(std::coroutine_handle<> h)
            {
                f_(std::forward<T>(x_));

                h.resume();
            }
            void await_resume() {}

        private:
            F f_;
            T x_;
        
        };
        
        template<typename T>
        auto apply(T &&x)
        {
            return awaitable<T>{f_, std::forward<T>(x)};
        }
    };

} // namespace sadhbhcraft::util
#endif//INCLUDED_ASYNC_HPP