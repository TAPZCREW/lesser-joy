export module lesserjoy:constants;

import std;
import frozen;

import stormkit.core;
import stormkit.platform.win32;

import wdk.hid;

using namespace stormkit;
using namespace stormkit::literals;

export namespace lj {
    struct controller_type {
        string_view name;
        string_view vendor;
        u16         vid;
        u16         pid;
        string_view product_string;
        string_view manufacturer_string;
    };

    enum class direction : u8 {
        host_to_device = 0x91,
        device_to_host = 0x01,
    };

    enum class transport_type : u8 {
        usb       = 0x00,
        bluetooth = 0x01,
    };

    enum class validate {
        yes,
        no,
    };

    inline constexpr auto dev_interface_hid_guid = win32::GUID {
        0x4D1E55B2,
        0xF16F,
        0x11CF,
        { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 }
    };

    inline constexpr auto drive_pool_tag  = u32 { 'lyoj' };
    inline constexpr auto drive_max_slots = 8_usize;

    // inline constexpr auto controllers_type = frozen::unordered_map<frozen::string, controller_type, 1> {
    inline constexpr auto controllers_type = make_static_hash_map<frozen::string, controller_type>({
      { "pro_controller",
       { .name                = "Nintendo Switch 2 Pro Controller",
          .vendor              = "Nintendo",
          .vid                 = 0x057E,
          .pid                 = 0x2069,
          .product_string      = "Pro Controller",
          .manufacturer_string = "Nintendo Co., Ltd." } }
    });

    // inline constexpr auto controllers_map = frozen::unordered_map<u32, frozen::string, 1> {
    inline constexpr auto controllers_map = make_static_hash_map<u32, frozen::string>({
      { controllers_type.at("pro_controller").vid | (controllers_type.at("pro_controller").pid << 16), "pro_controller" }
    });

    namespace hid {
        // HIDP_CAPS::InputReportByteLength / OutputReportByteLength exactly as the OS derives them
        // from `default_report_descriptor` below: 1 report-id byte + the declared report count.
        // report 0x05 and 0x09 each declare 63 data bytes; report 0x02 declares `0x95, 0x3F` = 63.
        // @see WDK Include/10.0.28000.0/shared/hidpi.h:245 (HIDP_CAPS)
        inline constexpr auto input_report_size  = 64_usize;
        inline constexpr auto output_report_id   = 0x02_u8;
        inline constexpr auto output_report_size = 64_usize;

        // semantic layout of the output report payload; only `data_size` bytes are meaningful,
        // the rest of the report is the reserved tail the descriptor still forces us to send.
        // @see https://github.com/ndeadly/switch2_controller_research/blob/master/hid_reports.md#output-report-0x02
        namespace rumble {
            inline constexpr auto block_size   = 0x10_usize;
            inline constexpr auto left_offset  = 0x01_usize;
            inline constexpr auto right_offset = left_offset + block_size;
            inline constexpr auto data_size    = right_offset + block_size; // 0x21
        } // namespace rumble

        // how long an output report send may stay in flight before we report failure to the caller.
        // hidapi's winapi backend uses a 1 s write timeout for the same reason.
        inline constexpr auto output_report_timeout_ms = 1000_u32;

        // human-readable name for an iocontrol code so the unhandled path in device.cpp can report
        // what a consumer actually asked for instead of swallowing it.
        // @see WDK Include/10.0.28000.0/shared/hidport.h:183-200 and shared/hidclass.h:141-174
        inline constexpr auto ioctl_names = make_static_hash_map<u32, frozen::string>({
          { wdk::IOCTL_HID_GET_DRIVER_CONFIG, "IOCTL_HID_GET_DRIVER_CONFIG" },
          { wdk::IOCTL_HID_SET_DRIVER_CONFIG, "IOCTL_HID_SET_DRIVER_CONFIG" },
          { wdk::IOCTL_HID_GET_POLL_FREQUENCY_MSEC, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC" },
          { wdk::IOCTL_HID_SET_POLL_FREQUENCY_MSEC, "IOCTL_HID_SET_POLL_FREQUENCY_MSEC" },
          { wdk::IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS, "IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS" },
          { wdk::IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS, "IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS" },
          { wdk::IOCTL_HID_GET_COLLECTION_INFORMATION, "IOCTL_HID_GET_COLLECTION_INFORMATION" },
          { wdk::IOCTL_HID_ENABLE_WAKE_ON_SX, "IOCTL_HID_ENABLE_WAKE_ON_SX" },
          { wdk::IOCTL_HID_SET_S0_IDLE_TIMEOUT, "IOCTL_HID_SET_S0_IDLE_TIMEOUT" },
          { wdk::IOCTL_HID_GET_COLLECTION_DESCRIPTOR, "IOCTL_HID_GET_COLLECTION_DESCRIPTOR" },
          { wdk::IOCTL_HID_FLUSH_QUEUE, "IOCTL_HID_FLUSH_QUEUE" },
          { wdk::IOCTL_HID_SET_FEATURE, "IOCTL_HID_SET_FEATURE" },
          { wdk::IOCTL_HID_SET_OUTPUT_REPORT, "IOCTL_HID_SET_OUTPUT_REPORT" },
          { wdk::IOCTL_HID_GET_FEATURE, "IOCTL_HID_GET_FEATURE" },
          { wdk::IOCTL_HID_GET_HARDWARE_ID, "IOCTL_HID_GET_HARDWARE_ID" },
          { wdk::IOCTL_HID_GET_INDEXED_STRING, "IOCTL_HID_GET_INDEXED_STRING" },
          { wdk::IOCTL_HID_GET_INPUT_REPORT, "IOCTL_HID_GET_INPUT_REPORT" },
          { wdk::IOCTL_HID_GET_OUTPUT_REPORT, "IOCTL_HID_GET_OUTPUT_REPORT" },
          { wdk::IOCTL_GET_PHYSICAL_DESCRIPTOR, "IOCTL_GET_PHYSICAL_DESCRIPTOR" },
          { wdk::IOCTL_HID_GET_MANUFACTURER_STRING, "IOCTL_HID_GET_MANUFACTURER_STRING" },
          { wdk::IOCTL_HID_GET_PRODUCT_STRING, "IOCTL_HID_GET_PRODUCT_STRING" },
          { wdk::IOCTL_HID_GET_SERIALNUMBER_STRING, "IOCTL_HID_GET_SERIALNUMBER_STRING" },
          { wdk::IOCTL_HID_GET_MS_GENRE_DESCRIPTOR, "IOCTL_HID_GET_MS_GENRE_DESCRIPTOR" },
          { wdk::IOCTL_HID_ENABLE_SECURE_READ, "IOCTL_HID_ENABLE_SECURE_READ" },
          { wdk::IOCTL_HID_DISABLE_SECURE_READ, "IOCTL_HID_DISABLE_SECURE_READ" },
          { wdk::IOCTL_HID_DEVICERESET_NOTIFICATION, "IOCTL_HID_DEVICERESET_NOTIFICATION" }
        });

        constexpr auto ioctl_name(u32 io_control_code) noexcept -> string_view {
            const auto found = ioctl_names.find(io_control_code);
            if (found != ioctl_names.end())
                return { found->second.data(), found->second.size() };

            return "UNKNOWN_IOCTL";
        }

        inline constexpr auto default_report_descriptor = into<array>(
          as_bytes,
          {
            // clang-format off
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
                  // clang-format on
          });

        inline constexpr auto default_descriptor = wdk::HID_DESCRIPTOR {
            .bLength         = sizeof(wdk::HID_DESCRIPTOR),
            .bDescriptorType = 0x21, // HID == 0x21
            .bcdHID          = 0x0111,
            .bCountry        = 0x00, // UNUSED
            .bNumDescriptors = 0x01,
            .DescriptorList  = { { .bReportType = 0x22, .wReportLength = sizeof(default_report_descriptor) } }
        };
    } // namespace hid
} // namespace lj
