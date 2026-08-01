export module lesserjoy.hid:unknown_0x18_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;

export namespace lj::hid::unknown_0x18 {
    template<Transport TRANSPORT, Subcommand_id SUB_ID, CommandData DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using Command = hid::Command<TRANSPORT, Command_id::UNKNOWN_0x18, SUB_ID, DATA, FILL_PAYLOAD>;

    template<Transport TRANSPORT>
    using Unknown_0x01_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x01, CommandData { .report_payload_length = 0x08 }>;

    template<Transport TRANSPORT>
    using Unknown_0x03_command = Command<TRANSPORT,
                                         Subcommand_id::UNKNOWN_0x03,
                                         CommandData { .command_payload_length = 0x01, .report_payload_length = 0x01 }>;
} // namespace lj::hid::unknown_0x18
