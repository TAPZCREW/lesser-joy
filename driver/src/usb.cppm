module;

#include "windows.hpp"

#include "usb.hpp"

#include <hidport.h>

#include <stormkit/core/try_expected.hpp>

#undef move

export module lesserjoy.usb;

import std;

import stormkit.core;

import lesserjoy.ntstatus;
import lesserjoy.log;

using namespace stormkit;
using namespace stormkit::literals;

namespace stdr = std::ranges;

export namespace lj::usb {
    struct Usb_device_context {
        WDFUSBDEVICE device = nullptr;

        USB_DEVICE_DESCRIPTOR descriptor = {};

        WDFMEMORY product_string = nullptr;

        WDFUSBINTERFACE interface = nullptr;
    };

    auto send_command(const Usb_device_context& ctx, array_view<const byte> buffer) -> Expected<ref<const Usb_device_context>>;
} // namespace lj::usb

module: private;

namespace lj::usb {
    auto send_command(const Usb_device_context& ctx, array_view<const byte> buffer) -> Expected<ref<const Usb_device_context>> {
        auto send_options = WDF_REQUEST_SEND_OPTIONS {};
        WDF_REQUEST_SEND_OPTIONS_INIT(&send_options, WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&send_options, WDF_REL_TIMEOUT_IN_SEC(3));

        auto control_setup_packet = WDF_USB_CONTROL_SETUP_PACKET {};
        WDF_USB_CONTROL_SETUP_PACKET_INIT_CLASS(&control_setup_packet,
                                                WDF_USB_BMREQUEST_DIRECTION::BmRequestHostToDevice,
                                                WDF_USB_BMREQUEST_RECIPIENT::BmRequestToInterface,
                                                0,
                                                0,
                                                0);

        auto buffer_descriptor = WDF_MEMORY_DESCRIPTOR {};
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&buffer_descriptor, std::bit_cast<PVOID>(stdr::data(buffer)), stdr::size(buffer));

        auto byte_transferred = ULONG { 0 };
        LoggedTry(lj::win_call(WdfUsbTargetDeviceSendControlTransferSynchronously,
                               ctx.device,
                               WDF_NO_HANDLE,
                               &send_options,
                               &control_setup_packet,
                               &buffer_descriptor,
                               &byte_transferred),
                  "Failed to send buffer to USB device!");

        dlog("USB: {} bytes sent", byte_transferred);

        Return { as_ref(ctx) };
    }
} // namespace lj::usb
