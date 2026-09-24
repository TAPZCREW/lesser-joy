export module lesserjoy.hid:unknown_0x07_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;

export namespace lj::hid::unknown_0x07 {
    template<transport TRANSPORT, subcommand_id SUB_ID, command_data DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using command = hid::command<TRANSPORT, command_id::UNKNOWN_0x07, SUB_ID, DATA, FILL_PAYLOAD>;

    template<transport TRANSPORT>
    using unknown_0x01_command = command<TRANSPORT, subcommand_id::UNKNOWN_0x01, command_data { .report_payload_length = 0x01 }>;
    template<transport TRANSPORT>
    using unknown_0x02_command = command<TRANSPORT, subcommand_id::UNKNOWN_0x02>;
} // namespace lj::hid::unknown_0x07
