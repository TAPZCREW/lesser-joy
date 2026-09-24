export module lesserjoy.hid:feature_select_commands;

import std;

import stormkit.core;

import :command_ids;

using namespace stormkit;

export namespace lj::hid::feature_select {
    template<transport TRANSPORT, subcommand_id SUB_ID, command_data DATA = {}, auto FILL_PAYLOAD = monadic::noop()>
    using command = hid::command<TRANSPORT, command_id::FEATURE_SELECT, SUB_ID, DATA, FILL_PAYLOAD>;

    template<transport TRANSPORT>
    using get_feature_info_command = command<
      TRANSPORT,
      subcommand_id::GET_FEATURE_INFO,
      { .command_payload_length = 0x04, .report_payload_length = 0x0B },
      [](array_view<byte, 0x04> payload, feature_flag flags) static noexcept { payload[0] = as<byte>(flags); }>;

    template<transport TRANSPORT>
    using set_feature_mask_command = command<
      TRANSPORT,
      subcommand_id::SET_FEATURE_MASK,
      { .command_payload_length = 0x04, .report_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, feature_flag flags) static noexcept { payload[0] = as<byte>(flags); }>;

    template<transport TRANSPORT>
    using clear_feature_mask_command = command<TRANSPORT,
                                               subcommand_id::CLEAR_FEATURE_MASK,
                                               { .command_payload_length = 0x04, .report_payload_length = 0x04 }>;

    template<transport TRANSPORT>
    using enable_features_command = command<
      TRANSPORT,
      subcommand_id::ENABLE_FEATURES,
      { .command_payload_length = 0x04, .report_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, feature_flag flags) static noexcept { payload[0] = as<byte>(flags); }>;

    template<transport TRANSPORT>
    using disable_features_command = command<
      TRANSPORT,
      subcommand_id::DISABLE_FEATURES,
      { .command_payload_length = 0x04, .report_payload_length = 0x04 },
      [](array_view<byte, 0x04> payload, feature_flag flags) static noexcept { payload[0] = as<byte>(flags); }>;

    template<transport TRANSPORT>
    using configure_features_command = command<
      TRANSPORT,
      subcommand_id::CONFIGURE_FEATURES,
      { .command_payload_length = 0x0A, .report_payload_length = 0x28 },
      [](array_view<byte, 0x0A> payload, feature_flag flags) static noexcept { payload[0] = as<byte>(flags); }>;
} // namespace lj::hid::feature_select
