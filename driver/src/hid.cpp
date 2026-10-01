module;

#include "windows.hpp"

#include <hidport.h>

#include <stormkit/core/contract_macro.hpp>
#include <stormkit/core/try_expected.hpp>

module lesserjoy.hid;

import lesserjoy.wdf;

using namespace stormkit::literals;

namespace lj::hid::ioctl {
    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_device_descriptor(WDFREQUEST& request, const HID_DESCRIPTOR& descriptor) noexcept -> system_result<void> {
        Try(fill_wdf_request_memory(request, bytes_of(descriptor)));
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_device_attributes(WDFREQUEST& request, const HID_DEVICE_ATTRIBUTES& attributes) noexcept -> system_result<void> {
        Try(fill_wdf_request_memory(request, bytes_of(attributes)));
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_report_descriptor(WDFREQUEST& request, const report_descriptor& descriptor) noexcept -> system_result<void> {
        Try(fill_wdf_request_memory(request, bytes_of(descriptor)));
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto read_report(WDFREQUEST& request, const usb::context& usb) -> system_result<void> {
        const auto& ctx = usb.continuous_reader;

        return ctx.last_input_report.read([&request](const auto& report) noexcept {
            return fill_wdf_request_memory(request, array_view { stdr::data(report.buffer), report.size });
        });

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto write_report(WDFREQUEST& request, array_view<const byte> report) -> system_result<void> {
        Try(fill_wdf_request_memory(request, report));

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_string(WDFREQUEST& request, string_view product_string, string_view serial_string) -> system_result<void> {
        auto raw_buffer  = raw_ptr<void> { nullptr };
        auto buffer_size = 0_usize;

        auto string_id = 0_u32;

        CustomLoggedTry(lj::win_call(WdfRequestRetrieveInputBuffer, request, sizeof(u32), &raw_buffer, &buffer_size),
                        dlog,
                        "WdfRequestRetrieveInputBuffer failed!");

        string_id = (*reinterpret_cast<u32*>(raw_buffer)) & 0xFFFF;

        const auto is_serial = (string_id == 16 or string_id == 3); // HID_STRING_ID_ISERIALNUMBER
        ilog("{} {}", product_string, serial_string);

        if (is_serial) {
            Try(fill_wdf_request_memory(request, bytes_of(serial_string)));
        } else {
            Try(fill_wdf_request_memory(request, bytes_of(product_string)));
        }

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_indexed_string(WDFREQUEST& request, string_view product_string) -> system_result<void> {
        Try(fill_wdf_request_memory(request, bytes_of(product_string)));

        return {};
    }
} // namespace lj::hid::ioctl
