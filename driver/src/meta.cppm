module;

#ifdef __clang__
    #define HAS_BUILTINS
#endif

export module meta;

namespace lj::meta {
    namespace details {
        template<class T>
        struct LazyType {
            using Type = T;
        };

        template<bool, template<class...> typename, typename, typename>
        struct If;

        template<template<class...> typename LazyType, typename Then, typename OrElse>
        struct If<false, LazyType, Then, OrElse> {
            using Type = LazyType<OrElse>::Type;
        };

        template<template<class...> typename LazyType, typename Then, typename OrElse>
        struct If<true, LazyType, Then, OrElse> {
            using Type = LazyType<Then>::Type;
        };

        template<typename T>
        struct RemoveReference {
            using Type = T;
        };

        template<typename T>
        struct RemoveReference<T&> {
            using Type = T;
        };

        template<typename T>
        struct RemoveReference<T&&> {
            using Type = T;
        };

        template<typename T>
        struct RemoveConst {
            using Type = T;
        };

        template<typename T>
        struct RemoveConst<const T> {
            using Type = T;
        };

        template<typename T>
        struct RemoveVolatile {
            using Type = T;
        };

        template<typename T>
        struct RemoveVolatile<volatile T> {
            using Type = T;
        };

        template<typename T>
        struct RemovePointer {
            using Type = T;
        };

        template<typename T>
        struct RemovePointer<T*> {
            using Type = T;
        };

        template<typename T>
        struct AddConst {
            using Type = const T;
        };

        template<typename T>
        struct AddVolatile {
            using Type = volatile T;
        };

        template<typename T>
        struct AddLValueReference {
            using Type = T&;
        };

        template<typename T>
        struct AddRValueReference {
            using Type = T&&;
        };

#ifdef HAS_BUILTINS
        template<typename T>
        static constexpr auto IsConst = __is_const(T);
#else
        template<typename T>
        static constexpr auto IsConst = false;

        template<typename T>
        static constexpr auto IsConst<const T> = true;
#endif

#ifdef HAS_BUILTINS
        template<typename T>
        static constexpr auto IsVolatile = __is_volatile(T);
#else
        template<typename T>
        static constexpr auto IsVolatile = false;

        template<typename T>
        static constexpr auto IsVolatile<volatile T> = true;
#endif

#ifdef HAS_BUILTINS
        template<typename T>
        static constexpr auto IsRValueReference = __is_rvalue_reference(T);
#else
        template<typename T>
        static constexpr auto IsRValueReference = false;

        template<typename T>
        static constexpr auto IsRValueReference<T&&> = true;
#endif

#ifdef HAS_BUILTINS
        template<typename T>
        static constexpr auto IsLValueReference = __is_lvalue_reference(T);
#else
        template<typename T>
        static constexpr auto IsLValueReference = false;

        template<typename T>
        static constexpr auto IsLValueReference<T&> = true;
#endif

#ifdef HAS_BUILTINS
        template<typename T>
        static constexpr auto IsReference = __is_reference(T);
#else
        template<typename T>
        static constexpr auto IsReference = IsLValueReference<T> or IsRValueReference<T>;
#endif

#ifdef HAS_BUILTINS
        template<typename T>
        static constexpr auto IsPointer = __is_pointer(T);
#else
        template<typename T>
        struct IsPointerImpl {
            static constexpr auto Value = false;
        };

        template<typename T>
        struct IsPointerImpl<T*> {
            static constexpr auto Value = true;
        };

        template<typename T>
        static constexpr auto IsPointer = IsPointerImpl<T>::Value;
#endif

#ifdef HAS_BUILTINS
        template<typename T, typename U>
        static constexpr auto Is = __is_same(T, U);
#else
        template<typename T, typename U>
        static constexpr auto Is = false;

        template<typename T>
        static constexpr auto Is<T, T> = true;
#endif

#ifdef HAS_BUILTINS
        template<typename T>
        static constexpr auto Is<T, void> = __is_void(T);
#endif

#ifdef HAS_BUILTINS
        template<typename T>
        static constexpr auto IsTriviallyCopyable = __is_trivially_copyable(T);
#else
        template<typename T>
        static constexpr auto IsTriviallyCopyable = false;
#endif
    } // namespace details

    export {
        // ALGO
        template<bool Cond, typename Then, typename OrElse>
        using If = details::If<Cond, details::LazyType, Then, OrElse>::Type;

        // QUERIES
        template<typename T>
        concept IsCopyable = __is_constructible(T, const T&) or __is_assignable(T, const T&);

        template<typename T>
        concept IsMovable = __is_constructible(T, T&&) or __is_assignable(T, T&&);

        template<typename T>
        concept IsConst = details::IsConst<T>;

        template<typename T>
        concept IsVolatile = details::IsVolatile<T>;

        template<typename T>
        concept IsReference = details::IsReference<T>;

        template<typename T>
        concept IsPointer = details::IsPointer<T>;

        template<typename T>
        concept IsRValueReference = details::IsRValueReference<T>;

        template<typename T>
        concept IsLValueReference = details::IsLValueReference<T>;

        template<typename T>
        concept IsTriviallyCopyable = details::IsTriviallyCopyable<T>;

        template<typename T, typename U>
        concept Is = details::Is<T, U>;

        template<typename T, typename... Us>
        concept IsAnyOf = (Is<T, Us> and ...);

        // MANIPULATION
        template<typename T>
        using ToPlainType = typename details::RemoveConst<
          typename details::RemoveVolatile<typename details::RemoveReference<T>::Type>::Type>::Type;

        template<auto value>
        using PlainTypeOf = ToPlainType<decltype(value)>;

        template<typename T>
        using AddConst = typename details::AddConst<T>::Type;

        template<typename T>
        using AddVolatile = typename details::AddVolatile<T>::Type;

        template<typename T>
        using AddRValueReference = typename details::AddRValueReference<T>::Type;

        template<typename T>
        using AddLValueReference = typename details::AddLValueReference<T>::Type;

        template<typename T>
        using RemoveConst = typename details::RemoveConst<T>::Type;

        template<typename T>
        using RemoveReference = typename details::RemoveReference<T>::Type;

        template<typename T>
        using RemovePointer = typename details::RemovePointer<T>::Type;

        template<typename T>
        using RemoveIndirections = RemovePointer<RemoveReference<T>>;
    }

    namespace details {
        template<typename T, typename U>
        struct ForwardConst {
          private:
            using T1 = RemoveIndirections<T>;
            using U1 = meta::If<IsConst<T1>, typename AddConst<U>::Type, U>;
            using U2 = meta::If<IsVolatile<T1>, typename AddVolatile<U1>::Type, U1>;

          public:
            using Type = U2;
        };

        template<typename T, typename U>
        struct ForwardRef {
          private:
            using U2 = meta::If<IsLValueReference<T>, typename AddLValueReference<U>::Type, U>;
            using U3 = meta::If<IsRValueReference<T>, typename AddRValueReference<U2>::Type, U2>;

          public:
            using Type = U2;
        };
    } // namespace details

    export {
        template<typename T, typename U>
        using ForwardConst = details::ForwardConst<T, U>::Type;

        template<typename T, typename U>
        using ForwardRefTo = details::ForwardRef<T, U>::Type;

        template<typename T, typename U>
        using ForwardLike = ForwardRefTo<T, ForwardConst<T, U>>;
    }
} // namespace lj::meta
