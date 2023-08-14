#ifndef INCLUDED_UTIL_CONCEPTS_HPP
#define INCLUDED_UTIL_CONCEPTS_HPP

#include <concepts>
#include <coroutine>
#include <type_traits>


namespace sadhbhcraft::util
{
    template <typename T>
    concept NumberConcept = std::is_arithmetic<T>::value;

    template <typename T>
    concept AwaitableConcept =
        requires(T x) {
            { x.await_ready() } -> std::convertible_to<bool>;
            { x.await_suspend(std::declval<std::coroutine_handle<>>()) };
            { x.await_resume() };
        };
    
    template<typename T, typename A>
    concept PromiseConcept =
        requires(T x) {
            { x.yield_value(std::declval<A&&>) };
        };

    template<typename T, typename A>
    concept GeneratorConcept =
        requires(T x) {
            PromiseConcept<typename T::promise_type, A>;
        };

    template <typename T, typename A>
    struct IsGenerator
    {
        template<typename X> struct test : std::false_type {};
        template<GeneratorConcept<A> S> struct test<S> : std::true_type {};
        static constexpr bool value = test<typename std::remove_cvref<T>::type>();
    };

    template<typename T, typename A = int>
    concept RangeConcept =
        requires(T x) {
            { *std::declval<T>().begin() } -> std::convertible_to<A>;
            { *std::declval<T>().end() } -> std::convertible_to<A>;
        };

    template<template <typename> class T, typename A = int>
        struct IsRange
        {
            template<typename X> struct test : std::false_type {};
            template<RangeConcept<A> S> struct test<S> : std::true_type {};
            static constexpr bool value = test<T<A>>();
        };

    template<typename T, typename A = int>
    concept RandomStackConcept =
        requires(T x) {
            // Iterate from top down to bottom
            RangeConcept<T, A>;
            // Must be able to emplace at random location
            {  std::declval<T>().emplace(std::declval<T>().begin(), std::declval<A>()) };
            // Must be able to jump to random location
            {  std::declval<T>().end() - std::declval<T>().begin() } -> std::convertible_to<int>;
            // Tell top of the stack
            {  std::declval<T>().front() } -> std::convertible_to<A>;
        };

    template<template <typename> class T, typename A = int>
        struct IsRandomStack
        {
            template<typename X> struct test : std::false_type {};
            template<RandomStackConcept<A> S> struct test<S> : std::true_type {};
            static constexpr bool value = test<T<A>>();
        };

    template<typename T, typename A = int>
    concept QueueConcept =
        requires(T x) {
            // Iterate from first (oldest) to last (most recent) in the queue
            RangeConcept<T, A>;
            // Must be able to emplace at the end (this is FIFO queue)
            {  std::declval<T>().emplace_back(std::declval<A>()) };
            // Tell next in the queue
            {  std::declval<T>().front() } -> std::convertible_to<A>;
        };

    template<template <typename> class T, typename A = int>
        struct IsQueue
        {
            template<typename X> struct test : std::false_type {};
            template<QueueConcept<A> S> struct test<S> : std::true_type {};
            static constexpr bool value = test<T<A>>();
        };

}// end of namespace sadhbhcraft::util
#endif//INCLUDED_UTIL_CONCEPTS_HPP
