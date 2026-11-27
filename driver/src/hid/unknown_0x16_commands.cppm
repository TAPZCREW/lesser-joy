export module lesserjoy:hid.unknown_0x16_commands;

import std;

import stormkit.core;

import :hid.command_ids;

using namespace stormkit;

export namespace lj::hid::unknown_0x16 {
    template<subcommand_id Sub_id, command_data Data = {}, auto Fill_payload = monadic::noop()>
    using command = hid::command<command_id::unknown_0x16, Sub_id, Data, Fill_payload>;

    using unknown_0x01_command = command<subcommand_id::unknown_0x01, command_data { .report_payload_length = 0x18 }>;
} // namespace lj::hid::unknown_0x16
