module;

#include "contract_macro.hpp"
#include <stormkit/core/try_expected.hpp>

module lesserjoy;

import :wdf;

using namespace stormkit::literals;

namespace lj::hid::ioctl {
    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_device_descriptor(wdk::WDFREQUEST request, const wdk::HID_DESCRIPTOR& descriptor) noexcept -> system_result<void> {
        Try(fill_wdf_request_memory(request, bytes_of(descriptor)));
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_device_attributes(wdk::WDFREQUEST request, const wdk::HID_DEVICE_ATTRIBUTES& attributes) noexcept
      -> system_result<void> {
        Try(fill_wdf_request_memory(request, bytes_of(attributes)));
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_report_descriptor(wdk::WDFREQUEST request, const report_descriptor& descriptor) noexcept -> system_result<void> {
        Try(fill_wdf_request_memory(request, bytes_of(descriptor)));
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto read_report(wdk::WDFREQUEST request, const transport::usb_context& usb) noexcept -> system_result<void> {
        return usb.write_report_to(request);
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto write_report(wdk::WDFREQUEST request, array_view<const byte> report) noexcept -> system_result<void> {
        Try(fill_wdf_request_memory(request, report));

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto write_report(wdk::WDFREQUEST request, const transport::usb_context& ctx) noexcept -> system_result<void> {
        TryTo(report, get_wdf_request_memory(request));

        // every HID consumer sizes this buffer from HIDP_CAPS::OutputReportByteLength, which the OS
        // derives from the report descriptor we hand out at IOCTL_HID_GET_REPORT_DESCRIPTOR.
        // a HID output report is a fixed-size record: a short write is only a prefix, the controller's
        // parser keeps buffering and the rumble command never completes -- with nothing ever failing.
        if (stdr::size(report) != hid::output_report_size) {
            elog("Output report has unexpected size! got: {}, expects: {} (HIDP_CAPS::OutputReportByteLength)",
                 stdr::size(report),
                 hid::output_report_size);
            return std::unexpected<system_error2::nt_code> { wdk::STATUS_INVALID_BUFFER_SIZE };
        }

        if (report[0] != as<byte>(hid::output_report_id)) {
            elog("Output report id mismatch! got: {:#x}, expects: {:#x}", as<u8>(report[0]), hid::output_report_id);
            return std::unexpected<system_error2::nt_code> { std::in_place, wdk::STATUS_INVALID_PARAMETER };
        }

        // the two HD rumble blocks, @see hid_reports.md#output-report-0x02
        dlog("rumble L {::#x} R {::#x}",
             array_view<const u8> { reinterpret_cast<const u8*>(stdr::data(report)) + hid::rumble::left_offset,
                                    hid::rumble::block_size },
             array_view<const u8> { reinterpret_cast<const u8*>(stdr::data(report)) + hid::rumble::right_offset,
                                    hid::rumble::block_size });

        // bounded so a consumer such as joy.cpl or hidapi sees a real failure instead of a
        // fire-and-forget success that only says the USB queue accepted the request
        Try(ctx.send_data_bounded(report, transport::usb_context::endpoint_type::hid, output_report_timeout_ms));

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_string(wdk::WDFREQUEST request,
                    string_view manufacturer_string,
                    string_view product_string,
                    string_view serial_string) noexcept -> system_result<void> {
        WDK_EXPECTS(product_string[product_string.size()] == '\0');
        WDK_EXPECTS(serial_string[serial_string.size()] == '\0');

        auto raw_buffer  = raw_ptr<void> { nullptr };
        auto buffer_size = 0_usize;

        auto string_id = 0_u32;

        CustomLoggedTry(lj::win_call(wdk::WdfRequestRetrieveInputBuffer, request, sizeof(u32), &raw_buffer, &buffer_size),
                        elog,
                        "WdfRequestRetrieveInputBuffer failed!");

        if (raw_buffer != nullptr and buffer_size != 0) {
            string_id = (*reinterpret_cast<u32*>(raw_buffer)) & 0xFFFF;

            // HID_STRING_ID_IMANUFACTURER / IPRODUCT / ISERIALNUMBER (hidport.h:213-215).
            // answering a manufacturer request with the product string makes launchers and UIs
            // display the wrong device, so map every id explicitly.
            auto string = wstring {};
            switch (string_id) {
                case wdk::HID_STRING_ID_ISERIALNUMBER: string = ascii_to_wide(serial_string); break;
                case wdk::HID_STRING_ID_IMANUFACTURER: string = ascii_to_wide(manufacturer_string); break;
                case wdk::HID_STRING_ID_IPRODUCT: string = ascii_to_wide(product_string); break;
                default:
                    wlog("IOCTL_HID_GET_STRING: unknown string id {:#x} -- falling back to the product string",
                         string_id);
                    string = ascii_to_wide(product_string);
                    break;
            }

            // size + 1 to include \0
            auto string_as_bytes = array_view { reinterpret_cast<const byte*>(stdr::data(string)),
                                                (stdr::size(string) + 1) * sizeof(wchar_t) };

            Try(fill_wdf_request_memory(request, string_as_bytes));
        }

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_wide_string(wdk::WDFREQUEST request, string_view value) noexcept -> system_result<void> {
        WDK_EXPECTS(value[value.size()] == '\0');

        const auto string = ascii_to_wide(value);

        // size + 1 to include \0
        auto string_as_bytes = array_view { reinterpret_cast<const byte*>(stdr::data(string)),
                                            (stdr::size(string) + 1) * sizeof(wchar_t) };

        Try(fill_wdf_request_memory(request, string_as_bytes));

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_indexed_string(wdk::WDFREQUEST request, string_view product_string) noexcept -> system_result<void> {
        return get_wide_string(request, product_string);
    }
} // namespace lj::hid::ioctl
