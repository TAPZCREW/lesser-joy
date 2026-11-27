export module lesserjoy:hid.vibration_commands;

import std;

import stormkit.core;

import :constants;
import :hid.command_ids;

using namespace stormkit;

namespace stdr = std::ranges;

export namespace lj::hid::vibration {
    namespace fill_payload {
        inline constexpr auto play_sample = [](array_view<byte, 0x04> payload, vibration::sample sample) static noexcept {
            payload[0] = as<byte>(sample);
        };

        // @see https://github.com/ndeadly/switch2_controller_research/blob/master/commands.md#subcommand-0x08---send-vibration-data
        // the 20-byte payload format is not understood upstream, so this variant simply forwards caller
        // supplied bytes: that turns the command into a probe surface instead of a fixed 20 zero bytes.
        inline constexpr auto send_data = [](array_view<byte, 0x14> payload, array_view<const byte> data) static noexcept {
            stdr::copy_n(stdr::begin(data), std::min(payload.size(), data.size()), stdr::begin(payload));
        };
    } // namespace fill_payload

    template<subcommand_id Sub_id, command_data Data = {}, auto Fill_payload = monadic::noop()>
    using command = hid::command<command_id::vibration, Sub_id, Data, Fill_payload>;

    using unknown_0x01_command = command<subcommand_id::unknown_0x01>;
    using play_sample_command  = command<subcommand_id::play_sample,
                                         command_data { .command_payload_length = 0x04 },
                                         fill_payload::play_sample>;
    using unknown_0x03_command = command<subcommand_id::unknown_0x03>;
    using unknown_0x04_command = command<subcommand_id::unknown_0x04>;
    using unknown_0x05_command = command<subcommand_id::unknown_0x05>;
    using unknown_0x06_command = command<subcommand_id::unknown_0x06>;
    using unknown_0x07_command = command<subcommand_id::unknown_0x07>;
    using send_data_command    = command<subcommand_id::send_data,
                                         command_data { .command_payload_length = 0x14 },
                                         fill_payload::send_data>;
    using unknown_0x09_command = command<subcommand_id::unknown_0x09>;

    // a rumble frame, so callers never hand-assemble the output report.
    // @see https://github.com/ndeadly/switch2_controller_research/blob/master/hid_reports.md#output-report-0x02
    struct frame {
        array<byte, hid::rumble::block_size> left  = {};
        array<byte, hid::rumble::block_size> right = {};
    };

    constexpr auto make_output_report(const frame& f) noexcept -> array<byte, hid::output_report_size> {
        auto out = array<byte, hid::output_report_size> {};
        out[0]   = as<byte>(hid::output_report_id);

        auto left_view  = mutable_view_of(out).subspan(hid::rumble::left_offset, hid::rumble::block_size);
        auto right_view = mutable_view_of(out).subspan(hid::rumble::right_offset, hid::rumble::block_size);

        stdr::copy(f.left, stdr::begin(left_view));
        stdr::copy(f.right, stdr::begin(right_view));

        return out; // the trailing bytes stay zero: they are the reserved tail the descriptor forces us to send
    }
} // namespace lj::hid::vibration
