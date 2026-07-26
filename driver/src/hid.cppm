module;

#include "windows.hpp"

#include <hidport.h>

export module lesserjoy.hid;

import std;

import stormkit.core;

import lesserjoy.log;
import lesserjoy.constants;

using namespace stormkit;

namespace stdr = std::ranges;

export namespace lj::hid {
    using Report_descriptor = std::span<const u8>;

    struct Control_info {
        u8 report_id;
        u8 control_code;
    };

    struct Input_info {
        u8 report_id;
        u8 data;
    };

    struct Output_info {
        u8  report_id;
        u8  data;
        u16 _;
        u32 _;
    };

    auto get_device_descriptor(WDFREQUEST& request, const HID_DESCRIPTOR& descriptor) noexcept -> NTSTATUS;
    auto get_device_attributes(WDFREQUEST& request, const HID_DEVICE_ATTRIBUTES& attributes) noexcept -> NTSTATUS;
    auto get_report_descriptor(WDFREQUEST& request, const hid::Report_descriptor& descriptor) noexcept -> NTSTATUS;
    // inline constexpr auto OUTPUT_REPORT_SIZE = sizeof(Output_info) - 1;

    inline constexpr auto DEFAULT_REPORT_DESCRIPTOR = to_array<u8>({
      0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
      0x09, 0x05,       // Usage (Game Pad)
      0xA1, 0x01,       // Collection (Application)
      0x85, 0x09,       //   Report ID (9)
      0x05, 0xFF,       //   Usage Page (Reserved 0xFF)
      0x09, 0x01,       //   Usage (0x01)
      0x15, 0x00,       //   Logical Minimum (0)
      0x25, 0xFF,       //   Logical Maximum (-1)
      0x35, 0x00,       //   Physical Minimum (0)
      0x45, 0x00,       //   Physical Maximum (0)
      0x65, 0x00,       //   Unit (None)
      0x55, 0x00,       //   Unit Exponent (0)
      0x75, 0x08,       //   Report Size (8)
      0x95, 0x02,       //   Report Count (2)
      0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x05, 0x09,       //   Usage Page (Button)
      0x19, 0x01,       //   Usage Minimum (0x01)
      0x29, 0x15,       //   Usage Maximum (0x15)
      0x25, 0x01,       //   Logical Maximum (1)
      0x45, 0x01,       //   Physical Maximum (1)
      0x75, 0x01,       //   Report Size (1)
      0x95, 0x15,       //   Report Count (21)
      0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x05, 0x01,       //   Usage Page (Generic Desktop Ctrls)
      0x09, 0x01,       //   Usage (Pointer)
      0xA1, 0x00,       //   Collection (Physical)
      0x95, 0x03,       //    Report Count (3)
      0x81, 0x03,       //    Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x09, 0x30,       //    Usage (X)
      0x26, 0xFF, 0x0F, //    Logical Maximum (4095)
      0x45, 0x00,       //    Physical Maximum (0)
      0x75, 0x0C,       //    Report Size (12)
      0x95, 0x01,       //    Report Count (1)
      0x81, 0x02,       //    Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x09, 0x31,       //    Usage (Y)
      0x81, 0x02,       //    Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x09, 0x33,       //    Usage (Rx)
      0x81, 0x02,       //    Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x09, 0x35,       //    Usage (Rz)
      0x81, 0x02,       //    Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0xC1, 0x00,       //   End Collection
      0x05, 0xFF,       //   Usage Page (Reserved 0xFF)
      0x09, 0x02,       //   Usage (0x02)
      0x25, 0xFF,       //   Logical Maximum (-1)
      0x75, 0x08,       //   Report Size (8)
      0x95, 0x34,       //   Report Count (52)
      0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x85, 0x05,       //   Report ID (5)
      0x09, 0x01,       //   Usage (0x01)
      0x95, 0x3F,       //   Report Count (63)
      0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x85, 0x02,       //   Report ID (2)
      0x09, 0x01,       //   Usage (0x01)
      0x91, 0x02,       //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
      0xC1, 0x00,       // End Collection
    });

    // static_assert(sizeof(DEFAULT_REPORT_DESCRIPTOR) == 0x61);

    inline constexpr auto DEFAULT_DESCRIPTOR = HID_DESCRIPTOR {
        .bLength         = 0x09,
        .bDescriptorType = 0x21, // HID == 0x21
        .bcdHID          = 0x0111,
        .bCountry        = 0x00,
        .bNumDescriptors = 0x01,
        // .DescriptorList  = { { .bReportType = 0x22, .wReportLength = 0x61 } }
        .DescriptorList = { { .bReportType = 0x22, .wReportLength = sizeof(DEFAULT_REPORT_DESCRIPTOR) } }
    };

    // inline constexpr auto
    //   DEFAULT_OUTPUT_REPORT = into_bytes({ 0x01, /* Report ID */
    //                                        0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x27,
    //                                        0x10, 0x00, 0x32, 0xFF, 0x27, 0x10, 0x00, 0x32, 0xFF, 0x27, 0x10, 0x00,
    //                                        0x32, 0xFF, 0x27, 0x10, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });

    inline constexpr auto DEFAULT_OUTPUT_REPORT = into_bytes({
      0x03, /* Initialize command */
      0x91,
      0x00,
      0x0d,
      0x00,
      0x08,
      0x00,
      0x00,
      0x01,
      0x00, /* start HID output at 4ms intervals */
      0xff,
      0xff,
      0xff,
      0xff,
      0xff,
      0xff // MAC address (little endian)
    });

} // namespace lj::hid

module: private;

namespace lj::hid {
    namespace {
        NTSTATUS
        request_copy_from_buffer(WDFREQUEST request, std::span<const byte> from) {
            auto memory = WDFMEMORY {};
            auto status = WdfRequestRetrieveOutputMemory(request, &memory);
            if (not NT_SUCCESS(status)) {
                lj::elog("WdfRequestRetrieveOutputMemory failed : {:#x}", static_cast<u32>(status));
                return status;
            }

            auto output_buffer_extent = 0_usize;
            WdfMemoryGetBuffer(memory, &output_buffer_extent);
            if (output_buffer_extent < stdr::size(from)) {
                status = STATUS_INVALID_BUFFER_SIZE;
                lj::elog("request_copy_from_buffer: buffer too small. Size {}, expects {}\n",
                         output_buffer_extent,
                         stdr::size(from));
                return status;
            }

            status = WdfMemoryCopyFromBuffer(memory, 0, bit_cast<void*>(stdr::data(from)), stdr::size(from));
            if (not NT_SUCCESS(status)) {
                lj::elog("WdfMemoryCopyFromBuffer failed: {:0x}\n", static_cast<u32>(status));
                return status;
            }

            WdfRequestSetInformation(request, stdr::size(from));
            return status;
        }
    } // namespace

    auto get_device_descriptor(WDFREQUEST& request, const HID_DESCRIPTOR& descriptor) noexcept -> NTSTATUS {
        return request_copy_from_buffer(request, as_bytes(descriptor));
    }

    auto get_device_attributes(WDFREQUEST& request, const HID_DEVICE_ATTRIBUTES& attributes) noexcept -> NTSTATUS {
        return request_copy_from_buffer(request, as_bytes(attributes));
    }

    auto get_report_descriptor(WDFREQUEST& request, const Report_descriptor& descriptor) noexcept -> NTSTATUS {
        return request_copy_from_buffer(request, as_bytes(descriptor));
    }
} // namespace lj::hid
