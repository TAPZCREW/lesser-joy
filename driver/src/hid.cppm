module;

#include "contract_macro.hpp"
#include <stormkit/core/try_expected.hpp>

export module lesserjoy:hid;

import std;
import frozen;

import stormkit.core;

import stormkit.platform.win32;

import wdk.wdf;
import wdk.hid;

export import :hid.command_ids;
export import :hid.init_commands;
export import :hid.vibration_commands;
export import :hid.unknown_0x07_commands;
export import :hid.leds_commands;
export import :hid.feature_select_commands;
export import :hid.unknown_0x11_commands;
export import :hid.bluetooth_pairing_commands;
export import :hid.unknown_0x16_commands;
export import :hid.unknown_0x18_commands;

import :log;
import :constants;
import :common;
import :wdf;
import :transport.usb;

using namespace stormkit;
using namespace stormkit::literals;

namespace stdr = std::ranges;
namespace stdv = std::views;

export namespace lj::hid::ioctl {
    auto get_device_descriptor(wdk::WDFREQUEST request, const wdk::HID_DESCRIPTOR& descriptor) noexcept -> system_result<void>;
    auto get_device_attributes(wdk::WDFREQUEST request, const wdk::HID_DEVICE_ATTRIBUTES& attributes) noexcept
      -> system_result<void>;
    auto get_report_descriptor(wdk::WDFREQUEST request, const report_descriptor& descriptor) noexcept -> system_result<void>;

    auto read_report(wdk::WDFREQUEST request, const transport::usb_context& ctx) noexcept -> system_result<void>;
    auto write_report(wdk::WDFREQUEST request, array_view<const byte> from) noexcept -> system_result<void>;
    auto write_report(wdk::WDFREQUEST request, const transport::usb_context& ctx) noexcept -> system_result<void>;
    auto get_string(wdk::WDFREQUEST request,
                    string_view manufacturer_string,
                    string_view product_string,
                    string_view serial_string) noexcept -> system_result<void>;
    auto get_indexed_string(wdk::WDFREQUEST request, string_view product_string) noexcept -> system_result<void>;
    auto get_wide_string(wdk::WDFREQUEST request, string_view value) noexcept -> system_result<void>;
} // namespace lj::hid::ioctl
