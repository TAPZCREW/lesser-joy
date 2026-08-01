export module lesserjoy.hid:leds_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;

export namespace lj::hid::leds {
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
                                        CommandData { .command_payload_length = 0x08 }>;
    template<Transport TRANSPORT>
    using Flash_leds_command = Command<TRANSPORT, Subcommand_id::FLASH_LEDS, CommandData { .command_payload_length = 0x04 }>;
} // namespace lj::hid::leds
