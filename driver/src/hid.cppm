module;

#include "windows.hpp"

#include <hidport.h>

#include <stormkit/core/contract_macro.hpp>
#include <stormkit/core/try_expected.hpp>

export module lesserjoy.hid;

import std;
import frozen;

import stormkit.core;

export import :command_ids;
export import :init_commands;
export import :unknown_0x07_commands;
export import :leds_commands;
export import :feature_select_commands;
export import :unknown_0x11_commands;
export import :bluetooth_pairing_commands;
export import :unknown_0x16_commands;
export import :unknown_0x18_commands;

import lesserjoy.log;
import lesserjoy.constants;
import lesserjoy.common;
import lesserjoy.wdf;
import lesserjoy.usb;

using namespace stormkit;
using namespace stormkit::literals;

namespace stdr = std::ranges;
namespace stdv = std::views;

export namespace lj::hid {
    // template<typename Command, typename... Args>
    // auto send_command(const auto& ctx, Args&&... args) -> system_result<usize>;
    // template<typename Command, Validate VALIDATE = Validate::NO>
    // auto receive_command(auto& ctx) -> system_result<array<byte, Command::REPORT_LENGTH>>;
    // template<typename Command, Validate VALIDATE = Validate::NO, typename... Args>
    // auto send_command_receive_report(auto& ctx, Args&&... args) -> system_result<array<byte, Command::REPORT_LENGTH>>;

    template<typename Command, typename... Args>
    auto send_command(const usb::context& ctx, Args&&... args) -> system_result<void>;
    template<typename Command, typename... Args>
    auto send_command_validate(usb::context& ctx, Args&&... args) -> system_result<array<byte, Command::REPORT_LENGTH>>;

    template<typename Command>
    auto get_command_report(usb::context& ctx) -> system_result<array<byte, Command::REPORT_LENGTH>>;

    namespace ioctl {
        auto get_device_descriptor(WDFREQUEST& request, const HID_DESCRIPTOR& descriptor) noexcept -> system_result<void>;
        auto get_device_attributes(WDFREQUEST& request, const HID_DEVICE_ATTRIBUTES& attributes) noexcept -> system_result<void>;
        auto get_report_descriptor(WDFREQUEST& request, const Report_descriptor& descriptor) noexcept -> system_result<void>;

        auto read_report(WDFREQUEST& request, usb::context& ctx) -> system_result<void>;
        auto write_report(WDFREQUEST& request, array_view<const byte> from) -> system_result<void>;
        auto get_string(WDFREQUEST& request, string_view product_string, string_view serial_string) -> system_result<void>;
        auto get_indexed_string(WDFREQUEST& request, string_view product_string) -> system_result<void>;
    } // namespace ioctl
} // namespace lj::hid

////////////////////////////////////////////////////////////////////
///                      IMPLEMENTATION                          ///
////////////////////////////////////////////////////////////////////

namespace lj::hid {
    ////////////////////////////////////////
    ////////////////////////////////////////
    template<typename Command, typename... Args>
    STORMKIT_FORCE_INLINE
    inline auto send_command(usb::context& ctx, Args&&... args) -> system_result<void> {
        Try(usb::send_data_sync(ctx, Command::make_command(std::forward<Args>(args)...)));
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<typename Command>
    inline auto get_command_report(usb::context& ctx) -> system_result<array<byte, Command::REPORT_LENGTH>> {
        auto report = array<byte, Command::REPORT_LENGTH> {};
        TryTo(report_, usb::get_data_sync(ctx));

        stdr::copy(array_view { stdr::data(report_), Command::REPORT_LENGTH }, stdr::begin(report));

        return { std::move(report) };
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<typename Command, typename... Args>
    STORMKIT_FORCE_INLINE
    inline auto send_command_validate(usb::context& ctx, Args&&... args) -> system_result<array<byte, Command::REPORT_LENGTH>> {
        Try(send_command<Command>(ctx, std::forward<Args>(args)...));
        TryTo(report, (get_command_report<Command>(ctx)));

        if (not Command::validate_report(report)) {
            const auto got      = array_view<const u8> { std::bit_cast<const u8*>(stdr::data(report)),
                                                         stdr::size(Command::REPORT_HEADER) };
            const auto expected = array_view<const u8> { std::bit_cast<const u8*>(stdr::data(Command::REPORT_HEADER)),
                                                         stdr::size(Command::REPORT_HEADER) };
            dlog("Report header bytes mismatch! got: {::#x}, expected: {::#x}!", got, expected);
            return std::unexpected<system_error2::nt_code> { STATUS_UNSUCCESSFUL };
        }

        return { std::move(report) };
    }
} // namespace lj::hid
