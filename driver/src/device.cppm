module;

#include "windows.hpp"

#include "usb.hpp"

export module lesserjoy.device;

import std;

import stormkit.core;

import lesserjoy.log;
import lesserjoy.constants;
import lesserjoy.hid;

using namespace stormkit;

namespace stdr = std::ranges;

export namespace lj {
    struct Usb_device_context {
        WDFUSBDEVICE device;

        USB_DEVICE_DESCRIPTOR descriptor;

        WDFMEMORY product_string;
    };

    struct Device_context {
        WDFDEVICE device;

        WDFQUEUE default_queue;
        WDFQUEUE manual_queue;

        HID_DESCRIPTOR         hid_descriptor;
        hid::Report_descriptor report_descriptor;
        HID_DEVICE_ATTRIBUTES  hid_attributes;

        WDFMEMORY output_report_memory;

        Usb_device_context usb;

        u16 vendor_id;
        u16 product_id;
    };

    using PDevice_context = Device_context*;
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Device_context, GetDeviceContext)

    struct Queue_context {
        WDFQUEUE queue;

        Device_context* device_ctx;
    };

    using PQueue_context = Queue_context*;
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Queue_context, GetQueueContext)

    EVT_WDF_DRIVER_DEVICE_ADD      event_device_add;
    EVT_WDF_OBJECT_CONTEXT_CLEANUP event_device_cleanup;
} // namespace lj

module: private;

namespace lj {
    EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL event_io_device_control;
    EVT_WDF_DEVICE_PREPARE_HARDWARE    event_prepare_hardware;

    auto init_device_context(WDFDEVICE device) -> NTSTATUS {
        auto status = NTSTATUS { STATUS_SUCCESS };

        auto ctx = GetDeviceContext(device);
        std::memset(ctx, 0, sizeof(Device_context));
        ctx->device = device;

        constexpr auto bus_desc_key = DEVPROPKEY {
            .fmtid = { 0x540b947e, 0x8b40, 0x45bc, { 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2 } },
            .pid   = 4
        };

        auto property_data = WDF_DEVICE_PROPERTY_DATA {};
        WDF_DEVICE_PROPERTY_DATA_INIT(&property_data, &bus_desc_key);
        property_data.Flags |= PLUGPLAY_PROPERTY_PERSISTENT;
        property_data.Lcid = LOCALE_NEUTRAL;

        const auto& default_controller = CONTROLLERS_TYPE.at("pro_controller");

        WdfDeviceAssignProperty(device,
                                &property_data,
                                DEVPROP_TYPE_STRING,
                                stdr::size(default_controller.product_string),
                                std::bit_cast<void*>(stdr::data(default_controller.product_string)));

        ctx->hid_attributes           = zeroed<HID_DEVICE_ATTRIBUTES>();
        ctx->hid_attributes.Size      = sizeof(HID_DEVICE_ATTRIBUTES);
        ctx->hid_attributes.VendorID  = default_controller.vid;
        ctx->hid_attributes.ProductID = default_controller.pid;
        ctx->hid_descriptor           = hid::DEFAULT_DESCRIPTOR;
        ctx->report_descriptor        = hid::DEFAULT_REPORT_DESCRIPTOR;

        auto report_buffer = PUCHAR { nullptr };
        auto attributes    = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = device;

        status = WdfMemoryCreate(&attributes,
                                 NonPagedPoolNx,
                                 POOL_TAG,
                                 sizeof(hid::DEFAULT_OUTPUT_REPORT),
                                 &ctx->output_report_memory,
                                 std::bit_cast<PVOID*>(&report_buffer));
        if (not NT_SUCCESS(status)) {
            lj::elog("Failed to allocate hid report buffer memory! status: {:#x}", static_cast<u32>(status));
            return status;
        }

        std::memcpy(report_buffer, stdr::data(hid::DEFAULT_OUTPUT_REPORT), sizeof(hid::DEFAULT_OUTPUT_REPORT));

        return status;
    }

#pragma code_seg("PAGED")

    auto event_device_add(_In_ WDFDRIVER, _Inout_ PWDFDEVICE_INIT device_init) noexcept -> NTSTATUS {
        PAGED_CODE();

        lj::dlog("Event device add!");

        WdfFdoInitSetFilter(device_init);

        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, Device_context);
        attributes.EvtCleanupCallback = nullptr;

        auto power_callbacks = WDF_PNPPOWER_EVENT_CALLBACKS {};
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&power_callbacks);
        power_callbacks.EvtDevicePrepareHardware = event_prepare_hardware;

        WdfDeviceInitSetPnpPowerEventCallbacks(device_init, &power_callbacks);

        auto device = WDFDEVICE {};
        auto status = WdfDeviceCreate(&device_init, &attributes, &device);

        if (not NT_SUCCESS(status)) {
            lj::elog("Failed to create device! status: {:#x}", static_cast<u32>(status));
            return status;
        }

        status = init_device_context(device);
        if (not NT_SUCCESS(status)) {
            lj::elog("Failed to device context! status: {:#x}", static_cast<u32>(status));
            return status;
        }

        auto        ctx                = GetDeviceContext(device);
        const auto& default_controller = CONTROLLERS_TYPE.at("pro_controller");

        {
            auto queue_config = WDF_IO_QUEUE_CONFIG {};
            WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue_config, WdfIoQueueDispatchParallel);
            queue_config.EvtIoDeviceControl = event_io_device_control;

            auto queue_attributes = WDF_OBJECT_ATTRIBUTES {};
            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queue_attributes, Queue_context);

            status = WdfIoQueueCreate(device, &queue_config, &queue_attributes, &ctx->default_queue);
            if (not NT_SUCCESS(status)) {
                lj::elog("Failed to create io queue! status: {:#x}", static_cast<u32>(status));
                return status;
            }
            lj::ilog("Io queue successfully created!");

            auto queue_ctx        = GetQueueContext(ctx->default_queue);
            queue_ctx->queue      = ctx->default_queue;
            queue_ctx->device_ctx = ctx;
        }

        {
            auto queue_config = WDF_IO_QUEUE_CONFIG {};
            WDF_IO_QUEUE_CONFIG_INIT(&queue_config, WdfIoQueueDispatchManual);

            auto queue_attributes = WDF_OBJECT_ATTRIBUTES {};
            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queue_attributes, Queue_context);

            status = WdfIoQueueCreate(device, &queue_config, &queue_attributes, &ctx->manual_queue);
            if (not NT_SUCCESS(status)) {
                lj::elog("Failed to create io queue! status): {:#x}", static_cast<u32>(status));
                return status;
            }
            lj::ilog("Manual io queue successfully created!");

            auto queue_ctx        = GetQueueContext(ctx->manual_queue);
            queue_ctx->queue      = ctx->manual_queue;
            queue_ctx->device_ctx = ctx;
        }

        status = WdfDeviceCreateDeviceInterface(device, &DEVICE_INTERFACE_GUID.fmtid, nullptr);
        if (not NT_SUCCESS(status)) {
            lj::elog("Failed to expose device interface! status: {:#x}", static_cast<u32>(status));
            return status;
        }

        ilog("{} connected!", default_controller.name);

        return status;
    }

#pragma code_seg()

#pragma code_seg("PAGED")

    auto event_io_device_control(_In_ WDFQUEUE   queue,
                                 _In_ WDFREQUEST request,
                                 _In_            usize,
                                 _In_            usize,
                                 _In_ ULONG      io_control_code) noexcept -> void {
        PAGED_CODE();

        lj::dlog("event_io_device_control Called!");

        auto queue_ctx  = GetQueueContext(queue);
        auto device_ctx = queue_ctx->device_ctx;

        auto request_completed = true;
        auto status            = NTSTATUS { STATUS_NOT_IMPLEMENTED };
        switch (io_control_code) {
            case IOCTL_HID_GET_DEVICE_DESCRIPTOR: {
                status = hid::get_device_descriptor(request, device_ctx->hid_descriptor);
            } break;
            case IOCTL_HID_GET_DEVICE_ATTRIBUTES: {
                status = hid::get_device_attributes(request, device_ctx->hid_attributes);
            } break;
            case IOCTL_HID_GET_REPORT_DESCRIPTOR: {
                status = hid::get_report_descriptor(request, device_ctx->report_descriptor);
            } break;
            case IOCTL_HID_READ_REPORT: {
                wlog("IOCTL_HID_READ_REPORT not supported");
                // status = ReadReport(queue_ctx, request, &request_completed);
            } break;

            case IOCTL_HID_WRITE_REPORT: {
                wlog("IOCTL_HID_WRITE_REPORT not supported");
                // status = WriteReport(queueContext, Request);
            } break;

            case IOCTL_HID_GET_STRING: wlog("IOCTL_HID_GET_STRING not supported");
            case IOCTL_HID_GET_INDEXED_STRING: wlog("IOCTL_HID_GET_INDEXED_STRING not supported");

            case IOCTL_HID_DEVICERESET_NOTIFICATION: wlog("IOCTL_HID_DEVICERESET_NOTIFICATION not supported");
            case IOCTL_HID_ACTIVATE_DEVICE: wlog("IOCTL_HID_ACTIVTE_DEVICE not supported");
            case IOCTL_HID_DEACTIVATE_DEVICE: wlog("IOCTL_HID_DEACTIVATE_DEVICE not supported");
            case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST: wlog("IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST not supported");
            case IOCTL_UMDF_GET_PHYSICAL_DESCRIPTOR: wlog("IOCTL_UMDF_GET_PHYSICAL_DESCRIPTOR not supported");
            case IOCTL_UMDF_HID_GET_FEATURE: wlog("IOCTL_UMDF_HID_GET_FEATURE not supported");
            case IOCTL_UMDF_HID_GET_INPUT_REPORT: wlog("IOCTL_UMDF_HID_GET_INPUT_REPORT not supported");
            case IOCTL_UMDF_HID_SET_FEATURE: wlog("IOCTL_UMDF_HID_SET_FEATURE not supported");
            case IOCTL_UMDF_HID_SET_OUTPUT_REPORT: wlog("IOCTL_UMDF_HID_SET_OUTPUT_REPORT not supported");
            default: break;
        }

        if (request_completed) WdfRequestComplete(request, status);
    }

#pragma code_seg()

#pragma code_seg("PAGED")

    auto event_device_cleanup(_In_ WDFOBJECT device) -> void {
        PAGED_CODE();

        lj::dlog("Cleanup up device {:#x}", std::bit_cast<uptr>(device));

        // EventWriteUnloadObject(device);
    }

#pragma code_seg()

    auto event_prepare_hardware(WDFDEVICE device, WDFCMRESLIST, WDFCMRESLIST) -> NTSTATUS {
        auto status = STATUS_SUCCESS;

        auto ctx = GetDeviceContext(device);

        status = WdfUsbTargetDeviceCreate(device, WDF_NO_OBJECT_ATTRIBUTES, &ctx->usb.device);
        if (not NT_SUCCESS(status)) {
            lj::elog("Failed to create USB device context! status: {:#x}", static_cast<u32>(status));
            return status;
        }

        WdfUsbTargetDeviceGetDeviceDescriptor(ctx->usb.device, &ctx->usb.descriptor);

        ctx->vendor_id  = ctx->usb.descriptor.idVendor;
        ctx->product_id = ctx->usb.descriptor.idProduct;

        lj::ilog("Usb attached, PID: {:#x}, VID: {:#x}", ctx->vendor_id, ctx->product_id);

        return status;
    }

} // namespace lj
