module;

#include <stormkit/core/platform_macro.hpp>

#include "../contract_macro.hpp"

export module lesserjoy:hid.command_ids;

import std;

import stormkit.core;

import :constants;

using namespace stormkit;
using namespace stormkit::literals;

namespace stdr = std::ranges;
namespace stdv = std::views;

export {
    namespace lj::hid {
        // @see
        // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md
        enum class command_id : u8 {
            init              = 0x03,
            unknown_0x07      = 0x07,
            leds              = 0x09,
            vibration         = 0x0A,
            feature_select    = 0x0C,
            unknown_0x11      = 0x11,
            bluetooth_pairing = 0x15,
            unknown_0x16      = 0x16,
            unknown_0x18      = 0x18,
        };

        namespace init {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x03---initialisation
            enum class subcommand_id : u8 {
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x01---bluetooth-wake
                bt_wake = 0x01,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x02---bluetooth-cancel
                bt_cancel = 0x02,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x03---enable-usb-hid-reports
                enable_usb_hid_report = 0x03,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x07---send-pairing-info
                unknown_0x04 = 0x04,
                unknown_0x05 = 0x05,
                unknown_0x06 = 0x06,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x07---send-pairing-info
                send_pairing_info = 0x07,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x08---clear-pairing-info
                clear_pairing_info = 0x08,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x09---store-pairing-info
                store_pairing_info = 0x09,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x0a---select-input-report
                select_input_report = 0x0A,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x0c---initialise-usb
                unknown_0x0c = 0x0C,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x0d---initialise-usb
                initialize_usb = 0x0D,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x0c---initialise-usb
                unknown_0x0f = 0x0F,
            };

            enum class input_report_id {
                generic        = 0x05,
                alt_joycon_l_2 = 0x07,
                alt_joycon_r_2 = 0x08,
                alt_procon_2   = 0x09,
                alt_nso_gc_2   = 0x01,
            };

            enum class output_report_id {
                joycon_l_2 = 0x01,
                joycon_r_2 = 0x01,
                procon_2   = 0x02,
                nso_gc_2   = 0x03,
            };
        } // namespace init

        namespace unknown_0x07 {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x07---unknown
            enum class subcommand_id : u8 {
                unknown_0x01 = 0x01,
                unknown_0x02 = 0x02,
            };
        } // namespace unknown_0x07

        namespace leds {
            enum class subcommand_id : u8 {
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x01---set-player-1-led
                set_player_1 = 0x01,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x02---set-player-2-led
                set_player_2 = 0x02,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x03---set-player-3-led
                set_player_3 = 0x03,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x04---set-player-4-led
                set_player_4 = 0x04,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x05---set-all-leds-on
                all_leds_on = 0x05,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x06---set-all-leds-off
                all_leds_off = 0x06,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x07---set-led-pattern
                set_player_led_mask = 0x07,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x08---flash-leds
                flash_leds = 0x08,
            };

            enum class player : u8 {
                player_1 = 0x1,
                player_2 = 0x2,
                player_3 = 0x4,
                player_4 = 0x8,
            };
        } // namespace leds

        namespace vibration {
            enum class subcommand_id : u8 {
                unknown_0x01 = 0x01,
                play_sample  = 0x02,
                unknown_0x03 = 0x03,
                unknown_0x04 = 0x04,
                unknown_0x05 = 0x05,
                unknown_0x06 = 0x06,
                unknown_0x07 = 0x07,
                send_data    = 0x08,
                unknown_0x09 = 0x09,
            };

            enum class sample : u8 {
                silence              = 0x0,
                low_frequency_buzz   = 0x1,
                high_frequency_buzz  = 0x2,
                soft_click           = 0x3,
                high_beep_beep_sound = 0x4,
                stronger_soft_click  = 0x5,
                high_beep_sound      = 0x6,
                higher_beep_sound    = 0x7,
            };
        } // namespace vibration

        namespace feature_select {
            enum class subcommand_id : u8 {
                get_feature_info   = 0x01,
                set_feature_mask   = 0x02,
                clear_feature_mask = 0x03,
                enable_features    = 0x04,
                disable_features   = 0x05,
                configure_features = 0x06,
            };

            enum class feature_flag : u8 {
                button_state  = 0x01,
                analog_sticks = 0x02,
                imu           = 0x04,
                unused_1      = 0x08,
                mouse_data    = 0x10,
                rumble        = 0x20,
                unused_2      = 0x40,
                magnetometer  = 0x80,
            };
        } // namespace feature_select

        namespace unknown_0x11 {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x11---unknown
            enum class subcommand_id : u8 {
                unknown_0x01 = 0x01,
                unknown_0x03 = 0x03,
                unknown_0x04 = 0x04,
            };
        } // namespace unknown_0x11

        namespace bluetooth_pairing {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x15---bluetooth-pairing
            enum class subcommand_id : u8 {
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x01---exchange-addresses
                exchange_bluetooth_address = 0x01,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x02---confirm-ltk
                confirm_ltk = 0x02,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x03---finalise-pairing
                finalize_pairing = 0x03,
                // @see
                // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x04---exchange-keys
                exchange_ltk_components = 0x04,
            };
        } // namespace bluetooth_pairing

        namespace unknown_0x16 {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x16---unknown
            enum class subcommand_id : u8 {
                unknown_0x01 = 0x01,
            };
        } // namespace unknown_0x16

        namespace unknown_0x18 {
            // @see
            // https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#command-0x18---unknown
            enum class subcommand_id : u8 {
                unknown_0x01 = 0x01,
                unknown_0x03 = 0x03,
            };
        } // namespace unknown_0x18

        template<command_id>
        struct subcommand_enum;

        template<>
        struct subcommand_enum<command_id::init> {
            using type = init::subcommand_id;
        };

        template<>
        struct subcommand_enum<command_id::unknown_0x07> {
            using type = unknown_0x07::subcommand_id;
        };

        template<>
        struct subcommand_enum<command_id::leds> {
            using type = leds::subcommand_id;
        };

        template<>
        struct subcommand_enum<command_id::vibration> {
            using type = vibration::subcommand_id;
        };

        template<>
        struct subcommand_enum<command_id::feature_select> {
            using type = feature_select::subcommand_id;
        };

        template<>
        struct subcommand_enum<command_id::unknown_0x11> {
            using type = unknown_0x11::subcommand_id;
        };

        template<>
        struct subcommand_enum<command_id::bluetooth_pairing> {
            using type = bluetooth_pairing::subcommand_id;
        };

        template<>
        struct subcommand_enum<command_id::unknown_0x16> {
            using type = unknown_0x16::subcommand_id;
        };

        template<>
        struct subcommand_enum<command_id::unknown_0x18> {
            using type = unknown_0x18::subcommand_id;
        };

        struct command_data {
            u8 command_payload_length = 0x00;
            u8 report_payload_length  = 0x00;
        };

        template<command_id                         Id,
                 typename subcommand_enum<Id>::type Sub_id,
                 command_data                       Data         = {},
                 auto                               Fill_payload = monadic::noop()>
        struct command {
            static constexpr auto id     = Id;
            static constexpr auto sub_id = Sub_id;

            template<transport_type Transport>
            static constexpr auto ack                    = Transport == transport_type::usb ? 0xF8 : 0x78;
            static constexpr auto command_payload_length = Data.command_payload_length;
            static constexpr auto report_payload_length  = Data.report_payload_length;

            template<transport_type Transport>
            static constexpr auto command_header = into<
              array>(as_bytes,
                     { as<u8>(id),
                       as<u8>(direction::host_to_device),
                       as<u8>(Transport),
                       as<u8>(sub_id),
                       0x00_u8,
                       as<u8>(command_payload_length),
                       0x00_u8,
                       0x00_u8 });
            template<transport_type Transport>
            static constexpr auto report_header = into<
              array>(as_bytes,
                     { as<u8>(id),
                       as<u8>(direction::device_to_host),
                       as<u8>(Transport),
                       as<u8>(sub_id),
                       0x00_u8,
                       as<u8>(ack<Transport>),
                       0x00_u8,
                       0x00_u8 });

            // usb and ble have same size
            static constexpr auto command_length = stdr::size(command_header<transport_type::usb>) + command_payload_length;
            static constexpr auto report_length  = stdr::size(report_header<transport_type::usb>) + report_payload_length;
            static_assert(report_length <= 64);

            template<typename... Ts>
            static constexpr auto make_payload(Ts&&... args) noexcept -> array<byte, command_payload_length>;
            template<transport_type Transport, typename... Ts>
            static constexpr auto make_command(Ts&&... args) noexcept -> array<byte, command_length>;
            template<transport_type Transport>
            static constexpr auto make_report() noexcept -> array<byte, report_length>;
            template<transport_type Transport>
            static constexpr auto validate_report(array_view<const byte, report_length> report) noexcept -> bool;
        };
    } // namespace lj::hid

    template<>
    inline constexpr auto stormkit::core::meta::flag_trait<lj::hid::leds::player> = true;
    template<>
    inline constexpr auto stormkit::core::meta::flag_trait<lj::hid::feature_select::feature_flag> = true;
}

////////////////////////////////////////////////////////////////////
///                      IMPLEMENTATION                          ///
////////////////////////////////////////////////////////////////////

namespace lj::hid {
    ////////////////////////////////////////
    ////////////////////////////////////////
    template<command_id Id, typename subcommand_enum<Id>::type Sub_id, command_data Data, auto Fill_payload>
    template<typename... Ts>
    STORMKIT_FORCE_INLINE
    constexpr auto command<Id, Sub_id, Data, Fill_payload>::make_payload(Ts&&... args) noexcept
      -> array<byte, command_payload_length> {
        auto out = array<byte, command_payload_length> {};
        Fill_payload(out, std::forward<Ts>(args)...);
        return out;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<command_id Id, typename subcommand_enum<Id>::type Sub_id, command_data Data, auto Fill_payload>
    template<transport_type Transport, typename... Ts>
    constexpr auto command<Id, Sub_id, Data, Fill_payload>::make_command(Ts&&... args) noexcept -> array<byte, command_length> {
        const auto payload = make_payload(std::forward<Ts>(args)...);
        auto       command = array<byte, command_length> {};
        WDK_ASSERT_RETURN(stdr::size(payload) == command_payload_length, command);

#ifdef STORMKIT_CPPLIB_IS_MSSTL
        stdr::copy(command_header<Transport>, stdr::begin(command));
        stdr::copy(payload, stdr::begin(command) + stdr::size(command_header<Transport>));
#else
        stdr::copy(stdv::concat(command_header<Transport>, payload), stdr::begin(command));
#endif
        return command;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<command_id Id, typename subcommand_enum<Id>::type Sub_id, command_data Data, auto Fill_payload>
    template<transport_type Transport>
    constexpr auto command<Id, Sub_id, Data, Fill_payload>::make_report() noexcept -> array<byte, report_length> {
        auto report = array<byte, report_length> {};
        stdr::copy(report_header<Transport>, stdr::begin(report));
        return report;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<command_id Id, typename subcommand_enum<Id>::type Sub_id, command_data Data, auto Fill_payload>
    template<transport_type Transport>
    STORMKIT_FORCE_INLINE
    constexpr auto command<Id, Sub_id, Data, Fill_payload>::validate_report(array_view<const byte, report_length> report) noexcept
      -> bool {
        return std::memcmp(stdr::data(report), stdr::data(report_header<Transport>), stdr::size(report_header<Transport>)) == 0;
    }
} // namespace lj::hid
