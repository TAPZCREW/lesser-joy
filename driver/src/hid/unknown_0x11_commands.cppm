export module lesserjoy.hid:unknown_0x11_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;

export namespace lj::hid::unknown_0x11 {
    template<transport TRANSPORT, subcommand_id SUB_ID, command_data DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using command = hid::command<TRANSPORT, command_id::UNKNOWN_0x11, SUB_ID, DATA, FILL_PAYLOAD>;

    template<transport TRANSPORT>
    using unknown_0x01_command = command<TRANSPORT, subcommand_id::UNKNOWN_0x01, command_data { .report_payload_length = 0x04 }>;

    template<transport TRANSPORT>
    using unknown_0x03_command = command<TRANSPORT, subcommand_id::UNKNOWN_0x03, command_data { .report_payload_length = 0x1D }>;

    template<transport TRANSPORT>
    using unknown_0x04_command = command<TRANSPORT, subcommand_id::UNKNOWN_0x04>;
} // namespace lj::hid::unknown_0x11
