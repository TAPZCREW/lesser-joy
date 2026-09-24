module;

#include "windows.hpp"

#include "usb.hpp"

export module lesserjoy.usb;

import std;

import stormkit.core;

import lesserjoy.wdf;
import lesserjoy.common;

using namespace stormkit;
using namespace stormkit::literals;

namespace stdr = std::ranges;

export namespace lj::usb {
    auto init_context(device_context& ctx, WDFDEVICE) noexcept -> system_result<void>;
    auto event_device_entry(device_context& ctx) noexcept -> system_result<void>;
    auto event_device_exit(device_context& ctx) noexcept -> system_result<void>;

    auto send_data(context& ctx, array_view<const byte> payload) noexcept -> system_result<void>;
    auto send_data_sync(const context& ctx, array_view<const byte> payload) noexcept -> system_result<void>;

    auto get_data_sync(const usb::context& usb) noexcept -> system_result<hid::command_report_buffer>;

    auto send_control_request(const context&         ctx,
                              byte                   request,
                              byte                   value,
                              byte                   index = 0x00_b,
                              array_view<const byte> data  = {}) noexcept -> system_result<void>;
} // namespace lj::usb
