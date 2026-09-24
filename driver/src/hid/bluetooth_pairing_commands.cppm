export module lesserjoy.hid:bluetooth_pairing_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;
using namespace stormkit::literals;

export namespace lj::hid::bluetooth_pairing {
    template<transport TRANSPORT, subcommand_id SUB_ID, command_data DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using command = hid::command<TRANSPORT, command_id::BLUETOOTH_PAIRING, SUB_ID, DATA, FILL_PAYLOAD>;

    template<transport TRANSPORT>
    using exchange_bluetooth_address_command = command<
      TRANSPORT,
      subcommand_id::EXCHANGE_BLUETOOTH_ADDRESS,
      command_data { .command_payload_length = 0x08, .report_payload_length = 0x09 },
      [](array_view<byte, 0x08> payload) static noexcept {
          payload[1] = 0x01_b;
          stdr::copy(into<array>(as_bytes, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }), stdr::begin(payload) + 2);
      }>;

    template<transport TRANSPORT>
    using confirm_ltk_command = command<
      TRANSPORT,
      subcommand_id::CONFIRM_LTK,
      command_data { .command_payload_length = 0x11, .report_payload_length = 0x11 },
      [](array_view<byte, 0x11> payload) static noexcept {
          stdr::copy(into<
                       array>(as_bytes,
                              { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }),
                     stdr::begin(payload) + 1);
      }>;

    template<transport TRANSPORT>
    using finalize_pairing_command = command<TRANSPORT,
                                             subcommand_id::FINALIZE_PAIRING,
                                             command_data { .command_payload_length = 0x01, .report_payload_length = 0x01 }>;

    template<transport TRANSPORT>
    using exchange_ltk_components_command = command<
      TRANSPORT,
      subcommand_id::EXCHANGE_LTK_COMPONENTS,
      command_data { .command_payload_length = 0x11, .report_payload_length = 0x11 },
      [](array_view<byte, 0x11> payload) static noexcept {
          stdr::copy(into<
                       array>(as_bytes,
                              { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }),
                     stdr::begin(payload) + 1);
      }>;
} // namespace lj::hid::bluetooth_pairing
