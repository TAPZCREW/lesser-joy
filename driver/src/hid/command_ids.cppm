module;

#include <stormkit/core/contract_macro.hpp>
#include <stormkit/core/platform_macro.hpp>

export module lesserjoy.hid:command_ids;

import std;

import stormkit.core;

import lesserjoy.constants;

using namespace stormkit;

namespace stdr = std::ranges;
namespace stdv = std::views;

export {
    namespace lj::hid {
        // @see
        // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md
        enum class Command_id : u8 {
            INIT              = 0x03,
            UNKNOWN_0x07      = 0x07,
            LEDS              = 0x09,
            FEATURE_SELECT    = 0x0C,
            UNKNOWN_0x11      = 0x11,
            BLUETOOTH_PAIRING = 0x15,
            UNKNOWN_0x16      = 0x16,
            UNKNOWN_0x18      = 0x18,
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

        namespace unknown_0x07 {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x07---unknown
            enum class Subcommand_id : u8 {
                UNKNOWN_0x01 = 0x01,
                UNKNOWN_0x02 = 0x02,
            };
        } // namespace unknown_0x07

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

        namespace feature_select {
            enum class Subcommand_id : u8 {
                GET_FEATURE_INFO   = 0x01,
                SET_FEATURE_MASK   = 0x02,
                CLEAR_FEATURE_MASK = 0x03,
                ENABLE_FEATURES    = 0x04,
                DISABLE_FEATURES   = 0x05,
                CONFIGURE_FEATURES = 0x06,
            };

            enum class Feature_flag : u8 {
                BUTTON_STATE  = 0x01,
                ANALOG_STICKS = 0x02,
                IMU           = 0x04,
                UNUSED_1      = 0x08,
                MOUSE_DATA    = 0x10,
                RUMBLE        = 0x20,
                UNUSED_2      = 0x40,
                MAGNETOMETER  = 0x80,
            };
        } // namespace feature_select

        namespace unknown_0x11 {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x11---unknown
            enum class Subcommand_id {
                UNKNOWN_0x01 = 0x01,
                UNKNOWN_0x03 = 0x03,
                UNKNOWN_0x04 = 0x04,
            };
        } // namespace unknown_0x11

        namespace bluetooth_pairing {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x15---bluetooth-pairing
            enum class Subcommand_id : u8 {
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x01---exchange-addresses
                EXCHANGE_BLUETOOTH_ADDRESS = 0x01,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x02---confirm-ltk
                CONFIRM_LTK = 0x02,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x03---finalise-pairing
                FINALIZE_PAIRING = 0x03,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x04---exchange-keys
                EXCHANGE_LTK_COMPONENTS = 0x04,
            };
        } // namespace bluetooth_pairing

        namespace unknown_0x16 {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x16---unknown
            enum class Subcommand_id : u8 {
                UNKNOWN_0x01 = 0x01,
            };
        } // namespace unknown_0x16

        namespace unknown_0x18 {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x18---unknown
            enum class Subcommand_id : u8 {
                UNKNOWN_0x01 = 0x01,
                UNKNOWN_0x03 = 0x03,
            };
        } // namespace unknown_0x18

        template<Command_id>
        struct Subcommand_enum;

        template<>
        struct Subcommand_enum<Command_id::INIT> {
            using type = init::Subcommand_id;
        };

        template<>
        struct Subcommand_enum<Command_id::UNKNOWN_0x07> {
            using type = unknown_0x07::Subcommand_id;
        };

        template<>
        struct Subcommand_enum<Command_id::LEDS> {
            using type = leds::Subcommand_id;
        };

        template<>
        struct Subcommand_enum<Command_id::FEATURE_SELECT> {
            using type = feature_select::Subcommand_id;
        };

        template<>
        struct Subcommand_enum<Command_id::UNKNOWN_0x11> {
            using type = unknown_0x11::Subcommand_id;
        };

        template<>
        struct Subcommand_enum<Command_id::BLUETOOTH_PAIRING> {
            using type = bluetooth_pairing::Subcommand_id;
        };

        template<>
        struct Subcommand_enum<Command_id::UNKNOWN_0x16> {
            using type = unknown_0x16::Subcommand_id;
        };

        template<>
        struct Subcommand_enum<Command_id::UNKNOWN_0x18> {
            using type = unknown_0x18::Subcommand_id;
        };

        struct CommandData {
            u8 command_payload_length  = 0x00;
            u8 report_payload_length = 0x00;
        };

        template<Transport                           TRANSPORT_,
                 Command_id                          ID_,
                 typename Subcommand_enum<ID_>::type SUB_ID_,
                 CommandData                         DATA         = {},
                 auto                                FILL_PAYLOAD = monadic::noop()>
        struct Command {
            static constexpr auto ID        = ID_;
            static constexpr auto SUB_ID    = SUB_ID_;
            static constexpr auto TRANSPORT = TRANSPORT_;

            static constexpr auto ACK                     = TRANSPORT == Transport::USB ? 0xF8 : 0x78;
            static constexpr auto COMMAND_PAYLOAD_LENGTH  = DATA.command_payload_length;
            static constexpr auto REPORT_PAYLOAD_LENGTH = DATA.report_payload_length;

            static constexpr auto
              COMMAND_HEADER = into_bytes({ narrow<u8>(ID),
                                            narrow<u8>(Direction::HOST_TO_DEVICE),
                                            narrow<u8>(TRANSPORT),
                                            narrow<u8>(SUB_ID),
                                            0x00_u8,
                                            narrow<u8>(COMMAND_PAYLOAD_LENGTH),
                                            0x00_u8,
                                            0x00_u8 });
            static constexpr auto
              REPORT_HEADER = into_bytes({ narrow<u8>(ID),
                                             narrow<u8>(Direction::DEVICE_TO_HOST),
                                             narrow<u8>(TRANSPORT),
                                             narrow<u8>(SUB_ID),
                                             0x00_u8,
                                             narrow<u8>(ACK),
                                             0x00_u8,
                                             0x00_u8 });

            static constexpr auto COMMAND_LENGTH  = stdr::size(COMMAND_HEADER) + COMMAND_PAYLOAD_LENGTH;
            static constexpr auto REPORT_LENGTH = stdr::size(REPORT_HEADER) + REPORT_PAYLOAD_LENGTH;

            template<typename... Args>
            static constexpr auto make_payload(Args&&... args) noexcept -> array<byte, COMMAND_PAYLOAD_LENGTH>;
            template<typename... Args>
            static constexpr auto make_command(Args&&... args) noexcept -> array<byte, COMMAND_LENGTH>;
            static constexpr auto make_report() noexcept -> array<byte, REPORT_LENGTH>;
            static constexpr auto validate_report(array_view<const byte> report) noexcept -> bool;
        };
    } // namespace lj::hid

    template<>
    inline constexpr auto stormkit::core::meta::FLAG_TRAIT<lj::hid::leds::Player> = true;
    template<>
    inline constexpr auto stormkit::core::meta::FLAG_TRAIT<lj::hid::feature_select::Feature_flag> = true;
}

////////////////////////////////////////////////////////////////////
///                      IMPLEMENTATION                          ///
////////////////////////////////////////////////////////////////////

namespace lj::hid {
    ////////////////////////////////////////
    ////////////////////////////////////////
    template<Transport                           TRANSPORT_,
             Command_id                          ID_,
             typename Subcommand_enum<ID_>::type SUB_ID_,
             CommandData                         DATA,
             auto                                FILL_PAYLOAD>
    template<typename... Args>
             STORMKIT_FORCE_INLINE
    constexpr auto Command<TRANSPORT_, ID_, SUB_ID_, DATA, FILL_PAYLOAD>::make_payload(Args&&... args) noexcept
      -> array<byte, COMMAND_PAYLOAD_LENGTH> {
        auto out = array<byte, COMMAND_PAYLOAD_LENGTH> {};
        FILL_PAYLOAD(out, std::forward<Args>(args)...);
        return out;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<Transport                           TRANSPORT_,
             Command_id                          ID_,
             typename Subcommand_enum<ID_>::type SUB_ID_,
             CommandData                         DATA,
             auto                                FILL_PAYLOAD>
    template<typename... Args>
    constexpr auto Command<TRANSPORT_, ID_, SUB_ID_, DATA, FILL_PAYLOAD>::make_command(Args&&... args) noexcept
      -> array<byte, COMMAND_LENGTH> {
        const auto payload = make_payload(std::forward<Args>(args)...);
        ENSURES(stdr::size(payload) == COMMAND_PAYLOAD_LENGTH);

        auto command = array<byte, COMMAND_LENGTH> {};
        stdr::copy(stdv::concat(COMMAND_HEADER, payload), stdr::begin(command));
        return command;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<Transport                           TRANSPORT_,
             Command_id                          ID_,
             typename Subcommand_enum<ID_>::type SUB_ID_,
             CommandData                         DATA,
             auto                                FILL_PAYLOAD>
    constexpr auto Command<TRANSPORT_, ID_, SUB_ID_, DATA, FILL_PAYLOAD>::make_report() noexcept
      -> array<byte, REPORT_LENGTH> {
        auto report = array<byte, REPORT_LENGTH> {};
        stdr::copy(REPORT_HEADER, stdr::begin(report));
        return report;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<Transport                           TRANSPORT_,
             Command_id                          ID_,
             typename Subcommand_enum<ID_>::type SUB_ID_,
             CommandData                         DATA,
             auto                                FILL_PAYLOAD>
             STORMKIT_FORCE_INLINE
    constexpr auto Command<TRANSPORT_, ID_, SUB_ID_, DATA, FILL_PAYLOAD>::validate_report(array_view<const byte>
                                                                                              report) noexcept -> bool {
        return std::memcmp(std::bit_cast<void*>(stdr::data(report)),
                           std::bit_cast<void*>(stdr::data(REPORT_HEADER)),
                           stdr::size(REPORT_HEADER))
               == 0;
    }
} // namespace lj::hid
