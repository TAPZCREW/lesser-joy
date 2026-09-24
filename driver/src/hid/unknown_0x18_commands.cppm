export module lesserjoy.hid:unknown_0x18_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;

export namespace lj::hid::unknown_0x18 {
    template<transport TRANSPORT, subcommand_id SUB_ID, command_data DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using command = hid::command<TRANSPORT, command_id::UNKNOWN_0x18, SUB_ID, DATA, FILL_PAYLOAD>;

    template<transport TRANSPORT>
    using unknown_0x01_command = command<TRANSPORT, subcommand_id::UNKNOWN_0x01, command_data { .report_payload_length = 0x08 }>;

    template<transport TRANSPORT>
    using unknown_0x03_command = command<TRANSPORT,
                                         subcommand_id::UNKNOWN_0x03,
                                         command_data { .command_payload_length = 0x01, .report_payload_length = 0x01 }>;
} // namespace lj::hid::unknown_0x18
