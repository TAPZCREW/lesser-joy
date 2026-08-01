export module lesserjoy.hid:feature_select_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;

export namespace lj::hid::feature_select {
    template<Transport TRANSPORT, Subcommand_id SUB_ID, CommandData DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using Command = hid::Command<TRANSPORT, Command_id::FEATURE_SELECT, SUB_ID, DATA, FILL_PAYLOAD>;

    template<Transport TRANSPORT>
    using Get_feature_info_command = Command<
      TRANSPORT,
      Subcommand_id::GET_FEATURE_INFO,
      { .command_payload_length = 0x04, .report_payload_length = 0x0B },
      [](array_view<byte, 0x04> payload, Feature_flag flags) static noexcept { payload[0] = narrow<byte>(flags); }>;

    template<Transport TRANSPORT>
    using Set_feature_mask_command = Command<
      TRANSPORT,
      Subcommand_id::SET_FEATURE_MASK,
      { .command_payload_length = 0x04, .report_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, Feature_flag flags) static noexcept { payload[0] = narrow<byte>(flags); }>;

    template<Transport TRANSPORT>
    using Clear_feature_mask_command = Command<TRANSPORT,
                                               Subcommand_id::CLEAR_FEATURE_MASK,
                                               { .command_payload_length = 0x04, .report_payload_length = 0x04 }>;

    template<Transport TRANSPORT>
    using Enable_features_command = Command<
      TRANSPORT,
      Subcommand_id::ENABLE_FEATURES,
      { .command_payload_length = 0x04, .report_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, Feature_flag flags) static noexcept { payload[0] = narrow<byte>(flags); }>;

    template<Transport TRANSPORT>
    using Disable_features_command = Command<
      TRANSPORT,
      Subcommand_id::DISABLE_FEATURES,
      { .command_payload_length = 0x04, .report_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, Feature_flag flags) static noexcept { payload[0] = narrow<byte>(flags); }>;

    template<Transport TRANSPORT>
    using Configure_features_command = Command<
      TRANSPORT,
      Subcommand_id::CONFIGURE_FEATURES,
      { .command_payload_length = 0x0A, .report_payload_length = 0x28 },
      [](array_view<byte, 0x0A> payload, Feature_flag flags) static noexcept { payload[0] = narrow<byte>(flags); }>;
} // namespace lj::hid::feature_select
