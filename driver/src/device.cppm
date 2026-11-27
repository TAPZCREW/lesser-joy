module;

#include "wdk/wdfobject_macros.hpp"

#include <stormkit/core/platform_macro.hpp>

#include <stormkit/core/try_expected.hpp>

export module lesserjoy:device;

import std;

import stormkit.core;
import stormkit.platform.win32;

import wdk.wdf;
import wdk.ntstatus;
import wdk.hid;

import :constants;
import :common;
import :transport.usb;

using namespace stormkit;

namespace stdr = std::ranges;

export namespace lj {
    class device_context {
      public:
        ~device_context() noexcept = default;

        device_context(const device_context&)                    = delete;
        auto operator=(const device_context&) -> device_context& = delete;

        device_context(device_context&&) noexcept                    = delete;
        auto operator=(device_context&&) noexcept -> device_context& = delete;

        // EvtDriverDeviceAdd
        static auto create(wdk::WDFDRIVER, wdk::PWDFDEVICE_INIT) -> wdk::NTSTATUS;

        inline auto device() const noexcept -> wdk::WDFDEVICE {
            expects(device_ != nullptr);
            return device_;
        }

        // inline auto is_usb() const noexcept -> bool { return is<transport::usb_context>(transport_); }
        inline auto is_usb() const noexcept -> bool {
            return std::holds_alternative<ref_ptr<transport::usb_context>>(transport_);
        }

        // inline auto usb_ctx() noexcept -> transport::usb_context& { return as<transport::usb_context>(transport_); }
        template<typename Self>
        inline auto usb_ctx(this Self& self) noexcept -> meta::forward_const_to<Self, transport::usb_context>& {
            return *std::get<ref_ptr<transport::usb_context>>(self.transport_);
        }

        // inline auto is_ble() const noexcept -> bool { return is<ble::context>(transport_); }
        inline auto is_ble() const noexcept -> bool { return std::holds_alternative<ref_ptr<ble::context>>(transport_); }

        // inline auto ble_ctx() noexcept -> ble::context& { return as<ble::context>(transport_); }
        template<typename Self>
        inline auto ble_ctx(this Self& self) noexcept -> meta::forward_const_to<Self, ble::context>& {
            return *std::get<ref_ptr<ble::context>>(self.transport_);
        }

        inline auto vendor_id() const noexcept -> u16 { return vendor_id_; }

        inline auto product_id() const noexcept -> u16 { return product_id_; }

        inline auto product_string() const noexcept -> const string& { return product_string_; }

        inline auto manufacturer_string() const noexcept -> const string& { return manufacturer_string_; }

        inline auto serial_string() const noexcept -> const string& { return serial_string_; }

        // answers for the "capability" iocontrols joy.cpl and the HID-games stack issue
        // (IOCTL_HID_GET/SET_DRIVER_CONFIG, ..._POLL_FREQUENCY_MSEC, ..._NUM_DEVICE_INPUT_BUFFERS,
        // ..._S0_IDLE_TIMEOUT, ..._SECURE_READ, ..._WAKE_ON_SX). they are stored and echoed instead of
        // refused so a consumer that configures the device ends up believing something true about it.
        inline auto poll_frequency_msec() const noexcept -> u32 { return poll_frequency_msec_; }

        inline auto num_input_buffers() const noexcept -> u32 { return num_input_buffers_; }

        inline auto ring_buffer_size() const noexcept -> u32 { return ring_buffer_size_; }

        inline auto set_poll_frequency_msec(u32 value) noexcept -> void { poll_frequency_msec_ = value; }

        inline auto set_num_input_buffers(u32 value) noexcept -> void { num_input_buffers_ = value; }

        inline auto set_ring_buffer_size(u32 value) noexcept -> void { ring_buffer_size_ = value; }

        inline auto set_s0_idle_timeout(u32 value) noexcept -> void { s0_idle_timeout_ = value; }

        inline auto set_secure_read(bool value) noexcept -> void { secure_read_ = value; }

        inline auto set_wake_on_sx(bool value) noexcept -> void { wake_on_sx_ = value; }

        inline auto hid_descriptor() const noexcept -> const wdk::HID_DESCRIPTOR& { return hid_descriptor_; }

        inline auto hid_attributes() const noexcept -> const wdk::HID_DEVICE_ATTRIBUTES& { return hid_attributes_; }

        inline auto report_descriptor() const noexcept -> const hid::report_descriptor& { return report_descriptor_; }

      private:
        // constructor is private because context types should be constructed with create static method
        inline explicit device_context(wdk::WDFDEVICE device_) noexcept : device_ { device_ } {}

        auto prepare_hardware() noexcept -> system_result<void>;
        auto release_hardware() noexcept -> system_result<void>;
        auto device_entry() noexcept -> system_result<void>;
        auto device_exit() noexcept -> system_result<void>;

        // EvtDevicePrepareHardware
        static auto prepare_hardware(wdk::WDFDEVICE, wdk::WDFCMRESLIST, wdk::WDFCMRESLIST) noexcept -> wdk::NTSTATUS;
        // EvtDeviceReleaseHardware
        static auto release_hardware(wdk::WDFDEVICE, wdk::WDFCMRESLIST) noexcept -> wdk::NTSTATUS;
        // EvtDeviceD0Entry
        static auto device_entry(wdk::WDFDEVICE, wdk::WDF_POWER_DEVICE_STATE) noexcept -> wdk::NTSTATUS;
        // EvtDeviceD0Exit
        static auto device_exit(wdk::WDFDEVICE, wdk::WDF_POWER_DEVICE_STATE) noexcept -> wdk::NTSTATUS;

        u16 vendor_id_  = 0;
        u16 product_id_ = 0;

        wdk::WDFDEVICE device_;

        string product_string_      = {};
        string serial_string_       = {};
        string manufacturer_string_ = {};

        u32  poll_frequency_msec_ = 4; // the report descriptor declares bInterval 4
        u32  num_input_buffers_   = 1; // a single continuous reader (transport/usb.cpp)
        u32  ring_buffer_size_    = 2;
        u32  s0_idle_timeout_     = 0;
        bool secure_read_         = false;
        bool wake_on_sx_          = false;

        wdk::HID_DESCRIPTOR        hid_descriptor_    = {};
        hid::report_descriptor     report_descriptor_ = {};
        wdk::HID_DEVICE_ATTRIBUTES hid_attributes_    = {};

        std::variant<std::monostate, ref_ptr<transport::usb_context>, ref_ptr<ble::context>> transport_ = {};
    };

    using Pdevice_context = device_context*;

    class queue_context {
      public:
        ~queue_context() noexcept = default;

        queue_context(const queue_context&)                    = delete;
        auto operator=(const queue_context&) -> queue_context& = delete;

        queue_context(queue_context&&) noexcept                    = delete;
        auto operator=(queue_context&&) noexcept -> queue_context& = delete;

        static auto create(wdk::WDFDEVICE) noexcept -> wdk::NTSTATUS;

        inline auto device_ctx() noexcept -> device_context& { return *device_ctx_; }

        inline auto queue() noexcept -> wdk::WDFQUEUE {
            expects(queue_ != nullptr);
            return queue_;
        }

      private:
        // constructor is private because context types should be constructed with create static method
        inline explicit queue_context(device_context* device_ctx_, wdk::WDFQUEUE queue_) noexcept
            : device_ctx_ { device_ctx_ }, queue_ { queue_ } {}

        struct tag {};

        auto io_control(wdk::WDFQUEUE, wdk::WDFREQUEST, usize, usize, u32, tag) noexcept -> void;

        // EvtIoDeviceControl
        static auto io_control(wdk::WDFQUEUE, wdk::WDFREQUEST, usize, usize, ULONG) noexcept -> void;

        ref_ptr<device_context> device_ctx_;
        wdk::WDFQUEUE                 queue_ = nullptr;
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
} // namespace lj
