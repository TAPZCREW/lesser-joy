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
    auto init_context(Device_context& ctx, WDFDEVICE) noexcept -> Expected<void>;
    auto event_device_entry(Device_context& ctx) noexcept -> Expected<void>;
    auto event_device_exit(Device_context& ctx) noexcept -> Expected<void>;

    auto send_data(Context& ctx, array_view<const byte> payload) noexcept -> Expected<void>;
    auto send_data_sync(const Context& ctx, array_view<const byte> payload) noexcept -> Expected<void>;

    auto get_data_sync(const usb::Context& usb) noexcept -> Expected<hid::Command_report_buffer>;

    auto send_control_request(const Context&         ctx,
                              byte                   request,
                              byte                   value,
                              byte                   index = 0x00_b,
                              array_view<const byte> data  = {}) noexcept -> Expected<void>;
} // namespace lj::usb
