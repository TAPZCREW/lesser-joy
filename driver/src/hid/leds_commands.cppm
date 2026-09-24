export module lesserjoy.hid:leds_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;

export namespace lj::hid::leds {
    template<transport TRANSPORT, subcommand_id SUB_ID, command_data DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using command = hid::command<TRANSPORT, command_id::LEDS, SUB_ID, DATA, FILL_PAYLOAD>;

    template<transport TRANSPORT>
    using set_player_1_command = command<TRANSPORT, subcommand_id::SET_PLAYER_1>;
    template<transport TRANSPORT>
    using set_player_2_command = command<TRANSPORT, subcommand_id::SET_PLAYER_2>;
    template<transport TRANSPORT>
    using set_player_3_command = command<TRANSPORT, subcommand_id::SET_PLAYER_3>;
    template<transport TRANSPORT>
    using set_player_4_command = command<TRANSPORT, subcommand_id::SET_PLAYER_4>;
    template<transport TRANSPORT>
    using All_leds_on_command = command<TRANSPORT, subcommand_id::ALL_LEDS_ON>;
    template<transport TRANSPORT>
    using All_leds_off_command = command<TRANSPORT, subcommand_id::ALL_LEDS_OFF>;
    template<transport TRANSPORT>
    using set_player_led_mask = command<TRANSPORT,
                                        subcommand_id::SET_PLAYER_LED_MASK,
                                        command_data { .command_payload_length = 0x08 }>;
    template<transport TRANSPORT>
    using Flash_leds_command = command<TRANSPORT, subcommand_id::FLASH_LEDS, command_data { .command_payload_length = 0x04 }>;
} // namespace lj::hid::leds
