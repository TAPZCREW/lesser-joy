export module lesserjoy.hid:bluetooth_pairing_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;
using namespace stormkit::literals;

export namespace lj::hid::bluetooth_pairing {
    template<Transport TRANSPORT, Subcommand_id SUB_ID, CommandData DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using Command = hid::Command<TRANSPORT, Command_id::BLUETOOTH_PAIRING, SUB_ID, DATA, FILL_PAYLOAD>;

    template<Transport TRANSPORT>
    using Exchange_bluetooth_address_command = Command<
      TRANSPORT,
      Subcommand_id::EXCHANGE_BLUETOOTH_ADDRESS,
      CommandData { .command_payload_length = 0x08, .report_payload_length = 0x09 },
      [](array_view<byte, 0x08> payload) static noexcept {
          payload[1] = 0x01_b;
          stdr::copy(into_bytes({ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }), stdr::begin(payload) + 2);
      }>;

    template<Transport TRANSPORT>
    using Confirm_ltk_command = Command<
      TRANSPORT,
      Subcommand_id::CONFIRM_LTK,
      CommandData { .command_payload_length = 0x11, .report_payload_length = 0x11 },
      [](array_view<byte, 0x11> payload) static noexcept {
          stdr::
            copy(into_bytes({ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }),
                 stdr::begin(payload) + 1);
      }>;

    template<Transport TRANSPORT>
    using Finalize_pairing_command = Command<TRANSPORT,
                                             Subcommand_id::FINALIZE_PAIRING,
                                             CommandData { .command_payload_length = 0x01, .report_payload_length = 0x01 }>;

    template<Transport TRANSPORT>
    using Exchange_ltk_components_command = Command<
      TRANSPORT,
      Subcommand_id::EXCHANGE_LTK_COMPONENTS,
      CommandData { .command_payload_length = 0x11, .report_payload_length = 0x11 },
      [](array_view<byte, 0x11> payload) static noexcept {
          stdr::
            copy(into_bytes({ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }),
                 stdr::begin(payload) + 1);
      }>;
} // namespace lj::hid::bluetooth_pairing
