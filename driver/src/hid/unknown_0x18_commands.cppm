export module lesserjoy:hid.unknown_0x18_commands;

import std;

import stormkit.core;

import :hid.command_ids;

using namespace stormkit;

export namespace lj::hid::unknown_0x18 {
    template<subcommand_id Sub_id, command_data Data = {}, auto Fill_payload = monadic::noop()>
    using command = hid::command<command_id::unknown_0x18, Sub_id, Data, Fill_payload>;

    using unknown_0x01_command = command<subcommand_id::unknown_0x01, command_data { .report_payload_length = 0x08 }>;
    using unknown_0x03_command = command<subcommand_id::unknown_0x03,
                                         command_data { .command_payload_length = 0x01, .report_payload_length = 0x01 }>;
} // namespace lj::hid::unknown_0x18
