module;

#include "windows.hpp"

#include "usb.hpp"

#include <Hidclass.h>

#include <stormkit/core/try_expected.hpp>

module lesserjoy.device;

import lesserjoy.wdf;
import lesserjoy.log;
import lesserjoy.hid;
import lesserjoy.usb;

using namespace stormkit;
using namespace stormkit::literals;

namespace lj {
    EVT_WDF_DEVICE_PREPARE_HARDWARE event_prepare_hardware;
    EVT_WDF_DEVICE_D0_ENTRY         event_device_entry;
    EVT_WDF_DEVICE_D0_EXIT          event_device_exit;

    EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL event_io_device_control;

    ////////////////////////////////////////
    ////////////////////////////////////////
    _Use_decl_annotations_ auto event_device_add(_In_ WDFDRIVER, _Inout_ PWDFDEVICE_INIT device_init) -> NTSTATUS {
        WdfFdoInitSetFilter(device_init);

        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, Device_context);
        attributes.EvtCleanupCallback = nullptr;

        auto power_callbacks = WDF_PNPPOWER_EVENT_CALLBACKS {};
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&power_callbacks);
        power_callbacks.EvtDevicePrepareHardware = event_prepare_hardware;
        power_callbacks.EvtDeviceD0Entry         = event_device_entry;
        power_callbacks.EvtDeviceD0Exit          = event_device_exit;

        WdfDeviceInitSetPnpPowerEventCallbacks(device_init, &power_callbacks);

        auto device = WDFDEVICE {};
        LoggedTryOr(lj::win_call(WdfDeviceCreate, &device_init, &attributes, &device),
                    monadic::unwrap(),
                    "Failed to create device!");

        auto ctx = GetDeviceContext(device);
        *ctx     = Device_context {};

        constexpr auto DEFAULT_CONTROLLER = CONTROLLERS_TYPE.at("pro_controller");

        ctx->device                   = device;
        ctx->hid_attributes.Size      = sizeof(HID_DEVICE_ATTRIBUTES);
        ctx->hid_attributes.VendorID  = DEFAULT_CONTROLLER.vid;
        ctx->hid_attributes.ProductID = DEFAULT_CONTROLLER.pid;
        ctx->hid_descriptor           = hid::DEFAULT_DESCRIPTOR;
        ctx->report_descriptor        = hid::DEFAULT_REPORT_DESCRIPTOR;

        stdr::copy(hid::DEFAULT_OUTPUT_REPORT, stdr::begin(ctx->output_report));

        {
            auto queue_config = WDF_IO_QUEUE_CONFIG {};
            WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue_config, WdfIoQueueDispatchParallel);
            queue_config.PowerManaged       = WdfTrue;
            queue_config.EvtIoDeviceControl = event_io_device_control;

            auto queue_attributes = WDF_OBJECT_ATTRIBUTES {};
            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queue_attributes, Queue_context);

            LoggedTryOr(lj::win_call(WdfIoQueueCreate, device, &queue_config, &queue_attributes, &ctx->default_queue),
                        monadic::unwrap(),
                        "Failed to create io queue!");
            lj::dlog("Io queue successfully created!");

            auto queue_ctx        = GetQueueContext(ctx->default_queue);
            queue_ctx->queue      = ctx->default_queue;
            queue_ctx->device_ctx = ctx;
        }

        // {
        //     auto queue_config = WDF_IO_QUEUE_CONFIG {};
        //     WDF_IO_QUEUE_CONFIG_INIT(&queue_config, WdfIoQueueDispatchManual);

        //    auto queue_attributes = WDF_OBJECT_ATTRIBUTES {};
        //    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queue_attributes, Queue_context);

        //    LoggedTryOr(lj::win_call(WdfIoQueueCreate, device, &queue_config, &queue_attributes, &ctx->manual_queue),
        //                monadic::unwrap(),
        //                "Failed to create io queue!");
        //    lj::dlog("Manual io queue successfully created!");

        //    auto queue_ctx        = GetQueueContext(ctx->manual_queue);
        //    queue_ctx->queue      = ctx->manual_queue;
        //    queue_ctx->device_ctx = ctx;
        // }

        // LoggedTryOr(lj::win_call(WdfDeviceCreateDeviceInterface, device, &DEVICE_INTERFACE_GUID.fmtid, nullptr),
        //             monadic::unwrap(),
        //             "Failed to expose device interface!");
        LoggedTryOr(lj::win_call(WdfDeviceCreateDeviceInterface, device, &DEV_INTERFACE_HID_GUID, nullptr),
                    monadic::unwrap(),
                    "Failed to expose device interface!");

        return STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    _Use_decl_annotations_ auto event_device_cleanup(_In_ WDFOBJECT device) -> void {
        lj::dlog("Cleanup up device {}", std::bit_cast<uptr>(device));

        // EventWriteUnloadObject(device);
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    _Use_decl_annotations_ auto event_io_device_control(_In_ WDFQUEUE   queue,
                                                        _In_ WDFREQUEST request,
                                                        _In_ usize      output_buffer_size,
                                                        _In_ usize      input_buffer_size,
                                                        _In_ ULONG      io_control_code) -> void {
        auto queue_ctx  = GetQueueContext(queue);
        auto device_ctx = queue_ctx->device_ctx;

        auto request_completed = true;
        auto status            = NTSTATUS { STATUS_SUCCESS };

        struct AtExit {
            AtExit(const bool& completed_, const WDFREQUEST& request_, const NTSTATUS& status_) noexcept
                : completed { completed_ }, request { request_ }, status { status_ } {}

            ~AtExit() noexcept {
                if (completed) WdfRequestComplete(request, status);
            }

            const bool&       completed;
            const WDFREQUEST& request;
            const NTSTATUS&   status;
        } _ { request_completed, request, status };

        const auto update_status = [&status](auto&& error) noexcept { status = error.value(); };

        switch (io_control_code) {
            case IOCTL_HID_GET_DEVICE_DESCRIPTOR: {
                LoggedTryOr(hid::ioctl::get_device_descriptor(request, device_ctx->hid_descriptor),
                            update_status,
                            "IOCTL Failed to get device descriptor!");
            } break;

            case IOCTL_HID_GET_DEVICE_ATTRIBUTES: {
                LoggedTryOr(hid::ioctl::get_device_attributes(request, device_ctx->hid_attributes),
                            update_status,
                            "IOCTL Failed to get device attributes!");
            } break;

            case IOCTL_HID_GET_REPORT_DESCRIPTOR: {
                LoggedTryOr(hid::ioctl::get_report_descriptor(request, device_ctx->report_descriptor),
                            update_status,
                            "IOCTL Failed to get report descriptor!");
            } break;

            case IOCTL_HID_READ_REPORT: {
                LoggedTryOr(hid::ioctl::read_report(request, as<usb::Context>(device_ctx->transport)),
                            update_status,
                            "IOCTL Failed to read report!");
            } break;

            case IOCTL_HID_WRITE_REPORT: {
                LoggedTryOr(hid::ioctl::write_report(request, device_ctx->output_report),
                            update_status,
                            "IOCTL Failed to write report!");
            } break;

            case IOCTL_HID_GET_STRING: {
                LoggedTryOr(hid::ioctl::get_string(request, device_ctx->product_string, device_ctx->product_string),
                            update_status,
                            "IOCTL Failed to get string!");
            } break;

            case IOCTL_HID_GET_INDEXED_STRING: {
                LoggedTryOr(hid::ioctl::get_indexed_string(request, device_ctx->serial_string),
                            update_status,
                            "IOCTL Failed to get indexed string!");
            } break;

            case IOCTL_HID_DEVICERESET_NOTIFICATION: {
                wlog("IOCTL_HID_DEVICERESET_NOTIFICATION not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            case IOCTL_HID_ACTIVATE_DEVICE: {
            } break;

            case IOCTL_HID_DEACTIVATE_DEVICE: {
            } break;

            case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST: {
            } break;

            case IOCTL_UMDF_GET_PHYSICAL_DESCRIPTOR: {
                wlog("IOCTL_UMDF_GET_PHYSICAL_DESCRIPTOR not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            case IOCTL_UMDF_HID_GET_FEATURE: {
                wlog("IOCTL_UMDF_HID_GET_FEATURE not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            case IOCTL_UMDF_HID_GET_INPUT_REPORT: {
                wlog("IOCTL_UMDF_HID_GET_INPUT_REPORT not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            case IOCTL_UMDF_HID_SET_FEATURE: {
                wlog("IOCTL_UMDF_HID_SET_FEATURE not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            case IOCTL_UMDF_HID_SET_OUTPUT_REPORT: {
                wlog("IOCTL_UMDF_HID_SET_OUTPUT_REPORT not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            default: {
                status = STATUS_NOT_IMPLEMENTED;
            } break;
        }
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    _Use_decl_annotations_ auto event_prepare_hardware(WDFDEVICE device, WDFCMRESLIST, WDFCMRESLIST) -> NTSTATUS {
        auto ctx = GetDeviceContext(device);

        LoggedTryOr(usb::init_context(*ctx, device), monadic::unwrap(), "Failed to initialize USB context!");
        lj::ilog("{} attached (USB), PID: {:#x}, VID: {:#x}", ctx->product_string, ctx->vendor_id, ctx->product_id);

        return STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    _Use_decl_annotations_ auto event_device_entry(WDFDEVICE device, WDF_POWER_DEVICE_STATE) -> NTSTATUS {
        auto& ctx = *GetDeviceContext(device);
        usb::event_device_entry(ctx);

        return STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    _Use_decl_annotations_ auto event_device_exit(WDFDEVICE device, WDF_POWER_DEVICE_STATE) -> NTSTATUS {
        auto& ctx = *GetDeviceContext(device);
        usb::event_device_exit(ctx);

        return STATUS_SUCCESS;
    }
} // namespace lj
