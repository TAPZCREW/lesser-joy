module;

#include "windows.hpp"

#include <hidport.h>

#include <stormkit/core/contract_macro.hpp>
#include <stormkit/core/try_expected.hpp>

export module lesserjoy.hid;

import std;
import frozen;

import stormkit.core;

import lesserjoy.log;
import lesserjoy.constants;
import lesserjoy.ntstatus;
import lesserjoy.usb;

using namespace stormkit;

namespace stdr = std::ranges;
namespace stdv = std::views;

export namespace lj::hid {
    using Report_descriptor = array_view<const byte>;

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

    enum class Direction : u8 {
        HOST_TO_DEVICE = 0x91,
        DEVICE_TO_HOST = 0x01,
    };

    enum class Transport : u8 {
        USB       = 0x00,
        BLUETOOTH = 0x01,
    };

    auto get_device_descriptor(WDFREQUEST& request, const HID_DESCRIPTOR& descriptor) noexcept -> Expected<void>;
    auto get_device_attributes(WDFREQUEST& request, const HID_DEVICE_ATTRIBUTES& attributes) noexcept -> Expected<void>;
    auto get_report_descriptor(WDFREQUEST& request, const hid::Report_descriptor& descriptor) noexcept -> Expected<void>;

    auto read_report(WDFREQUEST& request, array_view<byte> to) -> Expected<void>;
    auto write_report(WDFREQUEST& request, array_view<const byte> from) -> Expected<void>;
    auto get_string(WDFREQUEST& request, string_view product_string, string_view serial_string) -> Expected<void>;
    auto get_indexed_string(WDFREQUEST& request, string_view product_string) -> Expected<void>;

    enum class Validate {
        YES,
        NO,
    };

    template<typename Command, typename... Args>
    auto send_command_sync(const auto& ctx, Args&&... args) -> Expected<usize>;
    template<typename Command, Validate VALIDATE = Validate::NO>
    auto receive_command_sync(const auto& ctx) -> Expected<array<byte, Command::RESPONSE_LENGTH>>;
    template<typename Command, Validate VALIDATE = Validate::NO, typename... Args>
    auto send_command_receive_response_sync(const auto& ctx, Args&&... args) -> Expected<array<byte, Command::RESPONSE_LENGTH>>;

    inline constexpr auto DEFAULT_OUTPUT_REPORT_PRO_CONTROLLER = into_bytes({
      0x02, // report id
      // hd rumble data left
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
      // hd rumble data right
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
      // unused
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

    // @see
    // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md
    enum class Command_id : u8 {
        INIT         = 0x03,
        UNKNOWN_0x07 = 0x07,
        LEDS         = 0x09,
    };

    struct CommandData {
        u8 ack                     = 0xF8;
        u8 request_payload_length  = 0x00;
        u8 response_payload_length = 0x00;
    };

    template<Command_id>
    struct Subcommand_enum;

    template<Transport                           TRANSPORT_,
             Command_id                          ID_,
             typename Subcommand_enum<ID_>::type SUB_ID_,
             CommandData                         DATA         = {},
             auto                                FILL_PAYLOAD = monadic::noop()>
    struct Command {
        static constexpr auto ID        = ID_;
        static constexpr auto SUB_ID    = SUB_ID_;
        static constexpr auto TRANSPORT = TRANSPORT_;

        static constexpr auto ACK                     = DATA.ack;
        static constexpr auto REQUEST_PAYLOAD_LENGTH  = DATA.request_payload_length;
        static constexpr auto RESPONSE_PAYLOAD_LENGTH = DATA.response_payload_length;

        static constexpr auto
          REQUEST_HEADER = into_bytes({ narrow<u8>(ID),
                                        narrow<u8>(Direction::HOST_TO_DEVICE),
                                        narrow<u8>(TRANSPORT),
                                        narrow<u8>(SUB_ID),
                                        0x00_u8,
                                        narrow<u8>(REQUEST_PAYLOAD_LENGTH),
                                        0x00_u8,
                                        0x00_u8 });
        static constexpr auto
          RESPONSE_HEADER = into_bytes({ narrow<u8>(ID),
                                         narrow<u8>(Direction::DEVICE_TO_HOST),
                                         narrow<u8>(TRANSPORT),
                                         narrow<u8>(SUB_ID),
                                         0x00_u8,
                                         narrow<u8>(ACK),
                                         0x00_u8,
                                         0x00_u8 });

        static constexpr auto REQUEST_LENGTH  = stdr::size(REQUEST_HEADER) + REQUEST_PAYLOAD_LENGTH;
        static constexpr auto RESPONSE_LENGTH = stdr::size(RESPONSE_HEADER) + RESPONSE_PAYLOAD_LENGTH;

        template<typename... Args>
        static constexpr auto make_payload(Args&&... args) noexcept -> array<byte, REQUEST_PAYLOAD_LENGTH>;
        template<typename... Args>
        static constexpr auto make_request(Args&&... args) noexcept -> array<byte, REQUEST_LENGTH>;
        static constexpr auto make_response() noexcept -> array<byte, RESPONSE_LENGTH>;
        static constexpr auto validate_response(array_view<const byte> response) noexcept -> bool;
    };

    namespace init {
        // @see
        // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x03---initialisation
        enum class Subcommand_id : u8 {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x01---bluetooth-wake
            BT_WAKE = 0x01,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x02---bluetooth-cancel
            BT_CANCEL = 0x02,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x03---enable-usb-hid-reports
            ENABLE_USB_HID_REPORT = 0x03,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x07---send-pairing-info
            UNKNOWN_0x04 = 0x04,
            UNKNOWN_0x05 = 0x05,
            UNKNOWN_0x06 = 0x06,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x07---send-pairing-info
            SEND_PAIRING_INFO = 0x07,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x08---clear-pairing-info
            CLEAR_PAIRING_INFO = 0x08,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x09---store-pairing-info
            STORE_PAIRING_INFO = 0x09,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x0a---select-input-report
            SELECT_INPUT_REPORT = 0x0A,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x0c---initialise-usb
            UNKNOWN_0x0C = 0x0C,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x0d---initialise-usb
            INITIALIZE_USB = 0x0D,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x0c---initialise-usb
            UNKNOWN_0x0F = 0x0F,
        };

        enum class Input_report_id {
            GENERIC        = 0x05,
            ALT_JOYCON_L_2 = 0x07,
            ALT_JOYCON_R_2 = 0x08,
            ALT_PROCON_2   = 0x09,
            ALT_NSO_GC_2   = 0x01,
        };

        enum class Output_report_id {
            JOYCON_L_2 = 0x01,
            JOYCON_R_2 = 0x01,
            PROCON_2   = 0x02,
            NSO_GC_2   = 0x03,
        };
    } // namespace init

    namespace leds {
        enum class Subcommand_id : u8 {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x01---set-player-1-led
            SET_PLAYER_1 = 0x01,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x02---set-player-2-led
            SET_PLAYER_2 = 0x02,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x03---set-player-3-led
            SET_PLAYER_3 = 0x03,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x04---set-player-4-led
            SET_PLAYER_4 = 0x04,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x05---set-all-leds-on
            ALL_LEDS_ON = 0x05,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x06---set-all-leds-off
            ALL_LEDS_OFF = 0x06,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x07---set-led-pattern
            SET_PLAYER_LED_MASK = 0x07,
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x08---flash-leds
            FLASH_LEDS = 0x08,
        };

        enum class Player : u8 {
            PLAYER_1 = 0x1,
            PLAYER_2 = 0x2,
            PLAYER_3 = 0x4,
            PLAYER_4 = 0x8,
        };

    } // namespace leds

    namespace unknown_0x07 {
        enum class Subcommand_id : u8 {
            UNKNOWN_0x01 = 0x01,
            UNKNOWN_0x02 = 0x02,
        };
    } // namespace unknown_0x07

    template<>
    struct Subcommand_enum<Command_id::INIT> {
        using type = init::Subcommand_id;
    };

    template<>
    struct Subcommand_enum<Command_id::LEDS> {
        using type = leds::Subcommand_id;
    };

    template<>
    struct Subcommand_enum<Command_id::UNKNOWN_0x07> {
        using type = unknown_0x07::Subcommand_id;
    };

    namespace init {
        template<Transport TRANSPORT, Subcommand_id SUB_ID, CommandData DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
        using Command = hid::Command<TRANSPORT, Command_id::INIT, SUB_ID, DATA, FILL_PAYLOAD>;

        template<Transport TRANSPORT>
        using Bt_wake_command = Command<TRANSPORT, Subcommand_id::BT_WAKE, CommandData { .request_payload_length = 0x04 }>;
        template<Transport TRANSPORT>
        using Bt_cancel_command = Command<TRANSPORT, Subcommand_id::BT_CANCEL>;
        template<Transport TRANSPORT>
        using Enable_usb_hid_report_command = Command<
          TRANSPORT,
          Subcommand_id::ENABLE_USB_HID_REPORT,
          CommandData { .request_payload_length = 0x04, .response_payload_length = 0x04 }>;
        template<Transport TRANSPORT>
        using Unknown_0x04_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x04>;
        template<Transport TRANSPORT>
        using Unknown_0x05_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x05>;
        template<Transport TRANSPORT>
        using Unknown_0x06_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x06>;
        template<Transport TRANSPORT>
        using Send_pairing_info_command = Command<TRANSPORT,
                                                  Subcommand_id::SEND_PAIRING_INFO,
                                                  CommandData { .ack = 0x78, .request_payload_length = 0x16 }>;
        template<Transport TRANSPORT>
        using Clear_pairing_info_command = Command<TRANSPORT, Subcommand_id::CLEAR_PAIRING_INFO>;
        template<Transport TRANSPORT>
        using Store_pairing_command = Command<TRANSPORT, Subcommand_id::STORE_PAIRING_INFO, CommandData { .ack = 0x78 }>;
        template<Transport TRANSPORT>
        using Select_input_report_command = Command<
          TRANSPORT,
          Subcommand_id::SELECT_INPUT_REPORT,
          CommandData { .request_payload_length = 0x04 },
          [](array_view<byte, 0x04> payload, Input_report_id id) static noexcept { payload[0] = narrow<byte>(id); }>;
        template<Transport TRANSPORT>
        using Unknown_0x0C_command = Command<TRANSPORT,
                                             Subcommand_id::UNKNOWN_0x0C,
                                             CommandData { .request_payload_length = 0x04 }>;
        template<Transport TRANSPORT>
        using Initialize_usb_command = Command<TRANSPORT,
                                               Subcommand_id::INITIALIZE_USB,
                                               CommandData { .request_payload_length = 0x08, .response_payload_length = 0x04 }>;
        template<Transport TRANSPORT>
        using Unknown_0x0F_command = Command<TRANSPORT,
                                             Subcommand_id::UNKNOWN_0x0F,
                                             CommandData { .response_payload_length = 0x04 }>;
    } // namespace init

    namespace unknown_0x07 {
        template<Transport TRANSPORT, Subcommand_id SUB_ID, CommandData DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
        using Command = hid::Command<TRANSPORT, Command_id::UNKNOWN_0x07, SUB_ID, DATA, FILL_PAYLOAD>;

        template<Transport TRANSPORT>
        using Unknown_0x01_command = Command<TRANSPORT,
                                             Subcommand_id::UNKNOWN_0x01,
                                             CommandData { .response_payload_length = 0x01 }>;
        template<Transport TRANSPORT>
        using Unknown_0x02_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x02>;
    } // namespace unknown_0x07

    namespace leds {
        template<Transport TRANSPORT, Subcommand_id SUB_ID, CommandData DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
        using Command = hid::Command<TRANSPORT, Command_id::LEDS, SUB_ID, DATA, FILL_PAYLOAD>;

        template<Transport TRANSPORT>
        using Set_player_1_command = Command<TRANSPORT, Subcommand_id::SET_PLAYER_1>;
        template<Transport TRANSPORT>
        using Set_player_2_command = Command<TRANSPORT, Subcommand_id::SET_PLAYER_2>;
        template<Transport TRANSPORT>
        using Set_player_3_command = Command<TRANSPORT, Subcommand_id::SET_PLAYER_3>;
        template<Transport TRANSPORT>
        using Set_player_4_command = Command<TRANSPORT, Subcommand_id::SET_PLAYER_4>;
        template<Transport TRANSPORT>
        using All_leds_on_command = Command<TRANSPORT, Subcommand_id::ALL_LEDS_ON>;
        template<Transport TRANSPORT>
        using All_leds_off_command = Command<TRANSPORT, Subcommand_id::ALL_LEDS_OFF>;
        template<Transport TRANSPORT>
        using Set_player_led_mask = Command<TRANSPORT,
                                            Subcommand_id::SET_PLAYER_LED_MASK,
                                            CommandData { .request_payload_length = 0x08 }>;
        template<Transport TRANSPORT>
        using Flash_leds_command = Command<TRANSPORT, Subcommand_id::FLASH_LEDS, CommandData { .request_payload_length = 0x04 }>;
    } // namespace leds
} // namespace lj::hid

export namespace stormkit { inline namespace core { namespace meta {
    template<>
    inline constexpr auto FLAG_TRAIT<lj::hid::leds::Player> = true;
}}} // namespace stormkit::core::meta

namespace lj::hid {
    template<typename Command, typename... Args>
    STORMKIT_FORCE_INLINE
    inline auto send_command_sync(const auto& ctx, Args&&... args) -> Expected<usize> {
        if constexpr (Command::TRANSPORT == Transport::USB)
            return usb::send_command_sync(ctx, Command::make_request(std::forward<Args>(args)...));
    }

    template<typename Command, Validate VALIDATE>
    inline auto receive_command_sync(const auto& ctx) -> Expected<array<byte, Command::RESPONSE_LENGTH>> {
        auto response = array<byte, Command::RESPONSE_LENGTH> {};

        auto size = 0_usize;
        if constexpr (Command::TRANSPORT == Transport::USB) size = Try(usb::receive_response_sync(ctx, response));

        if constexpr (VALIDATE == Validate::YES) {
            if (size != Command::RESPONSE_LENGTH) {
                elog("Response byte count mismatch! got: {} expected: {}", size, Command::RESPONSE_LENGTH);
                Return std::unexpected<system_error2::nt_code> { STATUS_UNSUCCESSFUL };
            } else if (not Command::validate_response(response)) {
                const auto got      = array_view<const u8> { std::bit_cast<const u8*>(stdr::data(response)),
                                                             stdr::size(Command::RESPONSE_HEADER) };
                const auto expected = array_view<const u8> { std::bit_cast<const u8*>(stdr::data(Command::RESPONSE_HEADER)),
                                                             stdr::size(Command::RESPONSE_HEADER) };
                elog("Response bytes mismatch! got: {::#x} expected: {::#x}", got, expected);
                Return std::unexpected<system_error2::nt_code> { STATUS_UNSUCCESSFUL };
            }
        }

        return { std::move(response) };
    }

    template<typename Command, Validate VALIDATE, typename... Args>
    STORMKIT_FORCE_INLINE
    inline auto send_command_receive_response_sync(const auto& ctx, Args&&... args)
      -> Expected<array<byte, Command::RESPONSE_LENGTH>> {
        DiscardTry(send_command_sync<Command>(ctx, std::forward<Args>(args)...));
        Return Try((receive_command_sync<Command, VALIDATE>(ctx)));
    }

    template<Transport                           TRANSPORT_,
             Command_id                          ID_,
             typename Subcommand_enum<ID_>::type SUB_ID_,
             CommandData                         DATA,
             auto                                FILL_PAYLOAD>
    template<typename... Args>
             STORMKIT_FORCE_INLINE
    constexpr auto Command<TRANSPORT_, ID_, SUB_ID_, DATA, FILL_PAYLOAD>::make_payload(Args&&... args) noexcept
      -> array<byte, REQUEST_PAYLOAD_LENGTH> {
        auto out = array<byte, REQUEST_PAYLOAD_LENGTH> {};
        FILL_PAYLOAD(out, std::forward<Args>(args)...);
        return out;
    }

    template<Transport                           TRANSPORT_,
             Command_id                          ID_,
             typename Subcommand_enum<ID_>::type SUB_ID_,
             CommandData                         DATA,
             auto                                FILL_PAYLOAD>
    template<typename... Args>
    constexpr auto Command<TRANSPORT_, ID_, SUB_ID_, DATA, FILL_PAYLOAD>::make_request(Args&&... args) noexcept
      -> array<byte, REQUEST_LENGTH> {
        const auto payload = make_payload(std::forward<Args>(args)...);
        ENSURES(stdr::size(payload) == REQUEST_PAYLOAD_LENGTH);

        auto request = array<byte, REQUEST_LENGTH> {};
        stdr::copy(stdv::concat(REQUEST_HEADER, payload), stdr::begin(request));
        return request;
    }

    template<Transport                           TRANSPORT_,
             Command_id                          ID_,
             typename Subcommand_enum<ID_>::type SUB_ID_,
             CommandData                         DATA,
             auto                                FILL_PAYLOAD>
    constexpr auto Command<TRANSPORT_, ID_, SUB_ID_, DATA, FILL_PAYLOAD>::make_response() noexcept
      -> array<byte, RESPONSE_LENGTH> {
        auto response = array<byte, RESPONSE_LENGTH> {};
        stdr::copy(RESPONSE_HEADER, stdr::begin(response));
        return response;
    }

    template<Transport                           TRANSPORT_,
             Command_id                          ID_,
             typename Subcommand_enum<ID_>::type SUB_ID_,
             CommandData                         DATA,
             auto                                FILL_PAYLOAD>
             STORMKIT_FORCE_INLINE
    constexpr auto Command<TRANSPORT_, ID_, SUB_ID_, DATA, FILL_PAYLOAD>::validate_response(array_view<const byte>
                                                                                              response) noexcept -> bool {
        return std::memcmp(std::bit_cast<void*>(stdr::data(response)),
                           std::bit_cast<void*>(stdr::data(RESPONSE_HEADER)),
                           stdr::size(RESPONSE_HEADER))
               == 0;
    }
} // namespace lj::hid

module: private;

namespace lj::hid {
    namespace {
        auto request_copy_from_buffer(WDFREQUEST request, array_view<const byte> to) -> Expected<usize> {
            auto memory = WDFMEMORY {};
            LoggedTry(lj::win_call(WdfRequestRetrieveOutputMemory, request, &memory), "WdfRequestRetrieveOutputMemory failed!");

            auto output_buffer_extent = 0_usize;
            WdfMemoryGetBuffer(memory, &output_buffer_extent);
            if (output_buffer_extent < stdr::size(to)) {
                lj::elog("request_copy_from_buffer: buffer too small! Size {}, expects {}\n",
                         output_buffer_extent,
                         stdr::size(to));
                Return std::unexpected<system_error2::nt_code> { std::in_place, STATUS_INVALID_BUFFER_SIZE };
            }

            LoggedTry(lj::win_call(WdfMemoryCopyFromBuffer, memory, 0, bit_cast<void*>(stdr::data(to)), stdr::size(to)),
                      "WdfMemoryCopyFromBuffer failed!");

            WdfRequestSetInformation(request, stdr::size(to));

            Return { output_buffer_extent };
        }
    } // namespace

    auto get_device_descriptor(WDFREQUEST& request, const HID_DESCRIPTOR& descriptor) noexcept -> Expected<void> {
        DiscardTry(request_copy_from_buffer(request, as_bytes(descriptor)));
        Return {};
    }

    auto get_device_attributes(WDFREQUEST& request, const HID_DEVICE_ATTRIBUTES& attributes) noexcept -> Expected<void> {
        DiscardTry(request_copy_from_buffer(request, as_bytes(attributes)));
        Return {};
    }

    auto get_report_descriptor(WDFREQUEST& request, const Report_descriptor& descriptor) noexcept -> Expected<void> {
        DiscardTry(request_copy_from_buffer(request, as_bytes(descriptor)));
        Return {};
    }

    auto read_report(WDFREQUEST& request, array_view<byte> to) -> Expected<void> {
        const auto size = Try(request_copy_from_buffer(request, to));

        dlog("readed {} byte(s):\n{}", size, array_view<const u8> { std::bit_cast<const u8*>(stdr::data(to)), size });

        Return {};
    }

    auto write_report(WDFREQUEST& request, array_view<const byte> from) -> Expected<void> {
        auto raw_buffer  = PVOID { nullptr };
        auto buffer_size = 0_usize;

        Try(lj::win_call(WdfRequestRetrieveInputBuffer, request, sizeof(u32), &raw_buffer, &buffer_size));

        auto data = array_view<const byte> { std::bit_cast<const byte*>(raw_buffer), buffer_size };
        // stdr::copy(from, to);

        // const auto size = Try(request_copy_from_buffer(request, from));

        dlog("wrote {} byte(s):\n{}",
             buffer_size,
             array_view<const u8> { std::bit_cast<const u8*>(stdr::data(data)), buffer_size });

        Return {};
    }

    auto get_string(WDFREQUEST& request, string_view product_string, string_view serial_string) -> Expected<void> {
        auto raw_buffer  = PVOID { nullptr };
        auto buffer_size = 0_usize;

        auto string_id = 0_u32;

        Try(lj::win_call(WdfRequestRetrieveInputBuffer, request, sizeof(u32), &raw_buffer, &buffer_size));

        string_id = *std::bit_cast<u32*>(raw_buffer) & 0xFFFF;

        const auto is_serial = (string_id == 16 or string_id == 3); // HID_STRING_ID_ISERIALNUMBER

        if (is_serial) Try(request_copy_from_buffer(request, as_bytes(stdr::data(serial_string), stdr::size(serial_string))));
        else
            Try(request_copy_from_buffer(request, as_bytes(stdr::data(product_string), stdr::size(product_string))));

        Return {};
    }

    auto get_indexed_string(WDFREQUEST& request, string_view product_string) -> Expected<void> {
        Try(request_copy_from_buffer(request, as_bytes(stdr::data(product_string), stdr::size(product_string))));

        Return {};
    }
} // namespace lj::hid
