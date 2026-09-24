export module lesserjoy.hid:init_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;
using namespace stormkit::literals;

export namespace lj::hid::init {
    template<transport TRANSPORT, subcommand_id SUB_ID, command_data DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using command = hid::command<TRANSPORT, command_id::INIT, SUB_ID, DATA, FILL_PAYLOAD>;

    template<transport TRANSPORT>
    using bt_wake_command = command<
      TRANSPORT,
      subcommand_id::BT_WAKE,
      command_data { .command_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, bool enabled = true) static noexcept { payload[0] = (enabled) ? 0x01_b : 0x00_b; }>;
    template<transport TRANSPORT>
    using bt_cancel_command = command<TRANSPORT, subcommand_id::BT_CANCEL>;

    template<transport TRANSPORT>
    using enable_usb_hid_report_command = command<
      TRANSPORT,
      subcommand_id::ENABLE_USB_HID_REPORT,
      command_data { .command_payload_length = 0x04, .report_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, bool enabled = true) static noexcept { payload[0] = (enabled) ? 0x01_b : 0x00_b; }>;
    template<transport TRANSPORT>
    using unknown_0x04_command = command<TRANSPORT, subcommand_id::UNKNOWN_0x04>;
    template<transport TRANSPORT>
    using unknown_0x05_command = command<TRANSPORT, subcommand_id::UNKNOWN_0x05>;
    template<transport TRANSPORT>
    using unknown_0x06_command = command<TRANSPORT, subcommand_id::UNKNOWN_0x06>;
    template<transport TRANSPORT>
    using send_pairing_info_command = command<TRANSPORT,
                                              subcommand_id::SEND_PAIRING_INFO,
                                              command_data { .command_payload_length = 0x16 }>;
    template<transport TRANSPORT>
    using clear_pairing_info_command = command<TRANSPORT, subcommand_id::CLEAR_PAIRING_INFO>;
    template<transport TRANSPORT>
    using store_pairing_command = command<TRANSPORT, subcommand_id::STORE_PAIRING_INFO>;

    template<transport TRANSPORT>
    using select_input_report_command = command<
      TRANSPORT,
      subcommand_id::SELECT_INPUT_REPORT,
      command_data { .command_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, input_report_id id) static noexcept { payload[0] = as<byte>(id); }>;
    template<transport TRANSPORT>
    using unknown_0x0C_command = command<TRANSPORT,
                                         subcommand_id::UNKNOWN_0x0C,
                                         command_data { .command_payload_length = 0x04 },
                                         [](array_view<byte, 0x04> payload) static noexcept { payload[0] = 0x01_b; }>;
    template<transport TRANSPORT>
    using initialize_usb_command = command<TRANSPORT,
                                           subcommand_id::INITIALIZE_USB,
                                           command_data { .command_payload_length = 0x08, .report_payload_length = 0x04 },
                                           [](array_view<byte, 0x08> payload) static noexcept {
                                               payload[0] = 0x01_b;
                                               stdr::copy(into<array>(as_bytes, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }),
                                                          stdr::begin(payload) + 2);
                                           }>;
    template<transport TRANSPORT>
    using unknown_0x0F_command = command<TRANSPORT, subcommand_id::UNKNOWN_0x0F, command_data { .report_payload_length = 0x04 }>;
} // namespace lj::hid::init
