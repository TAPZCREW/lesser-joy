export module lesserjoy:hid.leds_commands;

import std;

import stormkit.core;

import :hid.command_ids;

using namespace stormkit;

export namespace lj::hid::leds {
    template<subcommand_id Sub_id, command_data Data = {}, auto Fill_payload = monadic::noop()>
    using command = hid::command<command_id::leds, Sub_id, Data, Fill_payload>;

    using set_player_1_command = command<subcommand_id::set_player_1>;
    using set_player_2_command = command<subcommand_id::set_player_2>;
    using set_player_3_command = command<subcommand_id::set_player_3>;
    using set_player_4_command = command<subcommand_id::set_player_4>;
    using all_leds_on_command  = command<subcommand_id::all_leds_on>;
    using all_leds_off_command = command<subcommand_id::all_leds_off>;
    using set_player_led_mask  = command<subcommand_id::set_player_led_mask, command_data { .command_payload_length = 0x08 }>;
    using flash_leds_command   = command<subcommand_id::flash_leds, command_data { .command_payload_length = 0x04 }>;
} // namespace lj::hid::leds
