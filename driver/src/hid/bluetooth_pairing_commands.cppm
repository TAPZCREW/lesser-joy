export module lesserjoy:hid.bluetooth_pairing_commands;

import std;

import stormkit.core;

import :hid.command_ids;

using namespace stormkit;
using namespace stormkit::literals;

export namespace lj::hid::bluetooth_pairing {
    namespace fill_payload {
        inline constexpr auto exchange_bluetooth_address = [](array_view<byte, 0x08> payload) static noexcept {
            static constexpr auto payload_tpl = into<array>(as_bytes, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
            payload[1]                        = 0x01_b;
            stdr::copy(payload_tpl, stdr::begin(payload) + 2);
        };

        inline constexpr auto confirm_ltk = [](array_view<byte, 0x11> payload) static noexcept {
            static constexpr auto payload_tpl = into<
              array>(as_bytes,
                     { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
            stdr::copy(payload_tpl, stdr::begin(payload) + 1);
        };

        inline constexpr auto exchange_ltk_components = [](array_view<byte, 0x11> payload) static noexcept {
            static constexpr auto payload_tpl = into<
              array>(as_bytes,
                     { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
            stdr::copy(payload_tpl, stdr::begin(payload) + 1);
        };
    } // namespace fill_payload

    template<subcommand_id Sub_id, command_data Data = {}, auto Fill_payload = monadic::noop()>
    using command = hid::command<command_id::bluetooth_pairing, Sub_id, Data, Fill_payload>;

    using exchange_bluetooth_address_command = command<
      subcommand_id::exchange_bluetooth_address,
      command_data { .command_payload_length = 0x08, .report_payload_length = 0x09 },
      fill_payload::exchange_bluetooth_address>;

    using confirm_ltk_command = command<subcommand_id::confirm_ltk,
                                        command_data { .command_payload_length = 0x11, .report_payload_length = 0x11 },
                                        fill_payload::confirm_ltk>;

    using finalize_pairing_command = command<subcommand_id::finalize_pairing,
                                             command_data { .command_payload_length = 0x01, .report_payload_length = 0x01 }>;

    using exchange_ltk_components_command = command<
      subcommand_id::exchange_ltk_components,
      command_data { .command_payload_length = 0x11, .report_payload_length = 0x11 },
      fill_payload::exchange_ltk_components>;
} // namespace lj::hid::bluetooth_pairing
