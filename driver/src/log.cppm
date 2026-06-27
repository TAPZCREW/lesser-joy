module;

#define WIN32_NO_STATUS
#include <stormkit/core/platform/windows.hpp>
#undef WIN32_NO_STATUS
#include <devpropdef.h>
#include <ntstatus.h>
#include <wdf.h>

#include <stormkit/log/log_macro.hpp>

export module lesserjoy.log;

import std;

import stormkit.core;
import stormkit.log;

using namespace stormkit;
using namespace std::literals;

namespace stdr = std::ranges;

export namespace lj {
    class KernelLogger final: public log::Logger {
      public:
        explicit KernelLogger(LogClock::time_point start) noexcept;
        KernelLogger(LogClock::time_point start, log::Severity log_level) noexcept;

        KernelLogger(const KernelLogger&) noexcept                    = delete;
        auto operator=(const KernelLogger&) noexcept -> KernelLogger& = delete;

        KernelLogger(KernelLogger&&) noexcept                    = delete;
        auto operator=(KernelLogger&&) noexcept -> KernelLogger& = delete;

        ~KernelLogger() noexcept override;

        auto write(log::Severity severity, const log::Module& module, std::string_view string) noexcept -> void override;
        auto flush() noexcept -> void override;

      private:
        auto do_write(log::Severity, const log::Module&, std::string_view) noexcept -> void;
        auto do_write(log::Severity, std::string_view) noexcept -> void;
    };

    IN_MODULE_LOGGER("lesserjoy")
} // namespace lj

namespace lj {
    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    KernelLogger::KernelLogger(LogClock::time_point start) noexcept
        : Logger { std::move(start) } {
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    KernelLogger::KernelLogger(LogClock::time_point start, log::Severity log_level) noexcept
        : Logger { std::move(start), log_level } {
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    inline KernelLogger::~KernelLogger() noexcept = default;

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    inline auto KernelLogger::write(log::Severity severity, const log::Module& module, std::string_view string) noexcept -> void {
        if (stdr::empty(module.name)) do_write(severity, string);
        else
            do_write(severity, module, string);
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    inline auto KernelLogger::flush() noexcept -> void {
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    inline auto KernelLogger::do_write(log::Severity severity, std::string_view string) noexcept -> void {
        const auto now  = LogClock::now();
        const auto time = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_time);

        const auto str = std::format("[{}, {:%S}] {}", severity, time, string);
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, (stdr::data(str)));
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    inline auto KernelLogger::do_write(log::Severity severity, const log::Module& module, std::string_view string) noexcept
      -> void {
        const auto now  = LogClock::now();
        const auto time = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_time);

        const auto str = std::format("[{}, {:%S}] {} {}", severity, time, module.name, string);
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, (stdr::data(str)));
    }
} // namespace lj
