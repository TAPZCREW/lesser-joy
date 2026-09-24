module;

#include "windows.hpp"

export module lesserjoy.log;

import std;

import stormkit.core;
import stormkit.log;

using namespace stormkit;
using namespace stormkit::literals;
using namespace std::literals;

namespace stdr = std::ranges;

export namespace lj {
    class kernel_logger final: public log::logger {
      public:
        explicit kernel_logger(clock_type::time_point start) noexcept;
        kernel_logger(clock_type::time_point start, log::severity log_level) noexcept;

        kernel_logger(const kernel_logger&) noexcept                    = delete;
        auto operator=(const kernel_logger&) noexcept -> kernel_logger& = delete;

        kernel_logger(kernel_logger&&) noexcept                    = delete;
        auto operator=(kernel_logger&&) noexcept -> kernel_logger& = delete;

        ~kernel_logger() noexcept override;

        auto write(log::severity severity, const log::module& module, string_view string) noexcept -> void override;
        auto flush() noexcept -> void override;

      private:
        auto do_write(log::severity, const log::module&, string_view) noexcept -> void;
        auto do_write(log::severity, string_view) noexcept -> void;
    };

    template<class... Ts>
    auto dlog(std::format_string<Ts...> format, Ts&&... args) noexcept -> void;

    template<class... Ts>
    auto ilog(std::format_string<Ts...> format, Ts&&... args) noexcept -> void;

    template<class... Ts>
    auto wlog(std::format_string<Ts...> format, Ts&&... args) noexcept -> void;

    template<class... Ts>
    auto elog(std::format_string<Ts...> format, Ts&&... args) noexcept -> void;

    template<class... Ts>
    auto flog(std::format_string<Ts...> format, Ts&&... args) noexcept -> void;
} // namespace lj

////////////////////////////////////////////////////////////////////
///                      IMPLEMENTATION                          ///
////////////////////////////////////////////////////////////////////

namespace lj {
    constexpr auto LOG_MODULE = log::module { "lesserjoy" };

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    kernel_logger::kernel_logger(clock_type::time_point start) noexcept
        : log::logger { std::move(start) } {
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    kernel_logger::kernel_logger(clock_type::time_point start, log::severity log_level) noexcept
        : log::logger { std::move(start), log_level } {
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    inline kernel_logger::~kernel_logger() noexcept = default;

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    inline auto kernel_logger::write(log::severity severity, const log::module& module, string_view string) noexcept -> void {
        if (stdr::empty(module.name)) do_write(severity, string);
        else
            do_write(severity, module, string);
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    inline auto kernel_logger::flush() noexcept -> void {
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    inline auto kernel_logger::do_write(log::severity severity, string_view string) noexcept -> void {
        const auto str = std::format("[{}] {}\n", severity, string);
        OutputDebugStringA(stdr::data(str));
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    inline auto kernel_logger::do_write(log::severity severity, const log::module& module, string_view string) noexcept -> void {
        const auto str = std::format("[{}] {}: {}\n", severity, module.name, string);
        OutputDebugStringA(stdr::data(str));
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<class... Ts>                                                   
    STORMKIT_FORCE_INLINE
    inline auto dlog(std::format_string<Ts...> format, Ts&&... args) noexcept -> void {
        LOG_MODULE.dlog(std::move(format), std::forward<Ts>(args)...);
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<class... Ts>                                                   
    STORMKIT_FORCE_INLINE
    inline auto ilog(std::format_string<Ts...> format, Ts&&... args) noexcept -> void {
        LOG_MODULE.ilog(std::move(format), std::forward<Ts>(args)...);
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<class... Ts>                                                   
    STORMKIT_FORCE_INLINE
    inline auto wlog(std::format_string<Ts...> format, Ts&&... args) noexcept -> void {
        LOG_MODULE.wlog(std::move(format), std::forward<Ts>(args)...);
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<class... Ts>                                                   
    STORMKIT_FORCE_INLINE
    inline auto elog(std::format_string<Ts...> format, Ts&&... args) noexcept -> void {
        LOG_MODULE.elog(std::move(format), std::forward<Ts>(args)...);
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<class... Ts>                                                   
    STORMKIT_FORCE_INLINE
    inline auto flog(std::format_string<Ts...> format, Ts&&... args) noexcept -> void {
        LOG_MODULE.flog(std::move(format), std::forward<Ts>(args)...);
    }
} // namespace lj
