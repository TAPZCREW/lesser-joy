module;

#define WIN32_NO_STATUS
#include <stormkit/core/platform/windows.hpp>
#include <windows.h>
#undef WIN32_NO_STATUS
#include <devpropdef.h>
#include <ntstatus.h>
#include <wdf.h>

#include <hidport.h>

export module lesserjoy.device;

import std;

import stormkit.core;

import lesserjoy.log;
import lesserjoy.constants;

using namespace stormkit;

namespace stdr = std::ranges;

export {
    using Hid_report_descriptor = std::span<const u8>;

    struct Device_context {
        WDFDEVICE device;

        WDFQUEUE default_queue;
        WDFQUEUE manual_queue;

        HID_DESCRIPTOR        hid_descriptor;
        Hid_report_descriptor report_descriptor;
        HID_DEVICE_ATTRIBUTES hid_attributes;
    };

    using PDevice_context = Device_context*;
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Device_context, GetDeviceContext)

    struct Queue_context {
        WDFQUEUE queue;

        Device_context* device_ctx;
    };

    using PQueue_context = Queue_context*;
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Queue_context, GetQueueContext)

    struct Hid_control_info {
        u8 report_id;
        u8 control_code;
    };

    inline constexpr auto FEATURE_REPORT_SIZE_CB = sizeof(Hid_control_info) - 1;

    struct Hid_input_info {
        u8 report_id;
        u8 data;
    };

    inline constexpr auto INPUT_REPORT_SIZE_CB = sizeof(Hid_input_info) - 1;

    struct Hid_output_info {
        u8  report_id;
        u8  data;
        u16 _;
        u32 _;
    };

    inline constexpr auto OUTPUT_REPORT_SIZE_CB = sizeof(Hid_output_info) - 1;

    inline constexpr auto DEFAULT_REPORT_DESCRIPTOR = to_array<u8>({

      0x05,
      0x01, /* Usage Page (Generic Desktop) */
      0x09,
      0x05, /* Usage (Game Pad) */
      0xA1,
      0x01, /* Collection (Application) */
      0x85,
      0x01, /*   Report ID (1) */
      /* Left stick X, Y */
      0x09,
      0x30, /*   Usage (X) */
      0x09,
      0x31, /*   Usage (Y) */
      0x15,
      0x00, /*   Logical Minimum (0) */
      0x27,
      0xFF,
      0xFF,
      0x00,
      0x00, /*   Logical Maximum (65535) */
      0x75,
      0x10, /*   Report Size (16) */
      0x95,
      0x02, /*   Report Count (2) */
      0x81,
      0x02, /*   INPUT (Data, Var, Abs) */
      /* Right stick X, Y */
      0x09,
      0x33, /*   Usage (Rx) */
      0x09,
      0x34, /*   Usage (Ry) */
      0x81,
      0x02, /*   INPUT (Data, Var, Abs) */
      /* Triggers */
      0x09,
      0x32, /*   Usage (Z) */
      0x09,
      0x35, /*   Usage (Rz) */
      0x81,
      0x02, /*   INPUT (Data, Var, Abs) */
      /* Buttons 1-10 */
      0x05,
      0x09, /*   Usage Page (Button) */
      0x19,
      0x01, /*   Usage Minimum (1) */
      0x29,
      0x0A, /*   Usage Maximum (10) */
      0x15,
      0x00, /*   Logical Minimum (0) */
      0x25,
      0x01, /*   Logical Maximum (1) */
      0x75,
      0x01, /*   Report Size (1) */
      0x95,
      0x0A, /*   Report Count (10) */
      0x81,
      0x02, /*   INPUT (Data, Var, Abs) */
      /* 6-bit padding */
      0x75,
      0x06, /*   Report Size (6) */
      0x95,
      0x01, /*   Report Count (1) */
      0x81,
      0x01, /*   INPUT (Cnst) */
      /* Hat switch */
      0x05,
      0x01, /*   Usage Page (Generic Desktop) */
      0x09,
      0x39, /*   Usage (Hat switch) */
      0x15,
      0x01, /*   Logical Minimum (1) */
      0x25,
      0x08, /*   Logical Maximum (8) */
      0x35,
      0x00, /*   Physical Minimum (0) */
      0x46,
      0x3B,
      0x01, /*   Physical Maximum (315) */
      0x66,
      0x14,
      0x00, /*   Unit (Degrees) */
      0x75,
      0x04, /*   Report Size (4) */
      0x95,
      0x01, /*   Report Count (1) */
      0x81,
      0x42, /*   INPUT (Data, Var, Abs, Null) */
      /* 4-bit padding */
      0x75,
      0x04,
      0x95,
      0x01,
      0x15,
      0x00,
      0x25,
      0x00,
      0x35,
      0x00,
      0x45,
      0x00,
      0x65,
      0x00,
      0x81,
      0x03, /*   INPUT (Cnst, Var) */
      /* Feature report for user-mode → driver data channel */
      0x85,
      0x02, /*   Report ID (2) */
      0x06,
      0x00,
      0xFF, /*   Usage Page (Vendor Defined) */
      0x09,
      0x01, /*   Usage (0x01) */
      0x15,
      0x00, /*   Logical Minimum (0) */
      0x26,
      0xFF,
      0x00, /*   Logical Maximum (255) */
      0x75,
      0x08, /*   Report Size (8) */
      0x95,
      0x0E, /*   Report Count (14) */
      0xB1,
      0x02, /*   FEATURE (Data, Var, Abs) */
      0xC0  /* End Collection */
    });

    inline constexpr auto DEFAULT_HID_DESCRIPTOR = HID_DESCRIPTOR {
        0x09, // length of HID descriptor
        0x21, // descriptor type == HID  0x21
        0x0100, // hid spec release
        0x00, // country code == Not Specified
        0x01, // number of HID class descriptors
        {
                                             // DescriptorList[0]
          0x22,                             // report descriptor type 0x22
          sizeof(DEFAULT_REPORT_DESCRIPTOR) // total length of report descriptor
        }
    };
}

module: private;

namespace lj {
    EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL event_io_device_control;

    auto event_device_add(_In_ WDFDRIVER, _Inout_ PWDFDEVICE_INIT device_init) -> NTSTATUS {
        lj::dlog("Event device add!");

        WdfFdoInitSetFilter(device_init);

        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, Device_context);
        attributes.EvtCleanupCallback = nullptr;

        auto device = WDFDEVICE {};
        auto status = WdfDeviceCreate(&device_init, &attributes, &device);

        if (not NT_SUCCESS(status)) {
            lj::elog("Failed to create device! status: {:x}", status);
            return status;
        }

        auto ctx = GetDeviceContext(device);
        std::memset(ctx, 0, sizeof(Device_context));
        ctx->device = device;

        constexpr auto bus_desc_key = DEVPROPKEY {
            .fmtid = { 0x540b947e, 0x8b40, 0x45bc, { 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2 } },
            .pid   = 4
        };

        auto property_data = WDF_DEVICE_PROPERTY_DATA {};
        WDF_DEVICE_PROPERTY_DATA_INIT(&property_data, &bus_desc_key);
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

        {
            auto queue_config = WDF_IO_QUEUE_CONFIG {};
            WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue_config, WdfIoQueueDispatchParallel);
            queue_config.EvtIoDeviceControl = event_io_device_control;

            auto queue_attributes = WDF_OBJECT_ATTRIBUTES {};
            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queue_attributes, Queue_context);

            status = WdfIoQueueCreate(device, &queue_config, &queue_attributes, &ctx->default_queue);
            if (not NT_SUCCESS(status)) {
                lj::elog("Failed to create io queue! status: {:x}", status);
                return status;
            }

            auto queue_ctx        = GetQueueContext(ctx->default_queue);
            queue_ctx->queue      = ctx->default_queue;
            queue_ctx->device_ctx = ctx;
        }

        {
            auto queue_config = WDF_IO_QUEUE_CONFIG {};
            WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue_config, WdfIoQueueDispatchManual);

            auto queue_attributes = WDF_OBJECT_ATTRIBUTES {};
            WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queue_attributes, Queue_context);

            status = WdfIoQueueCreate(device, &queue_config, &queue_attributes, &ctx->manual_queue);
            if (not NT_SUCCESS(status)) {
                lj::elog("Failed to create io queue! status: {:x}", status);
                return status;
            }

            auto queue_ctx        = GetQueueContext(ctx->manual_queue);
            queue_ctx->queue      = ctx->manual_queue;
            queue_ctx->device_ctx = ctx;
        }

        ctx->hid_descriptor    = DEFAULT_HID_DESCRIPTOR;
        ctx->report_descriptor = DEFAULT_REPORT_DESCRIPTOR;

        return status;
    }

    NTSTATUS
    RequestCopyFromBuffer(_In_ WDFREQUEST Request,
                          _In_ PVOID      SourceBuffer,
                          _When_(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
                            _In_ size_t NumBytesToCopyFrom) {
        NTSTATUS  status;
        WDFMEMORY memory;
        size_t    outputBufferLength;

        status = WdfRequestRetrieveOutputMemory(Request, &memory);
        if (!NT_SUCCESS(status)) {
            KdPrint(("WdfRequestRetrieveOutputMemory failed 0x%x\n", status));
            return status;
        }

        WdfMemoryGetBuffer(memory, &outputBufferLength);
        if (outputBufferLength < NumBytesToCopyFrom) {
            status = STATUS_INVALID_BUFFER_SIZE;
            KdPrint(("RequestCopyFromBuffer: buffer too small. Size %d, expect %d\n",
                     (int)outputBufferLength,
                     (int)NumBytesToCopyFrom));
            return status;
        }

        status = WdfMemoryCopyFromBuffer(memory, 0, SourceBuffer, NumBytesToCopyFrom);
        if (!NT_SUCCESS(status)) {
            KdPrint(("WdfMemoryCopyFromBuffer failed 0x%x\n", status));
            return status;
        }

        WdfRequestSetInformation(Request, NumBytesToCopyFrom);
        return status;
    }

    auto event_io_device_control(_In_ WDFQUEUE queue, _In_ WDFREQUEST request, _In_ usize, _In_ usize, _In_ ULONG io_control_code)
      -> void {
        auto queue_ctx  = GetQueueContext(queue);
        auto device_ctx = queue_ctx->device_ctx;

        auto request_completed = true;
        auto status            = NTSTATUS { STATUS_NOT_IMPLEMENTED };
        switch (io_control_code) {
            case IOCTL_HID_GET_DEVICE_DESCRIPTOR: {
                status = RequestCopyFromBuffer(request, &device_ctx->hid_descriptor, device_ctx->hid_descriptor.bLength);
            } break;
            case IOCTL_HID_GET_DEVICE_ATTRIBUTES: {
                status = RequestCopyFromBuffer(request, &device_ctx->hid_attributes, sizeof(HID_DEVICE_ATTRIBUTES));
            } break;
            case IOCTL_HID_GET_REPORT_DESCRIPTOR: {
                status = RequestCopyFromBuffer(request,
                                               &device_ctx->report_descriptor,
                                               device_ctx->hid_descriptor.DescriptorList[0].wReportLength);
            } break;
            case IOCTL_HID_READ_REPORT: {
                // status = ReadReport(queue_ctx, request, &request_completed);
            } break;

            case IOCTL_HID_WRITE_REPORT: {
                // status = WriteReport(queueContext, Request);
            } break;
            default: break;
        }

        if (request_completed) WdfRequestComplete(request, status);
    }
} // namespace lj
