module;

#include "windows.hpp"

#include "usb.hpp"

#include <Hidclass.h>

#include <stormkit/core/try_expected.hpp>

export module lesserjoy.device;

import std;

import stormkit.core;

import lesserjoy.constants;
import lesserjoy.common;

using namespace stormkit;

namespace stdr = std::ranges;

export namespace lj {
    class device_context {
      public:
        device_context(const device_context&)                    = delete;
        auto operator=(const device_context&) -> device_context& = delete;

        device_context(device_context&&) noexcept                    = delete;
        auto operator=(device_context&&) noexcept -> device_context& = delete;

        // EvtDriverDeviceAdd
        static auto create(_In_ WDFDRIVER, _Inout_ PWDFDEVICE_INIT) -> NTSTATUS;

        inline auto device() const noexcept -> WDFDEVICE {
            expects(device_ != nullptr);
            return device_;
        }

        // inline auto is_usb() const noexcept -> bool { return is<usb::context>(transport_); }
        inline auto is_usb() const noexcept -> bool { return std::holds_alternative<usb::context>(transport_); }

        // inline auto usb_ctx() noexcept -> usb::context& { return as<usb::context>(transport_); }
        template<typename Self>
        inline auto usb_ctx(this Self& self) noexcept -> meta::forward_const_to<Self, usb::context>& {
            return std::get<usb::context>(self.transport_);
        }

        // inline auto is_ble() const noexcept -> bool { return is<ble::context>(transport_); }
        inline auto is_ble() const noexcept -> bool { return std::holds_alternative<ble::context>(transport_); }

        // inline auto ble_ctx() noexcept -> ble::context& { return as<ble::context>(transport_); }
        template<typename Self>
        inline auto ble_ctx(this Self& self) noexcept -> meta::forward_const_to<Self, ble::context>& {
            return std::get<ble::context>(self.transport_);
        }

        inline auto vendor_id() const noexcept -> u16 { return vendor_id_; }

        inline auto product_id() const noexcept -> u16 { return product_id_; }

        inline auto product_string() const noexcept -> const string& { return product_string_; }

        inline auto serial_string() const noexcept -> const string& { return serial_string_; }

        inline auto hid_descriptor() const noexcept -> const HID_DESCRIPTOR& { return hid_descriptor_; }

        inline auto hid_attributes() const noexcept -> const HID_DEVICE_ATTRIBUTES& { return hid_attributes_; }

        inline auto report_descriptor() const noexcept -> const hid::report_descriptor& { return report_descriptor_; }

        inline auto output_report() const noexcept -> const hid::output_report& { return output_report_; }

      private:
        // constructor and destructor are private because context types should be constructed with create static method and
        // let WDF handle destruction with destroy callback
        inline explicit device_context(WDFDEVICE device_) noexcept : device_ { device_ } {}

        ~device_context() noexcept = default;

        auto prepare_hardware() noexcept -> system_result<void>;
        auto release_hardware() noexcept -> system_result<void>;
        auto device_entry() noexcept -> system_result<void>;
        auto device_exit() noexcept -> system_result<void>;

        // EvtDestroyCallback
        static auto destroy(_In_ WDFOBJECT) noexcept -> void;
        // EvtDevicePrepareHardware
        static auto prepare_hardware(_In_ WDFDEVICE, _In_ WDFCMRESLIST, _In_ WDFCMRESLIST) noexcept -> NTSTATUS;
        // EvtDeviceReleaseHardware
        static auto release_hardware(_In_ WDFDEVICE, _In_ WDFCMRESLIST) noexcept -> NTSTATUS;
        // EvtDeviceD0Entry
        static auto device_entry(_In_ WDFDEVICE, _In_ WDF_POWER_DEVICE_STATE) noexcept -> NTSTATUS;
        // EvtDeviceD0Exit
        static auto device_exit(_In_ WDFDEVICE, WDF_POWER_DEVICE_STATE) noexcept -> NTSTATUS;

        u16 vendor_id_  = 0;
        u16 product_id_ = 0;

        WDFDEVICE device_;

        string product_string_ = {};
        string serial_string_  = {};

        HID_DESCRIPTOR         hid_descriptor_    = {};
        hid::report_descriptor report_descriptor_ = {};
        HID_DEVICE_ATTRIBUTES  hid_attributes_    = {};

        hid::output_report output_report_ = {};

        std::variant<std::monostate, usb::context, ble::context> transport_ = {};
    };

    using Pdevice_context = device_context*;

    class queue_context {
      public:
        queue_context(const queue_context&)                    = delete;
        auto operator=(const queue_context&) -> queue_context& = delete;

        queue_context(queue_context&&) noexcept                    = delete;
        auto operator=(queue_context&&) noexcept -> queue_context& = delete;

        static auto create(_In_ WDFDEVICE) noexcept -> NTSTATUS;

        inline auto device_ctx() noexcept -> const device_context& { return *device_ctx_; }

        inline auto queue() noexcept -> WDFQUEUE {
            expects(queue_ != nullptr);
            return queue_;
        }

      private:
        // constructor and destructor are private because context types should be constructed with create static method and
        // let WDF handle destruction with destroy callback
        inline explicit queue_context(const device_context* device_ctx_, WDFQUEUE queue_) noexcept
            : device_ctx_ { device_ctx_ }, queue_ { queue_ } {}

        ~queue_context() noexcept = default;

        struct tag {};

        auto io_control(WDFQUEUE, WDFREQUEST, usize, usize, u32, tag) const noexcept -> void;

        // EvtDestroyCallback
        static auto destroy(_In_ WDFOBJECT) noexcept -> void;
        // EvtIoDeviceControl
        static auto io_control(_In_ WDFQUEUE, _In_ WDFREQUEST, _In_ usize, _In_ usize, _In_ ULONG) noexcept -> void;

        ref_ptr<const device_context> device_ctx_;
        WDFQUEUE                      queue_ = nullptr;
    };

    using Pqueue_context = queue_context*;

    STORMKIT_PUSH_WARNINGS

#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(device_context, get_device_context)
    STORMKIT_POP_WARNINGS

    STORMKIT_PUSH_WARNINGS
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(queue_context, get_queue_context)
    STORMKIT_POP_WARNINGS

    EVT_WDF_OBJECT_CONTEXT_CLEANUP event_device_cleanup;
} // namespace lj
