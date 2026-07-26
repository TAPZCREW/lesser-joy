module;

#include "windows.hpp"

#include <hidport.h>

#include <stormkit/core/try_expected.hpp>

export module lesserjoy.hid;

import std;

import stormkit.core;

import lesserjoy.log;
import lesserjoy.constants;
import lesserjoy.ntstatus;

using namespace stormkit;

namespace stdr = std::ranges;

export namespace lj::hid {
    using Report_descriptor = std::span<const byte>;

    struct Control_info {
        u8 report_id    = 0;
        u8 control_code = 0;
    };

    struct Input_info {
        u8 report_id = 0;
        u8 data      = 0;
    };

    struct Output_info {
        u8  report_id = 0;
        u8  data      = 0;
        u16 _;
        u32 _;
    };

    auto get_device_descriptor(WDFREQUEST& request, const HID_DESCRIPTOR& descriptor) noexcept -> Expected<void>;
    auto get_device_attributes(WDFREQUEST& request, const HID_DEVICE_ATTRIBUTES& attributes) noexcept -> Expected<void>;
    auto get_report_descriptor(WDFREQUEST& request, const hid::Report_descriptor& descriptor) noexcept -> Expected<void>;

    auto read_report(WDFREQUEST& request) -> Expected<void>;

    enum class Direction : u8 {
        HOST_TO_DEVICE = 0x91,
        DEVICE_TO_HOST = 0x01,
    };

    enum class Transport : u8 {
        USB       = 0x00,
        BLUETOOTH = 0x01,
    };

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

    // inline constexpr auto DEFAULT_REPORT_DESCRIPTOR = to_array<u8>({
    //   0x05, 0x01,                   // Usage Page (Generic Desktop Ctrls)
    //   0x15, 0x00,                   // Logical Minimum (0)
    //   0x09, 0x04,                   // Usage (Joystick)
    //   0xA1, 0x01,                   // Collection (Application)
    //   0x85, 0x30,                   //   Report ID (48)
    //   0x05, 0x01,                   //   Usage Page (Generic Desktop Ctrls)
    //   0x05, 0x09,                   //   Usage Page (Button)
    //   0x19, 0x01,                   //   Usage Minimum (0x01)
    //   0x29, 0x0A,                   //   Usage Maximum (0x0A)
    //   0x15, 0x00,                   //   Logical Minimum (0)
    //   0x25, 0x01,                   //   Logical Maximum (1)
    //   0x75, 0x01,                   //   Report Size (1)
    //   0x95, 0x0A,                   //   Report Count (10)
    //   0x55, 0x00,                   //   Unit Exponent (0)
    //   0x65, 0x00,                   //   Unit (None)
    //   0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x05, 0x09,                   //   Usage Page (Button)
    //   0x19, 0x0B,                   //   Usage Minimum (0x0B)
    //   0x29, 0x0E,                   //   Usage Maximum (0x0E)
    //   0x15, 0x00,                   //   Logical Minimum (0)
    //   0x25, 0x01,                   //   Logical Maximum (1)
    //   0x75, 0x01,                   //   Report Size (1)
    //   0x95, 0x04,                   //   Report Count (4)
    //   0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x75, 0x01,                   //   Report Size (1)
    //   0x95, 0x02,                   //   Report Count (2)
    //   0x81, 0x03,                   //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x0B, 0x01, 0x00, 0x01, 0x00, //   Usage (0x010001)
    //   0xA1, 0x00,                   //   Collection (Physical)
    //   0x0B, 0x30, 0x00, 0x01, 0x00, //    Usage (0x010030)
    //   0x0B, 0x31, 0x00, 0x01, 0x00, //    Usage (0x010031)
    //   0x0B, 0x32, 0x00, 0x01, 0x00, //    Usage (0x010032)
    //   0x0B, 0x35, 0x00, 0x01, 0x00, //    Usage (0x010035)
    //   0x15, 0x00,                   //    Logical Minimum (0)
    //   0x27, 0xFF, 0xFF, 0x00, 0x00, //    Logical Maximum (65534)
    //   0x75, 0x10,                   //    Report Size (16)
    //   0x95, 0x04,                   //    Report Count (4)
    //   0x81, 0x02,                   //    Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0xC0,                         //   End Collection
    //   0x0B, 0x39, 0x00, 0x01, 0x00, //   Usage (0x010039)
    //   0x15, 0x00,                   //   Logical Minimum (0)
    //   0x25, 0x07,                   //   Logical Maximum (7)
    //   0x35, 0x00,                   //   Physical Minimum (0)
    //   0x46, 0x3B, 0x01,             //   Physical Maximum (315)
    //   0x65, 0x14,                   //   Unit (System: English Rotation, Length: Centimeter)
    //   0x75, 0x04,                   //   Report Size (4)
    //   0x95, 0x01,                   //   Report Count (1) Data, Var,  Abs,  No Wrap, Linear, Preferred State, Null State
    //   0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x05, 0x09,                   //   Usage Page (Button)
    //   0x19, 0x0F,                   //   Usage Minimum (0x0F)
    //   0x29, 0x12,                   //   Usage Maximum (0x12)
    //   0x15, 0x00,                   //   Logical Minimum (0)
    //   0x25, 0x01,                   //   Logical Maximum (1)
    //   0x75, 0x01,                   //   Report Size (1)
    //   0x95, 0x04,                   //   Report Count (4)
    //   0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x75, 0x08,                   //   Report Size (8)
    //   0x95, 0x34,                   //   Report Count (52)
    //   0x81, 0x03,                   //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x06, 0x00, 0xFF,             //   Usage Page (Vendor Defined 0xFF00)
    //   0x85, 0x21,                   //   Report ID (33)
    //   0x09, 0x01,                   //   Usage (0x01)
    //   0x75, 0x08,                   //   Report Size (8)
    //   0x95, 0x3F,                   //   Report Count (63)
    //   0x81, 0x03,                   //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x85, 0x81,                   //   Report ID (-127)
    //   0x09, 0x02,                   //   Usage (0x02)
    //   0x75, 0x08,                   //   Report Size (8)
    //   0x95, 0x3F,                   //   Report Count (63)
    //   0x81, 0x03,                   //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x85, 0x01,                   //   Report ID (1)
    //   0x09, 0x03,                   //   Usage (0x03)
    //   0x75, 0x08,                   //   Report Size (8)
    //   0x95, 0x3F,                   //   Report Count (63)
    //   0x91, 0x83,                   //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
    //   0x85, 0x10,                   //   Report ID (16)
    //   0x09, 0x04,                   //   Usage (0x04)
    //   0x75, 0x08,                   //   Report Size (8)
    //   0x95, 0x3F,                   //   Report Count (63)
    //   0x91, 0x83,                   //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
    //   0x85, 0x80,                   //   Report ID (-128)
    //   0x09, 0x05,                   //   Usage (0x05)
    //   0x75, 0x08,                   //   Report Size (8)
    //   0x95, 0x3F,                   //   Report Count (63)
    //   0x91, 0x83,                   //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
    //   0x85, 0x82,                   //   Report ID (-126)
    //   0x09, 0x06,                   //   Usage (0x06)
    //   0x75, 0x08,                   //   Report Size (8)
    //   0x95, 0x3F,                   //   Report Count (63)
    //   0x91, 0x83,                   //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
    //   0xC0,                         // End Collection
    // });

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
        namespace init {
            // @see https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x03---initialisation
            enum class Sub_command : u8 {
                BT_WAKE               = 0x01,
                BT_CANCEL             = 0x02,
                ENABLE_USB_HID_REPORT = 0x03,
                UNKNOWN_0x04          = 0x04,
                UNKNOWN_0x05          = 0x05,
                UNKNOWN_0x06          = 0x06,
                SEND_PAIRING_INFO     = 0x07,
                CLEAR_PAIRING_INFO    = 0x08,
                STORE_PAIRING_INFO    = 0x09,
                SELECT_INPUT_REPORT   = 0x0A,
                UNKNOWN_0x0C          = 0x0C,
                INITIALIZE_USB        = 0x0D,
                UNKNOWN_0x0F          = 0x0F,
            };

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x01---bluetooth-wake
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto bt_wake(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::BT_WAKE),
                                    0x00_u8,
                                    0x04_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x01_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x02---bluetooth-cancel
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto bt_cancel(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::BT_CANCEL),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x03---enable-usb-hid-reports
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto enable_usb_hid_report() noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(Transport::USB),
                                    narrow<u8>(Sub_command::ENABLE_USB_HID_REPORT),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // TODO
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x07---send-pairing-info
            // STORMKIT_FORCE_INLINE STORMKIT_PURE
            // constexpr auto send_pairing_info(Transport transport) noexcept -> decltype(auto) {
            //     return into_bytes({ 0x03,
            //                         narrow<u8>(Direction::HOST_TO_DEVICE),
            //                         narrow<u8>(transport),
            //                         narrow<u8>(Command),
            //                         0x00,
            //                         0x00,
            //                         0x00,
            //                         0x00 });
            // }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x08---clear-pairing-info
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto clear_pairing_info(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::CLEAR_PAIRING_INFO),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x09---store-pairing-info
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto store_pairing_info(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::STORE_PAIRING_INFO),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x0a---select-input-report
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto select_input_report(Transport transport, byte report_id) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::SELECT_INPUT_REPORT),
                                    0x0A_u8,
                                    0x00_u8,
                                    0x04_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    narrow<u8>(report_id),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x0d---initialise-usb
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto initialize_usb(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::INITIALIZE_USB),
                                    0x00_u8,
                                    0x08_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }
        } // namespace init

        namespace leds {
            enum class Sub_command : u8 {
                SET_PLAYER_1,
                SET_PLAYER_2,
                SET_PLAYER_3,
                SET_PLAYER_4,
                ALL_LEDS_ON,
                ALL_LEDS_OFF,
                SET_PLAYER_LED_MASK,
                FLASH_LEDS,
            };

            enum class Player {
                PLAYER_1 = 0x1,
                PLAYER_2 = 0x2,
                PLAYER_3 = 0x4,
                PLAYER_4 = 0x8,
            };

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x01---set-player-1-led
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto set_player_1(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::SET_PLAYER_1),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x02---set-player-2-led
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto set_player_2(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::SET_PLAYER_2),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x03---set-player-3-led
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto set_player_3(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::SET_PLAYER_3),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x04---set-player-4-led
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto set_player_4(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::SET_PLAYER_4),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x05---set-all-leds-on
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto all_leds_on(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::ALL_LEDS_ON),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x06---set-all-leds-off
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto all_leds_off(Transport transport) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::ALL_LEDS_OFF),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x07---set-led-pattern
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto set_player_led_mask(Transport transport, u8 mask) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::SET_PLAYER_LED_MASK),
                                    0x00_u8,
                                    0x08_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    narrow<u8>(mask),
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }

            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x08---flash-leds
            STORMKIT_FORCE_INLINE STORMKIT_PURE
            constexpr auto flash_leds(Transport transport, bool enabled) noexcept -> decltype(auto) {
                return into_bytes({ 0x03_u8,
                                    narrow<u8>(Direction::HOST_TO_DEVICE),
                                    narrow<u8>(transport),
                                    narrow<u8>(Sub_command::FLASH_LEDS),
                                    0x00_u8,
                                    0x08_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    enabled ? 0x01_u8 : 0x00_u8,
                                    0x00_u8,
                                    0x00_u8,
                                    0x00_u8 });
            }
        } // namespace leds

        inline constexpr auto UNKNOWN_COMMAND_0x07 = into_bytes({ 0x07, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 });

        inline constexpr auto REQUEST_CONTROLLER_MAC = into_bytes({
          0x15, 0x91, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x00,
          0x00, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Console MAC Address (Little Endian)
          0xFF,                                           // Byte 14 with bit 0 masked off
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF                    // Remainder of Console MAC Address
        });

        inline constexpr auto LTK_REQUEST = into_bytes({ 0x15, 0x91, 0x00, 0x02, 0x00, 0x11, 0x00, 0x00, 0x00,
                                                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // LTK - 16 byte key
                                                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

        inline constexpr auto
          SET_FEATURE_MASK = into_bytes({ 0x0c, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00 });
        inline constexpr auto UNKNOWN_COMMAND_0x11  = into_bytes({ 0x11, 0x91, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00 });
        inline constexpr auto RESET_VIBRATION_STATE = into_bytes({ 0x0a, 0x91, 0x00, 0x08, 0x00, 0x14, 0x00, 0x00, 0x01, 0xff,
                                                                   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x35, 0x00, 0x46,
                                                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        inline constexpr auto
          ENABLE_HAPTICS = into_bytes({ 0x03, 0x91, 0x00, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00 });
        inline constexpr auto GET_FIRMWARE_INFO = into_bytes({ 0x10, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00 });
        inline constexpr auto
          ENABLE_HID_REPORTS = into_bytes({ 0x03, 0x91, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 });
        inline constexpr auto NFC_UNKNOWN_COMMAND = into_bytes({ 0x01, 0x91, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00 });

    } // namespace commands

} // namespace lj::hid

export namespace stormkit { inline namespace core { namespace meta {
    template<>
    inline constexpr auto FLAG_TRAIT<lj::hid::commands::leds::Player> = true;
}}} // namespace stormkit::core::meta

module: private;

namespace lj::hid {
    namespace {
        auto request_copy_from_buffer(WDFREQUEST request, std::span<const byte> from) -> Expected<void> {
            auto memory = WDFMEMORY {};
            LoggedTry(lj::win_call(WdfRequestRetrieveOutputMemory, request, &memory), "WdfRequestRetrieveOutputMemory failed!");

            auto output_buffer_extent = 0_usize;
            WdfMemoryGetBuffer(memory, &output_buffer_extent);
            if (output_buffer_extent < stdr::size(from)) {
                lj::elog("request_copy_from_buffer: buffer too small! Size {}, expects {}\n",
                         output_buffer_extent,
                         stdr::size(from));
                Return std::unexpected<system_error2::nt_code> { std::in_place, STATUS_INVALID_BUFFER_SIZE };
            }

            LoggedTry(lj::win_call(WdfMemoryCopyFromBuffer, memory, 0, bit_cast<void*>(stdr::data(from)), stdr::size(from)),
                      "WdfMemoryCopyFromBuffer failed!");

            WdfRequestSetInformation(request, stdr::size(from));
            Return {};
        }
    } // namespace

    auto get_device_descriptor(WDFREQUEST& request, const HID_DESCRIPTOR& descriptor) noexcept -> Expected<void> {
        return request_copy_from_buffer(request, as_bytes(descriptor));
    }

    auto get_device_attributes(WDFREQUEST& request, const HID_DEVICE_ATTRIBUTES& attributes) noexcept -> Expected<void> {
        return request_copy_from_buffer(request, as_bytes(attributes));
    }

    auto get_report_descriptor(WDFREQUEST& request, const Report_descriptor& descriptor) noexcept -> Expected<void> {
        return request_copy_from_buffer(request, as_bytes(descriptor));
    }
} // namespace lj::hid
