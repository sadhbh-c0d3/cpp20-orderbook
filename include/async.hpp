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

        template<typename T>
        auto operator()(T &x)
        {
            return apply(&x);
        }

    private:
        F f_;

        template<typename T>
        struct awaitable
        {
            F f_;
            T *px_;
        
            bool await_ready() { return false; }
            void await_suspend(std::coroutine_handle<> h)
            {
                f_(*px_);
                h.resume();
            }
            void await_resume() {}
        };
        
        template<typename T>
        auto apply(T *px)
        {
            return awaitable<T>{f_, px};
        }
    };

} // namespace sadhbhcraft::util
#endif//INCLUDED_ASYNC_HPP