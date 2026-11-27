export module lesserjoy:hid.unknown_0x07_commands;

import std;

import stormkit.core;

import :hid.command_ids;

using namespace stormkit;

export namespace lj::hid::unknown_0x07 {
    template<subcommand_id Sub_id, command_data Data = {}, auto Fill_payload = monadic::noop()>
    using command = hid::command<command_id::unknown_0x07, Sub_id, Data, Fill_payload>;

    using unknown_0x01_command = command<subcommand_id::unknown_0x01, command_data { .report_payload_length = 0x01 }>;
    using unknown_0x02_command = command<subcommand_id::unknown_0x02>;
} // namespace lj::hid::unknown_0x07
