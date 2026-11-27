#ifndef LESSERJOY_CONTRACT_MACRO
#define LESSERJOY_CONTRACT_MACRO

import std;

import stormkit.core;
import wdk.ntstatus;

#include <stormkit/core/macro_utils.hpp>
#include <stormkit/core/platform_macro.hpp>

STORMKIT_FORCE_INLINE STORMKIT_CONST
constexpr auto as_string(stormkit::assert_type value) noexcept -> std::string_view {
    using enum stormkit::assert_type;
    switch (value) {
        case assertion: return "Contract check";
        case pre_condition: return "Pre condition check";
        case post_condition: return "Post condition check";
        default: break;
    };

    std::unreachable();
}

#define WDK_CONTRACT_PRINT(cond, type)                                                         \
    do {                                                                                       \
        static constexpr auto location_assert = std::source_location::current();               \
        static constexpr auto message_assert  = std::string_view { STORMKIT_STRINGIFY(cond) }; \
        static const auto     str_assert      = std::                                          \
          format("[ASSERTION] {} failed in \n"                                                 \
                 "    > file:     {}:{}:{}\n"                                                  \
                 "      function: {}\n"                                                        \
                 "      reason:   {}",                                                         \
                 as_string(type),                                                              \
                 location_assert.file_name(),                                                  \
                 location_assert.line(),                                                       \
                 location_assert.column(),                                                     \
                 location_assert.function_name(),                                              \
                 message_assert);                                                              \
        win32::OutputDebugStringA(stdr::data(str_assert));                                     \
    } while (false)

#if defined(LESSERJOY_DEBUG_MODE)
import stormkit.platform.win32;

    #include <version>

    #if defined(__cpp_lib_debugging) and __cpp_lib_debugging >= 202311L
        #define WDK_BREAKPOINT_TERMINATE          \
            do {                                  \
                if (std::is_debugger_present()) { \
                    std::breakpoint();            \
                } else                            \
                    std::terminate();             \
            } while (false)
    #else
        #define WDK_BREAKPOINT_TERMINATE \
            do { std::terminate(); } while (false)
    #endif

    #define WDK_ASSERT_IMPL(cond)                                               \
        do {                                                                    \
            if consteval {                                                      \
                stormkit::core::assert(cond);                                   \
            } else {                                                            \
                if (not(cond)) {                                                \
                    WDK_CONTRACT_PRINT(cond, stormkit::assert_type::assertion); \
                    WDK_BREAKPOINT_TERMINATE;                                   \
                }                                                               \
            }                                                                   \
        } while (false)

    #define WDK_EXPECTS_IMPL(cond)                                                  \
        do {                                                                        \
            if consteval {                                                          \
                stormkit::core::expects(cond);                                      \
            } else {                                                                \
                if (not(cond)) {                                                    \
                    WDK_CONTRACT_PRINT(cond, stormkit::assert_type::pre_condition); \
                    WDK_BREAKPOINT_TERMINATE;                                       \
                }                                                                   \
            }                                                                       \
        } while (false)

    #define WDK_ENSURES_IMPL(cond)                                                   \
        do {                                                                         \
            if consteval {                                                           \
                stormkit::core::ensures(cond);                                       \
            } else {                                                                 \
                if (not(cond)) {                                                     \
                    WDK_CONTRACT_PRINT(cond, stormkit::assert_type::post_condition); \
                    WDK_BREAKPOINT_TERMINATE;                                        \
                }                                                                    \
            }                                                                        \
        } while (false)

    #define WDK_ASSERT_BASE_VOID(cond)      WDK_ASSERT_IMPL(cond)
    #define WDK_ASSERT_BASE_RETURN(cond, _) WDK_ASSERT_IMPL(cond)
    #define WDK_ASSERT_BASE(cond, _)        WDK_ASSERT_IMPL(cond)

    #define WDK_EXPECTS_BASE_VOID(cond)      WDK_EXPECTS_IMPL(cond)
    #define WDK_EXPECTS_BASE_RETURN(cond, _) WDK_EXPECTS_IMPL(cond)
    #define WDK_EXPECTS_BASE(cond, _)        WDK_EXPECTS_IMPL(cond)

    #define WDK_ENSURES_BASE_VOID(cond)      WDK_ENSURES_IMPL(cond)
    #define WDK_ENSURES_BASE_RETURN(cond, _) WDK_ENSURES_IMPL(cond)
    #define WDK_ENSURES_BASE(cond, _)        WDK_ENSURES_IMPL(cond)
#else
    #define WDK_ASSERT_BASE_RETURN(cond, ret)                                   \
        do {                                                                    \
            if consteval {                                                      \
                stormkit::core::assert(cond);                                   \
            } else {                                                            \
                if (not(cond)) {                                                \
                    WDK_CONTRACT_PRINT(cond, stormkit::assert_type::assertion); \
                    return ret;                                                 \
                }                                                               \
            }                                                                   \
        } while (false)
    #define WDK_ASSERT_BASE_VOID(cond)                                          \
        do {                                                                    \
            if consteval {                                                      \
                stormkit::core::assert(cond);                                   \
            } else {                                                            \
                if (not(cond)) {                                                \
                    WDK_CONTRACT_PRINT(cond, stormkit::assert_type::assertion); \
                    return;                                                     \
                }                                                               \
            }                                                                   \
        } while (false)
    #define WDK_ASSERT_BASE(cond, status) \
        WDK_ASSERT_BASE_RETURN(cond, { std::unexpected { stormkit::error_code::from_ntstatus(status) } })

    #define WDK_EXPECTS_BASE_RETURN(cond, ret)                                      \
        do {                                                                        \
            if consteval {                                                          \
                stormkit::core::expects(cond);                                      \
            } else {                                                                \
                if (not(cond)) {                                                    \
                    WDK_CONTRACT_PRINT(cond, stormkit::assert_type::pre_condition); \
                    return ret;                                                     \
                }                                                                   \
            }                                                                       \
        } while (false)
    #define WDK_EXPECTS_BASE_VOID(cond)                                             \
        do {                                                                        \
            if consteval {                                                          \
                stormkit::core::expects(cond);                                      \
            } else {                                                                \
                if (not(cond)) {                                                    \
                    WDK_CONTRACT_PRINT(cond, stormkit::assert_type::pre_condition); \
                    return;                                                         \
                }                                                                   \
            }                                                                       \
        } while (false)
    #define WDK_EXPECTS_BASE(cond, status) \
        WDK_EXPECTS_BASE_RETURN(cond, { std::unexpected { stormkit::error_code::from_ntstatus(status) } })

    #define WDK_ENSURES_BASE_RETURN(cond, ret)                                      \
        do {                                                                        \
            if consteval {                                                          \
                stormkit::core::ensures(cond);                                      \
            } else {                                                                \
                if (not(cond)) {                                                    \
                    WDK_CONTRACT_PRINT(cond, stormkit::assert_type::pre_condition); \
                    return ret;                                                     \
                }                                                                   \
            }                                                                       \
        } while (false)
    #define WDK_ENSURES_BASE_VOID(cond)                                             \
        do {                                                                        \
            if consteval {                                                          \
                stormkit::core::ensures(cond);                                      \
            } else {                                                                \
                if (not(cond)) {                                                    \
                    WDK_CONTRACT_PRINT(cond, stormkit::assert_type::pre_condition); \
                    return;                                                         \
                }                                                                   \
            }                                                                       \
        } while (false)
    #define WDK_ENSURES_BASE(cond, status) \
        WDK_ENSURES_BASE_RETURN(cond, { std::unexpected { stormkit::error_code::from_ntstatus(status) } })
#endif

#define WDK_ASSERT_VOID(cond)        WDK_ASSERT_BASE_VOID(cond)
#define WDK_ASSERT(cond)             WDK_ASSERT_BASE(cond, wdk::STATUS_ASSERTION_FAILURE)
#define WDK_ASSERT_RETURN(cond, ret) WDK_ASSERT_BASE_RETURN(cond, ret)
#define WDK_ASSERT_NTSTATUS(cond)    WDK_ASSERT_BASE_RETURN(cond, wdk::STATUS_ASSERTION_FAILURE)

#define WDK_EXPECTS_VOID(cond)        WDK_EXPECTS_BASE_VOID(cond)
#define WDK_EXPECTS(cond)             WDK_EXPECTS_BASE(cond, wdk::STATUS_ASSERTION_FAILURE)
#define WDK_EXPECTS_RETURN(cond, ret) WDK_EXPECTS_BASE_RETURN(cond, ret)
#define WDK_EXPECTS_NTSTATUS(cond)    WDK_EXPECTS_BASE_RETURN(cond, wdk::STATUS_ASSERTION_FAILURE)

#define WDK_ENSURES_VOID(cond)        WDK_ENSURES_BASE_VOID(cond)
#define WDK_ENSURES(cond)             WDK_ENSURES_BASE(cond, wdk::STATUS_ASSERTION_FAILURE)
#define WDK_ENSURES_RETURN(cond, ret) WDK_ENSURES_BASE_RETURN(cond, ret)
#define WDK_ENSURES_NTSTATUS(cond)    WDK_ENSURES_BASE_RETURN(cond, wdk::STATUS_ASSERTION_FAILURE)

#endif
