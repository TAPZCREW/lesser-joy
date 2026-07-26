module;

#include "windows.hpp"

#include <hidport.h>

export module lesserjoy.hid;

import std;

import stormkit.core;

import lesserjoy.log;
import lesserjoy.constants;
import lesserjoy.ntstatus;

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

    auto read_report(WDFREQUEST& request) -> NTSTATUS;

    inline constexpr auto DEFAULT_REPORT_DESCRIPTOR = to_array<u8>({
      0x05, 0x01,                   // Usage Page (Generic Desktop Ctrls)
      0x15, 0x00,                   // Logical Minimum (0)
      0x09, 0x04,                   // Usage (Joystick)
      0xA1, 0x01,                   // Collection (Application)
      0x85, 0x30,                   //   Report ID (48)
      0x05, 0x01,                   //   Usage Page (Generic Desktop Ctrls)
      0x05, 0x09,                   //   Usage Page (Button)
      0x19, 0x01,                   //   Usage Minimum (0x01)
      0x29, 0x0A,                   //   Usage Maximum (0x0A)
      0x15, 0x00,                   //   Logical Minimum (0)
      0x25, 0x01,                   //   Logical Maximum (1)
      0x75, 0x01,                   //   Report Size (1)
      0x95, 0x0A,                   //   Report Count (10)
      0x55, 0x00,                   //   Unit Exponent (0)
      0x65, 0x00,                   //   Unit (None)
      0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x05, 0x09,                   //   Usage Page (Button)
      0x19, 0x0B,                   //   Usage Minimum (0x0B)
      0x29, 0x0E,                   //   Usage Maximum (0x0E)
      0x15, 0x00,                   //   Logical Minimum (0)
      0x25, 0x01,                   //   Logical Maximum (1)
      0x75, 0x01,                   //   Report Size (1)
      0x95, 0x04,                   //   Report Count (4)
      0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x75, 0x01,                   //   Report Size (1)
      0x95, 0x02,                   //   Report Count (2)
      0x81, 0x03,                   //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x0B, 0x01, 0x00, 0x01, 0x00, //   Usage (0x010001)
      0xA1, 0x00,                   //   Collection (Physical)
      0x0B, 0x30, 0x00, 0x01, 0x00, //    Usage (0x010030)
      0x0B, 0x31, 0x00, 0x01, 0x00, //    Usage (0x010031)
      0x0B, 0x32, 0x00, 0x01, 0x00, //    Usage (0x010032)
      0x0B, 0x35, 0x00, 0x01, 0x00, //    Usage (0x010035)
      0x15, 0x00,                   //    Logical Minimum (0)
      0x27, 0xFF, 0xFF, 0x00, 0x00, //    Logical Maximum (65534)
      0x75, 0x10,                   //    Report Size (16)
      0x95, 0x04,                   //    Report Count (4)
      0x81, 0x02,                   //    Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0xC0,                         //   End Collection
      0x0B, 0x39, 0x00, 0x01, 0x00, //   Usage (0x010039)
      0x15, 0x00,                   //   Logical Minimum (0)
      0x25, 0x07,                   //   Logical Maximum (7)
      0x35, 0x00,                   //   Physical Minimum (0)
      0x46, 0x3B, 0x01,             //   Physical Maximum (315)
      0x65, 0x14,                   //   Unit (System: English Rotation, Length: Centimeter)
      0x75, 0x04,                   //   Report Size (4)
      0x95, 0x01,                   //   Report Count (1) Data, Var,  Abs,  No Wrap, Linear, Preferred State, Null State
      0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x05, 0x09,                   //   Usage Page (Button)
      0x19, 0x0F,                   //   Usage Minimum (0x0F)
      0x29, 0x12,                   //   Usage Maximum (0x12)
      0x15, 0x00,                   //   Logical Minimum (0)
      0x25, 0x01,                   //   Logical Maximum (1)
      0x75, 0x01,                   //   Report Size (1)
      0x95, 0x04,                   //   Report Count (4)
      0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x75, 0x08,                   //   Report Size (8)
      0x95, 0x34,                   //   Report Count (52)
      0x81, 0x03,                   //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x06, 0x00, 0xFF,             //   Usage Page (Vendor Defined 0xFF00)
      0x85, 0x21,                   //   Report ID (33)
      0x09, 0x01,                   //   Usage (0x01)
      0x75, 0x08,                   //   Report Size (8)
      0x95, 0x3F,                   //   Report Count (63)
      0x81, 0x03,                   //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x85, 0x81,                   //   Report ID (-127)
      0x09, 0x02,                   //   Usage (0x02)
      0x75, 0x08,                   //   Report Size (8)
      0x95, 0x3F,                   //   Report Count (63)
      0x81, 0x03,                   //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
      0x85, 0x01,                   //   Report ID (1)
      0x09, 0x03,                   //   Usage (0x03)
      0x75, 0x08,                   //   Report Size (8)
      0x95, 0x3F,                   //   Report Count (63)
      0x91, 0x83,                   //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
      0x85, 0x10,                   //   Report ID (16)
      0x09, 0x04,                   //   Usage (0x04)
      0x75, 0x08,                   //   Report Size (8)
      0x95, 0x3F,                   //   Report Count (63)
      0x91, 0x83,                   //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
      0x85, 0x80,                   //   Report ID (-128)
      0x09, 0x05,                   //   Usage (0x05)
      0x75, 0x08,                   //   Report Size (8)
      0x95, 0x3F,                   //   Report Count (63)
      0x91, 0x83,                   //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
      0x85, 0x82,                   //   Report ID (-126)
      0x09, 0x06,                   //   Usage (0x06)
      0x75, 0x08,                   //   Report Size (8)
      0x95, 0x3F,                   //   Report Count (63)
      0x91, 0x83,                   //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
      0xC0,                         // End Collection
    });

    inline constexpr auto DEFAULT_DESCRIPTOR = HID_DESCRIPTOR {
        .bLength         = sizeof(HID_DESCRIPTOR),
        .bDescriptorType = 0x21, // HID == 0x21
        .bcdHID          = 0x0111,
        .bCountry        = 0x00, // UNUSED
        .bNumDescriptors = 0x01,
        // .DescriptorList  = { { .bReportType = 0x22, .wReportLength = 0x61 } }
        .DescriptorList = { { .bReportType = 0x22, .wReportLength = sizeof(DEFAULT_REPORT_DESCRIPTOR) } }
    };

    namespace commands {
        /* start HID output at 4ms intervals */
        inline constexpr auto INIT = into_bytes({
          0x03, /* Initialize command */
          0x91,
          0x00,
          0x0D,
          0x00,
          0x08,
          0x00,
          0x00,
          0x01,
          0x00,
          0xFF,
          0xFF,
          0xFF,
          0xFF,
          0xFF,
          0xFF // MAC address (little endian)
        });

        inline constexpr auto UNKNOWN_COMMAND_0x07 = into_bytes({ 0x07, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 });
        inline constexpr auto UNKNOWN_COMMAND_0x16 = into_bytes({ 0x16, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 });

        inline constexpr auto REQUEST_CONTROLLER_MAC = into_bytes({
          0x15, 0x91, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x00,
          0x00, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Console MAC Address (Little Endian)
          0xFF,                                           // Byte 14 with bit 0 masked off
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF                    // Remainder of Console MAC Address
        });

        inline constexpr auto LTK_REQUEST = into_bytes({ 0x15, 0x91, 0x00, 0x02, 0x00, 0x11, 0x00, 0x00, 0x00,
                                                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // LTK - 16 byte key
                                                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

        inline constexpr auto UNKNOWN_COMMAND_0x15 = into_bytes({ 0x15, 0x91, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00 });
        inline constexpr auto UNKNOWN_COMMAND_0x09
          = into_bytes({ 0x09, 0x91, 0x00, 0x07, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        inline constexpr auto
          IMU_COMMAND_0x02 = into_bytes({ 0x0c, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00 });
        inline constexpr auto UNKNOWN_COMMAND_0x11 = into_bytes({ 0x11, 0x91, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 });
        inline constexpr auto UNKNOWN_COMMAND_0x0A = into_bytes({ 0x0a, 0x91, 0x00, 0x08, 0x00, 0x14, 0x00, 0x00, 0x01, 0xff,
                                                                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x35, 0x00, 0x46,
                                                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        inline constexpr auto
          IMU_COMMAND_0x04 = into_bytes({ 0x0c, 0x91, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00 });
        inline constexpr auto
          ENABLE_HAPTICS = into_bytes({ 0x03, 0x91, 0x00, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00 });
        inline constexpr auto UNKNOWN_COMMAND_0x10 = into_bytes({ 0x10, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 });
        inline constexpr auto UNKNOWN_COMMAND_0x03 = into_bytes({ 0x03, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00 });
        inline constexpr auto UNKNOWN_COMMAND_0x01 = into_bytes({ 0x01, 0x91, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00 });
        inline constexpr auto
          UNKNOWN_COMMAND_0x0A_ALT           = into_bytes({ 0x0a, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0x00 });
        inline constexpr auto SET_PLAYER_LED = into_bytes(
          { 0x09,
            0x91,
            0x00,
            0x07,
            0x00,
            0x08,
            0x00,
            0x00,
            0x01, // LED bitfield - replace 0x00 with desired LED pattern
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00 });

    } // namespace commands

} // namespace lj::hid

module: private;

namespace lj::hid {
    namespace {
        NTSTATUS
        request_copy_from_buffer(WDFREQUEST request, std::span<const byte> from) {
            auto memory = WDFMEMORY {};
            auto status = WdfRequestRetrieveOutputMemory(request, &memory);
            if (not NT_SUCCESS(status)) {
                lj::elog("WdfRequestRetrieveOutputMemory failed : {}", narrow<Ntstatus>(status));
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
                lj::elog("WdfMemoryCopyFromBuffer failed: {}\n", narrow<Ntstatus>(status));
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
