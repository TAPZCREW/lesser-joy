export module lesserjoy.hid:init_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;
using namespace stormkit::literals;

export namespace lj::hid::init {
    template<Transport TRANSPORT, Subcommand_id SUB_ID, CommandData DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using Command = hid::Command<TRANSPORT, Command_id::INIT, SUB_ID, DATA, FILL_PAYLOAD>;

    template<Transport TRANSPORT>
    using Bt_wake_command = Command<
      TRANSPORT,
      Subcommand_id::BT_WAKE,
      CommandData { .command_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, bool enabled = true) static noexcept { payload[0] = (enabled) ? 0x01_b : 0x00_b; }>;
    template<Transport TRANSPORT>
    using Bt_cancel_command = Command<TRANSPORT, Subcommand_id::BT_CANCEL>;

    template<Transport TRANSPORT>
    using Enable_usb_hid_report_command = Command<
      TRANSPORT,
      Subcommand_id::ENABLE_USB_HID_REPORT,
      CommandData { .command_payload_length = 0x04, .report_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, bool enabled = true) static noexcept { payload[0] = (enabled) ? 0x01_b : 0x00_b; }>;
    template<Transport TRANSPORT>
    using Unknown_0x04_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x04>;
    template<Transport TRANSPORT>
    using Unknown_0x05_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x05>;
    template<Transport TRANSPORT>
    using Unknown_0x06_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x06>;
    template<Transport TRANSPORT>
    using Send_pairing_info_command = Command<TRANSPORT,
                                              Subcommand_id::SEND_PAIRING_INFO,
                                              CommandData { .command_payload_length = 0x16 }>;
    template<Transport TRANSPORT>
    using Clear_pairing_info_command = Command<TRANSPORT, Subcommand_id::CLEAR_PAIRING_INFO>;
    template<Transport TRANSPORT>
    using Store_pairing_command = Command<TRANSPORT, Subcommand_id::STORE_PAIRING_INFO>;

    template<Transport TRANSPORT>
    using Select_input_report_command = Command<
      TRANSPORT,
      Subcommand_id::SELECT_INPUT_REPORT,
      CommandData { .command_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, Input_report_id id) static noexcept { payload[0] = narrow<byte>(id); }>;
    template<Transport TRANSPORT>
    using Unknown_0x0C_command = Command<TRANSPORT,
                                         Subcommand_id::UNKNOWN_0x0C,
                                         CommandData { .command_payload_length = 0x04 },
                                         [](array_view<byte, 0x04> payload) static noexcept { payload[0] = 0x01_b; }>;
    template<Transport TRANSPORT>
    using Initialize_usb_command = Command<TRANSPORT,
                                           Subcommand_id::INITIALIZE_USB,
                                           CommandData { .command_payload_length = 0x08, .report_payload_length = 0x04 },
                                           [](array_view<byte, 0x08> payload) static noexcept {
                                               payload[0] = 0x01_b;
                                               stdr::copy(into_bytes({ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }),
                                                          stdr::begin(payload) + 2);
                                           }>;
    template<Transport TRANSPORT>
    using Unknown_0x0F_command = Command<TRANSPORT, Subcommand_id::UNKNOWN_0x0F, CommandData { .report_payload_length = 0x04 }>;
} // namespace lj::hid::init
