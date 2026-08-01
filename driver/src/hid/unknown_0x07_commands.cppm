export module lesserjoy.hid:unknown_0x07_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;

export namespace lj::hid::unknown_0x07 {
    template<Transport TRANSPORT, Subcommand_id SUB_ID, CommandData DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using Command = hid::Command<TRANSPORT, Command_id::UNKNOWN_0x07, SUB_ID, DATA, FILL_PAYLOAD>;

    template<Transport TRANSPORT>
    using Unknown_0x01_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x01, CommandData { .report_payload_length = 0x01 }>;
    template<Transport TRANSPORT>
    using Unknown_0x02_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x02>;
} // namespace lj::hid::unknown_0x07
