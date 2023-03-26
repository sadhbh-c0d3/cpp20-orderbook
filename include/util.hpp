#ifndef INCLUDED_UTIL_HPP 
#define INCLUDED_UTIL_HPP 

#include <optional>
#include <type_traits>


namespace sadhbhcraft::util
{
    template<typename T, typename F>
    concept ArgumentToCallable =
        requires(T &&x) {
            { std::declval<F>()(x) };
    };

    template <typename T>
    struct ValueStorageTrait
    {
        static constexpr int is_stored_by_value = std::is_default_constructible<T>::value;
        using type = typename std::conditional<is_stored_by_value, T, std::optional<T>>::type;

        template<typename X>
        requires std::is_same_v<type, X>
        constexpr static T&& extract_value(X &&obj)
        {
            if constexpr (is_stored_by_value)
            {
                return std::forward<T>(obj);
            }
            else
            {
                // We'll invoke: constexpr T&& std::optional<T>::operator*() && noexcept;
                return std::forward<T>(*obj);
            }
        }
    };

}//end of namespace sadhbhcraft::util
#endif//INCLUDED_UTIL_HPP 