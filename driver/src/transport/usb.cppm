module;

#include "../wdk/wdfobject_macros.hpp"

#include <stormkit/core/try_expected.hpp>

#include "../contract_macro.hpp"

export module lesserjoy:transport.usb;

import std;

import stormkit.core;

import wdk.wdf;
import wdk.usb;
import wdk.ntstatus;

import :wdf;
import :common;

using namespace stormkit;
using namespace stormkit::literals;

namespace stdr = std::ranges;

export namespace lj {
    class device_context;

    namespace transport {
        class usb_context {
          public:
            static constexpr auto transport = transport_type::usb;

            struct joystick {
                u16 min_x = 500;
                u16 max_x = 3500;
                u16 min_y = 500;
                u16 max_y = 3500;
            };

            enum class joystick_type : u8 {
                left  = 0,
                right = 1,
            };

            enum class endpoint_type : u8 {
                hid     = 0,
                command = 1,
            };

            usb_context(const usb_context&)                    = delete;
            auto operator=(const usb_context&) -> usb_context& = delete;

            usb_context(usb_context&&) noexcept;
            auto operator=(usb_context&&) noexcept -> usb_context&;

            static auto create(const device_context&) -> system_result<ref_ptr<usb_context>>;

            auto device_entry() noexcept -> system_result<void>;
            auto device_exit() noexcept -> system_result<void>;

            auto send_data(array_view<const byte>, endpoint_type) const noexcept -> system_result<void>;
            auto send_data_sync(array_view<const byte>, endpoint_type) const noexcept -> system_result<void>;

            // like send_data but waits for the controller to accept the write, so a consumer such as
            // joy.cpl or hidapi gets a real failure instead of "the USB queue accepted the request".
            auto send_data_bounded(array_view<const byte>, endpoint_type, ulong) const noexcept -> system_result<void>;

            // the feature mask that gates rumble is set once at D0Entry; a reset by the HID stack silently
            // drops it, so the next output report has to re-arm it.
            auto mark_needs_rearm() noexcept -> void;
            auto needs_rearm() const noexcept -> bool;
            auto rearm_features() noexcept -> system_result<void>;

            auto get_data_sync() const noexcept -> system_result<hid::command_report_buffer>;

            template<typename Command, bool Validate = true, typename... Ts>
            auto send_command(Ts&&... args) noexcept -> system_result<void>;

            auto send_control_request(byte, byte, byte = 0x00_b, array_view<const byte> = {}) noexcept -> system_result<void>;

            auto write_report_to(wdk::WDFREQUEST) const noexcept -> system_result<void>;

            auto update_calibration(joystick_type, u16 x, u16 y) noexcept -> void;
            auto joystick_bounds(joystick_type) const noexcept -> joystick;

            auto has_product_string() const noexcept -> bool;
            auto product_string() const noexcept -> const string&;

            auto has_serial_string() const noexcept -> bool;
            auto serial_string() const noexcept -> const string&;

            auto forward_request(wdk::WDFREQUEST) const noexcept -> system_result<void>;

          private:
            struct endpoint {
                wdk::WDFUSBINTERFACE interface = nullptr;

                wdk::WDFUSBPIPE in_pipe  = nullptr;
                wdk::WDFUSBPIPE out_pipe = nullptr;
            };

            usb_context(const device_context&,
                        wdk::WDFUSBDEVICE,
                        endpoint&&,
                        endpoint&&,
                        wdk::USB_DEVICE_DESCRIPTOR&&,
                        std::optional<string>,
                        std::optional<string>) noexcept;

            // EvtWdfUsbReaderCompletionRoutine
            static auto pipe_reader_completion(wdk::WDFUSBPIPE, wdk::WDFMEMORY, usize, wdk::WDFCONTEXT) noexcept -> void;
            // EvtWdfRequestCompletionRoutine
            static auto request_completion_routine(wdk::WDFREQUEST,
                                                   wdk::WDFIOTARGET,
                                                   wdk::PWDF_REQUEST_COMPLETION_PARAMS,
                                                   wdk::WDFCONTEXT) noexcept -> void;

            ref_ptr<const device_context> device_ctx_;

            wdk::WDFUSBDEVICE device_ = nullptr;

            endpoint hid_;
            endpoint command_;

            wdk::USB_DEVICE_DESCRIPTOR descriptor_ = {};

            array<joystick, 2> joysticks_ = {};

            locked<hid::input_report_buffer> last_input_report_;

            // the HID stack polls for input reports continuously. answering a poll straight from the cache
            // without waiting turns that poll into a spin: the stack re-issues the ioctl as fast as it can,
            // and the host spends its time in the framework's request plumbing instead of waiting on the
            // device. the continuous reader signals this when a report lands, so a poll blocks on the device
            // and the CPU goes back to zero between reports.
            // mutable because waiting on and reading a cache is a logical-const operation; the hand-written
            // move operations deliberately do not move these, a wait state belongs to the live context.
            mutable std::mutex report_wait_mutex_;
            mutable std::condition_variable report_wait_;
            mutable bool report_pending_ = false;

            // WDF allows a single in-flight I/O per pipe and the ioqueue is dispatched parallel, so two
            // consumers issuing output reports can interleave a format+send pair in the middle of a report.
            // the pipe is wrapped because locked<T> asserts a non-null value for pointer payloads
            // (stormkit/core/parallelism/locked.cppm:294), which is wrong for a pipe that legitimately
            // starts null while the context is being built.
            struct write_guard {
                wdk::WDFUSBPIPE pipe = nullptr;
            };

            locked<write_guard> hid_out_pipe_    = {};
            locked<write_guard> command_out_pipe_ = {};

            // input report 0x09, offset 0x0B: 0x38 when feature bit 5 (rumble) is set, 0x30 otherwise.
            // @see https://github.com/ndeadly/switch2_controller_research/blob/master/hid_reports.md#input-report-0x09
            byte rumble_gate_ = 0x00_b;

            // set when the HID stack resets the device: the feature mask is gone and must be re-sent
            // before the next output report is meaningful.
            bool needs_rearm_ = false;

            std::optional<string> product_string_;
            std::optional<string> serial_string_;
        };

        using Pusb_context = usb_context*;
        STORMKIT_PUSH_WARNINGS
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
        WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(usb_context, get_usb_context)
        STORMKIT_POP_WARNINGS
    } // namespace transport
} // namespace lj

////////////////////////////////////////////////////////////////////
///                      IMPLEMENTATION                          ///
////////////////////////////////////////////////////////////////////

namespace lj::transport {
    inline usb_context::usb_context(const device_context&        ctx,
                                    wdk::WDFUSBDEVICE            device,
                                    endpoint&&                   hid,
                                    endpoint&&                   command,
                                    wdk::USB_DEVICE_DESCRIPTOR&& descriptor,
                                    std::optional<string>        product_string,
                                    std::optional<string>        serial_string) noexcept
        : device_ctx_ { ctx },
          device_ { device },
          hid_ { std::move(hid) },
          command_ { std::move(command) },
          hid_out_pipe_ { std::in_place, hid_.out_pipe },
          command_out_pipe_ { std::in_place, command_.out_pipe },
          descriptor_ { std::move(descriptor) },
          product_string_ { std::move(product_string) },
          serial_string_ { std::move(serial_string) } {
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    inline usb_context::usb_context(usb_context&& other) noexcept
        : device_ctx_ { std::move(other.device_ctx_) },
          device_ { std::exchange(other.device_, nullptr) },
          hid_ { std::move(other.hid_) },
          command_ { std::move(other.command_) },
          descriptor_ { std::exchange(other.descriptor_, {}) },
          joysticks_ { std::move(other.joysticks_) },
          hid_out_pipe_ { std::move(other.hid_out_pipe_) },
          command_out_pipe_ { std::move(other.command_out_pipe_) },
          rumble_gate_ { other.rumble_gate_ },
          needs_rearm_ { other.needs_rearm_ },
          product_string_ { std::exchange(other.product_string_, std::nullopt) },
          serial_string_ { std::exchange(other.serial_string_, std::nullopt) } {
        {
            auto lock = std::scoped_lock { last_input_report_.mutex(), other.last_input_report_.mutex() };

            last_input_report_.unsafe() = std::move(other.last_input_report_.unsafe());
        }
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    inline auto usb_context::operator=(usb_context&& other) noexcept -> usb_context& {
        if (this == &other) [[unlikely]]
            return *this;

        device_ctx_     = std::move(other.device_ctx_);
        device_         = std::exchange(other.device_, nullptr);
        hid_            = std::move(other.hid_);
        command_        = std::move(other.command_);
        descriptor_     = std::exchange(other.descriptor_, {});
        joysticks_      = std::move(other.joysticks_);
        hid_out_pipe_   = std::move(other.hid_out_pipe_);
        command_out_pipe_ = std::move(other.command_out_pipe_);
        rumble_gate_  = other.rumble_gate_;
        needs_rearm_  = other.needs_rearm_;
        product_string_ = std::exchange(other.product_string_, std::nullopt);
        serial_string_  = std::exchange(other.serial_string_, std::nullopt);
        {
            auto lock = std::scoped_lock { last_input_report_.mutex(), other.last_input_report_.mutex() };

            last_input_report_.unsafe() = std::move(other.last_input_report_.unsafe());
        }

        return *this;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    inline auto usb_context::update_calibration(joystick_type type, u16 x, u16 y) noexcept -> void {
        const auto id = as<usize>(type);
        WDK_EXPECTS_VOID(id <= 1);

        auto& joystick = joysticks_[id];
        joystick.min_x = std::min(x, joystick.min_x);
        joystick.max_x = std::max(x, joystick.max_x);
        joystick.min_y = std::min(y, joystick.min_y);
        joystick.max_y = std::max(y, joystick.max_y);
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    inline auto usb_context::joystick_bounds(joystick_type type) const noexcept -> joystick {
        const auto id = as<usize>(type);
        WDK_EXPECTS_RETURN(id <= 1, {});

        return joysticks_[id];
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<typename Command, bool Validate = true, typename... Ts>
    auto usb_context::send_command(Ts&&... args) noexcept -> system_result<void> {
        if constexpr (sizeof...(args) == 0) {
            static constexpr auto command = Command::template make_command<transport>(std::forward<Ts>(args)...);
            Try(send_data(command, endpoint_type::command));
        } else {
            const auto command = Command::template make_command<transport>(std::forward<Ts>(args)...);
            Try(send_data(command, endpoint_type::command));
        }

        if constexpr (Validate) {
            TryTo(report_, get_data_sync());
            const auto report = array_view<const byte, Command::report_length> { stdr::data(report_), Command::report_length };

            if (not Command::template validate_report<transport>(report)) {
                const auto got      = array_view<const u8> { reinterpret_cast<const u8*>(stdr::data(report)),
                                                             stdr::size(Command::template report_header<transport>) };
                const auto expected = array_view<const u8> {
                    reinterpret_cast<const u8*>(stdr::data(Command::template report_header<transport>)),
                    stdr::size(Command::template report_header<transport>)
                };
                dlog("Report header bytes mismatch! got: {::#x}, expected: {::#x}!", got, expected);
                return std::unexpected<system_error2::nt_code> { wdk::STATUS_UNSUCCESSFUL };
            }
        }

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    auto usb_context::mark_needs_rearm() noexcept -> void {
        needs_rearm_ = true;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    auto usb_context::needs_rearm() const noexcept -> bool {
        return needs_rearm_;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    auto usb_context::has_product_string() const noexcept -> bool {
        return product_string_ != std::nullopt;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    auto usb_context::product_string() const noexcept -> const string& {
        // WDK_EXPECTS_RETURN(has_product_string(), [] static noexcept {
        //     static const auto str = string { "" };
        //     return str;
        // }());

        return *product_string_;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    auto usb_context::has_serial_string() const noexcept -> bool {
        return serial_string_ != std::nullopt;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    STORMKIT_FORCE_INLINE
    auto usb_context::serial_string() const noexcept -> const string& {
        // WDK_EXPECTS_RETURN(has_serial_string(), [] static noexcept {
        //     static const auto str = string { "" };
        //     return str;
        // }());

        return *serial_string_;
    }
} // namespace lj::transport
