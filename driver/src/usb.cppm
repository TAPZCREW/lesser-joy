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
import lesserjoy.constants;
import lesserjoy.log;

using namespace stormkit;
using namespace stormkit::literals;

namespace stdr = std::ranges;

export namespace lj::usb {
    struct Usb_device_context {
        WDFUSBDEVICE device   = nullptr;
        WDFUSBPIPE   in_pipe  = nullptr;
        WDFUSBPIPE   out_pipe = nullptr;

        USB_DEVICE_DESCRIPTOR descriptor = {};

        WDFMEMORY product_string = nullptr;

        WDFUSBINTERFACE interface = nullptr;
    };

    auto send_command_async(const Usb_device_context& ctx, array_view<const byte> payload) -> Expected<void>;
    auto send_command_sync(const Usb_device_context& ctx, array_view<const byte> payload) -> Expected<usize>;

    auto receive_response_sync(const Usb_device_context& ctx, array_view<byte> to) -> Expected<usize>;
} // namespace lj::usb

namespace lj::usb {
} // namespace lj::usb

module: private;

namespace lj::usb {
    EVT_WDF_REQUEST_COMPLETION_ROUTINE event_request_completion_routine;

    auto send_command_async(const Usb_device_context& ctx, array_view<const byte> payload) -> Expected<void> {
        ilog("Sending {::#x}", array_view<const u8> { std::bit_cast<const u8*>(stdr::data(payload)), stdr::size(payload) });

        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        auto target  = WdfUsbTargetDeviceGetIoTarget(ctx.device);
        auto request = WDFREQUEST {};
        LoggedTry(lj::win_call(WdfRequestCreate, &attributes, target, &request), "WdfRequestCreate failed!");

        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = request;

        auto memory           = WDFMEMORY {};
        auto write_buffer_ptr = PVOID { nullptr };
        LoggedTry(lj::win_call(WdfMemoryCreate,
                               &attributes,
                               NonPagedPoolNx,
                               POOL_TAG,
                               stdr::size(payload),
                               &memory,
                               &write_buffer_ptr),
                  "WdfMemoryCreate failed!");

        auto write_buffer = array_view<byte> { std::bit_cast<byte*>(write_buffer_ptr), stdr::size(payload) };
        stdr::copy(payload, stdr::begin(write_buffer));

        LoggedTry(lj::win_call(WdfUsbTargetPipeFormatRequestForWrite, ctx.out_pipe, request, memory, nullptr),
                  "WdfUsbTargetPipeFormatRequestForWrite failed!");

        WdfRequestSetCompletionRoutine(request, event_request_completion_routine, nullptr);

        if (WdfRequestSend(request, target, nullptr) == FALSE) {
            const auto status = WdfRequestGetStatus(request);
            elog("WdfRequestSend failed!\n    reason: {:#x}", narrow<cpp::ULong>(status));
            return std::unexpected<system_error2::nt_code> { std::in_place, status };
        }

        Return {};
    }

    auto send_command_sync(const Usb_device_context& ctx, array_view<const byte> payload) -> Expected<usize> {
        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        auto memory_descriptor = WDF_MEMORY_DESCRIPTOR {};
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memory_descriptor, std::bit_cast<PVOID>(stdr::data(payload)), stdr::size(payload));

        auto written = cpp::ULong { 0 };
        LoggedTry(lj::win_call(WdfUsbTargetPipeWriteSynchronously, ctx.out_pipe, nullptr, nullptr, &memory_descriptor, &written),
                  "WdfUsbTargetPipeWriteSynchronously failed!");

        Return { as<usize>(written) };
    }

    auto receive_response_sync(const Usb_device_context& ctx, array_view<byte> to) -> Expected<usize> {
        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        auto memory_descriptor = WDF_MEMORY_DESCRIPTOR {};
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memory_descriptor, std::bit_cast<PVOID>(stdr::data(to)), stdr::size(to));

        auto readed = cpp::ULong { 0 };
        LoggedTry(lj::win_call(WdfUsbTargetPipeReadSynchronously, ctx.in_pipe, nullptr, nullptr, &memory_descriptor, &readed),
                  "WdfUsbTargetPipeReadSynchronously failed!");

        return { as<usize>(readed) };
    }

    _Use_decl_annotations_ auto event_request_completion_routine(WDFREQUEST,
                                                                 WDFIOTARGET,
                                                                 PWDF_REQUEST_COMPLETION_PARAMS,
                                                                 WDFCONTEXT) -> void {
        ilog("HELLO");
    }
} // namespace lj::usb
