module;

#include <string.h>

export module utilities;

import meta;

export namespace lj {
    struct in_place_t {
    } in_place;

    enum class byte : char {
    };

    template<typename T>
    constexpr auto as_const(T& value) -> const T&;

    template<typename T>
    constexpr auto as_const(T&& value) -> const T&&;

    template<typename T>
    constexpr auto launder(T* value) -> T*;

    template<typename T>
    constexpr auto move(T&& value) -> meta::RemoveReference<T>&&;

    template<typename T>
    constexpr auto forward(meta::RemoveReference<T>& value) -> T&&;

    template<typename T>
    constexpr auto forward(meta::RemoveReference<T>&& value) -> T&&;

    template<typename T, typename U>
    constexpr auto forward_like(U&& value) -> auto&&;

    template<class To, class From>
        requires(sizeof(To) == sizeof(From))
    constexpr auto bit_cast(const From& src) noexcept -> To;

    template<typename T, typename U>
        requires(not meta::IsConst<T>)
    constexpr auto exchange(T& first, U&& second) -> decltype(auto);

    template <class Dispose>
    struct DisposeHandler {
        Dispose dispose;
        ~DisposeHandler() { dispose(); }
    };
} // namespace lj

namespace lj {
    template<typename T>
    constexpr auto as_const(T& value) -> const T& {
        return value;
    }

    template<typename T>
    constexpr auto as_const(T&& value) -> const T&& {
        return value;
    }

    template<typename T>
    constexpr auto launder(T* value) -> T* {
        return value;
    }

    template<typename T>
    constexpr auto move(T&& value) -> meta::RemoveReference<T>&& {
        return static_cast<meta::RemoveReference<T>&&>(value);
    }

    template<typename T>
    constexpr auto forward(meta::RemoveReference<T>& value) -> T&& {
        return static_cast<T&&>(value);
    }

    template<typename T>
    constexpr auto forward(meta::RemoveReference<T>&& value) -> T&& {
        return static_cast<T&&>(value);
    }

    template<typename T, typename U>
    constexpr auto forward_like(U&& value) -> auto&& {
        constexpr auto is_adding_const = meta::IsConst<meta::RemoveReference<T>>;

        if constexpr (meta::IsLValueReference<T&&>) {
            if constexpr (is_adding_const) return as_const(value);
            else
                return static_cast<meta::RemoveReference<U>&>(value);
        } else {
            if constexpr (is_adding_const) return move(as_const(value));
            else
                return move(value);
        }
    }

    template<class To, class From>
        requires(sizeof(To) == sizeof(From))
    constexpr auto bit_cast(const From& src) noexcept -> To {
        auto dst = To {};
        memcpy(&dst, &src, sizeof(To));
        return dst;
    }

    template<typename T, typename U>
        requires(not meta::IsConst<T>)
    constexpr auto exchange(T& first, U&& second) -> decltype(auto) {
        if constexpr (meta::IsMovable<T>) {
            auto out = move(first);

            first = forward<U>(second);

            return out;
        } else {
            auto out = auto(first);

            first = forward<U>(second);

            return out;
        }
    }
} // namespace lj
