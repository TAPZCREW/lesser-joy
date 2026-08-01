export module lesserjoy.hid:unknown_0x11_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;

export namespace lj::hid::unknown_0x11 {
    template<Transport TRANSPORT, Subcommand_id SUB_ID, CommandData DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using Command = hid::Command<TRANSPORT, Command_id::UNKNOWN_0x11, SUB_ID, DATA, FILL_PAYLOAD>;

    template<Transport TRANSPORT>
    using Unknown_0x01_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x01, CommandData { .report_payload_length = 0x04 }>;

    template<Transport TRANSPORT>
    using Unknown_0x03_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x03, CommandData { .report_payload_length = 0x1D }>;

    template<Transport TRANSPORT>
    using Unknown_0x04_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x04>;
} // namespace lj::hid::unknown_0x11
