module;

#include "windows.hpp"

#include <hidport.h>

export module lesserjoy.constants;

import std;
import frozen;

import stormkit.core;

using namespace stormkit;
using namespace stormkit::literals;

export namespace lj {
    struct Controller_type {
        string_view name;
        string_view vendor;
        u16         vid;
        u16         pid;
        string_view product_string;
        string_view manufacturer_string;
    };

    enum class Direction : u8 {
        HOST_TO_DEVICE = 0x91,
        DEVICE_TO_HOST = 0x01,
    };

    enum class Transport : u8 {
        USB       = 0x00,
        BLUETOOTH = 0x01,
    };

    enum class Validate {
        YES,
        NO,
    };

    inline constexpr auto DEV_INTERFACE_HID_GUID = GUID {
        0x4D1E55B2,
        0xF16F,
        0x11CF,
        { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 }
    };

    inline constexpr auto DRIVER_POOL_TAG  = u32 { 'lyoj' };
    inline constexpr auto DRIVER_MAX_SLOTS = 8_usize;

    inline constexpr auto CONTROLLERS_TYPE = frozen::unordered_map<frozen::string, Controller_type, 1> {
        { "pro_controller",
         { .name                = "Nintendo Switch 2 Pro Controller",
            .vendor              = "Nintendo",
            .vid                 = 0x057E,
            .pid                 = 0x2069,
            .product_string      = "Pro Controller",
            .manufacturer_string = "Nintendo Co., Ltd." } },
    };

    namespace hid {
        inline constexpr auto INPUT_REPORT_SIZE  = 64_usize;
        inline constexpr auto OUTPUT_REPORT_SIZE = 42_usize;

        inline constexpr auto DEFAULT_OUTPUT_REPORT = into_bytes({
          // report ID
          0x02,
          // rumble left
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          // rumble right
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          // reserved
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
        });

        inline constexpr auto DEFAULT_REPORT_DESCRIPTOR = into_bytes({
          0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
          0x09, 0x05,       // Usage (Game Pad)
          0xA1, 0x01,       // Collection (Application)
          0x85, 0x05,       //   Report ID (5)
          0x05, 0xFF,       //   Usage Page (Reserved 0xFF)
          0x09, 0x01,       //   Usage (0x01)
          0x15, 0x00,       //   Logical Minimum (0)
          0x26, 0xFF, 0x00, //   Logical Maximum (255)
          0x95, 0x3F,       //   Report Count (63)
          0x75, 0x08,       //   Report Size (8)
          0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
          0x85, 0x09,       //   Report ID (9)
          0x09, 0x01,       //   Usage (0x01)
          0x95, 0x02,       //   Report Count (2)
          0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
          0x05, 0x09,       //   Usage Page (Button)
          0x19, 0x01,       //   Usage Minimum (0x01)
          0x29, 0x15,       //   Usage Maximum (0x15)
          0x25, 0x01,       //   Logical Maximum (1)
          0x95, 0x15,       //   Report Count (21)
          0x75, 0x01,       //   Report Size (1)
          0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
          0x95, 0x01,       //   Report Count (1)
          0x75, 0x03,       //   Report Size (3)
          0x81, 0x03,       //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
          0x05, 0x01,       //   Usage Page (Generic Desktop Ctrls)
          0x09, 0x01,       //   Usage (Pointer)
          0xA1, 0x00,       //   Collection (Physical)
          0x09, 0x30,       //    Usage (X)
          0x09, 0x31,       //    Usage (Y)
          0x09, 0x33,       //    Usage (Rx)
          0x09, 0x35,       //    Usage (Rz)
          0x26, 0xFF, 0x0F, //    Logical Maximum (4095)
          0x95, 0x04,       //    Report Count (4)
          0x75, 0x0C,       //    Report Size (12)
          0x81, 0x02,       //    Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
          0xC0,             //   End Collection
          0x05, 0xFF,       //   Usage Page (Reserved 0xFF)
          0x09, 0x02,       //   Usage (0x02)
          0x26, 0xFF, 0x00, //   Logical Maximum (255)
          0x95, 0x34,       //   Report Count (52)
          0x75, 0x08,       //   Report Size (8)
          0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
          0x85, 0x02,       //   Report ID (2)
          0x09, 0x01,       //   Usage (0x01)
          0x95, 0x3F,       //   Report Count (63)
          0x91, 0x02,       //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
          0xC0,             // End Collection
        });

        inline constexpr auto DEFAULT_DESCRIPTOR = HID_DESCRIPTOR {
            .bLength         = sizeof(HID_DESCRIPTOR),
            .bDescriptorType = 0x21, // HID == 0x21
            .bcdHID          = 0x0111,
            .bCountry        = 0x00, // UNUSED
            .bNumDescriptors = 0x01,
            .DescriptorList  = { { .bReportType = 0x22, .wReportLength = sizeof(DEFAULT_REPORT_DESCRIPTOR) } }
        };
    } // namespace hid
} // namespace lj
