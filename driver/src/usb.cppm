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
        WDFUSBDEVICE device;

        USB_DEVICE_DESCRIPTOR descriptor;

        WDFMEMORY product_string;

        WDFUSBINTERFACE interface;
    };

    auto send_command(Usb_device_context& ctx, array_view<const byte> buffer) -> Expected<void>;
} // namespace lj::usb

module: private;

namespace lj::usb {
    auto send_command(Usb_device_context& ctx, array_view<const byte> buffer) -> Expected<void> {
        auto send_options = WDF_REQUEST_SEND_OPTIONS {};
        WDF_REQUEST_SEND_OPTIONS_INIT(&send_options, WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&send_options, WDF_REL_TIMEOUT_IN_SEC(3));

        auto control_setup_packet = WDF_USB_CONTROL_SETUP_PACKET {};
        // WDF_USB_CONTROL_SETUP_PACKET_INIT_CLASS(&control_setup_packet,
        //                                         WDF_USB_BMREQUEST_DIRECTION::BmRequestHostToDevice,
        //                                         WDF_USB_BMREQUEST_TYPE::BmRequestClass,
        //                                         0x09,

        //    );

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

        ilog("USB: {} bytes sent", byte_transferred);

        Return {};
    }
} // namespace lj::usb
