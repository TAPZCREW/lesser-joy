module;

#include "windows.hpp"

#include <hidport.h>

#include <stormkit/core/contract_macro.hpp>
#include <stormkit/core/try_expected.hpp>

module lesserjoy.hid;

import lesserjoy.wdf;

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
    auto get_report_descriptor(WDFREQUEST& request, const Report_descriptor& descriptor) noexcept -> system_result<void> {
        Try(fill_wdf_request_memory(request, bytes_of(descriptor)));
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto read_report(WDFREQUEST& request, usb::context& usb) -> system_result<void> {
        TryTo(data, get_wdf_request_memory(request));
        auto& ctx = usb.continuous_reader;

        return ctx.last_input_report.read([&request](const auto& report) noexcept {
            return fill_wdf_request_memory(request, array_view { stdr::data(report.buffer), report.size });
        });

        // while (not ctx.sync->stop_token.stop_requested()) {
        //     auto lock = std::unique_lock { ctx.sync->input_report_mutex };

        //    auto       found        = false;
        //    const auto remove_range = stdr::remove_if(ctx.pending_input_reports,
        //                                              [&ctx, &found](const auto& report) mutable noexcept {
        //                                                  if (not found and &ctx.pending_input_reports.front() == &report) {
        //                                                      found = true;
        //                                                      return true;
        //                                                  }
        //                                                  return false;
        //                                              });

        //    if (found) {
        //        auto  report_it = stdr::begin(remove_range);
        //        auto& report    = *report_it;
        //        Try(fill_wdf_request_memory(request, array_view { stdr::data(report.buffer), report.size }));
        //        ctx.pending_input_reports.erase(report_it, stdr::end(remove_range));
        //        break;
        //    }

        //    dlog("Waiting for input report...");

        //    ctx.sync->new_input_report_available.wait(lock);
        // }

        // const auto raw_report = ctx.reports.write([](auto& queue) static noexcept -> std::optional<Raw_input_report> {
        //     if (stdr::empty(queue)) return std::nullopt;

        //    auto data = std::move(queue.front());
        //    queue.pop();
        //    return { std::move(data) };
        // });
        // if (raw_report != std::nullopt)
        //    ilog("{::#x}", array_view<const u8> { std::bit_cast<const u8*>(stdr::data(*raw_report)), stdr::size(*raw_report)
        //    });

        // auto buffer = Raw_input_report {};
        // auto count  = Try(usb::receive_data_sync(ctx, buffer));
        // ilog("hid {} byte(s):\n{}",
        //      stdr::size(data),
        //      array_view<const u8> { std::bit_cast<const u8*>(stdr::data(data)), stdr::size(data) });
        // get last hid report

        // return {};
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
        auto raw_buffer  = PVOID { nullptr };
        auto buffer_size = 0_usize;

        auto string_id = 0_u32;

        CustomLoggedTry(lj::win_call(WdfRequestRetrieveInputBuffer, request, sizeof(u32), &raw_buffer, &buffer_size),
                        dlog,
                        "WdfRequestRetrieveInputBuffer failed!");

        string_id = *std::bit_cast<u32*>(raw_buffer) & 0xFFFF;

        const auto is_serial = (string_id == 16 or string_id == 3); // HID_STRING_ID_ISERIALNUMBER
        ilog("AAAAAA {} {}", product_string, serial_string);

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
