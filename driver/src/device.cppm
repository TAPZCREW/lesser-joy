module;

#include "windows.hpp"

#include "usb.hpp"

#include <stormkit/core/try_expected.hpp>

export module lesserjoy.device;

import std;

import stormkit.core;

import lesserjoy.log;
import lesserjoy.constants;
import lesserjoy.hid;
import lesserjoy.ntstatus;
import lesserjoy.usb;

using namespace stormkit;

namespace stdr = std::ranges;

export namespace lj {
    struct Device_context {
        WDFDEVICE device = nullptr;

        WDFQUEUE default_queue = nullptr;
        WDFQUEUE manual_queue  = nullptr;

        HID_DESCRIPTOR         hid_descriptor    = {};
        hid::Report_descriptor report_descriptor = {};
        HID_DEVICE_ATTRIBUTES  hid_attributes    = {};

        WDFMEMORY output_report_memory = nullptr;

        usb::Usb_device_context usb = {};

        u16 vendor_id  = 0;
        u16 product_id = 0;

        string product_string = {};

        array<byte, 64> input_report = {};
    };

    STORMKIT_PUSH_WARNINGS
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
    using PDevice_context = Device_context*;
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Device_context, GetDeviceContext)
    STORMKIT_POP_WARNINGS

    struct Queue_context {
        WDFQUEUE queue = nullptr;

        Device_context* device_ctx = nullptr;
    };

    STORMKIT_PUSH_WARNINGS
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
    using PQueue_context = Queue_context*;
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Queue_context, GetQueueContext)
    STORMKIT_POP_WARNINGS

    EVT_WDF_DRIVER_DEVICE_ADD      event_device_add;
    EVT_WDF_OBJECT_CONTEXT_CLEANUP event_device_cleanup;
} // namespace lj

module: private;

using namespace stormkit::literals;

auto format_as(const USB_DEVICE_DESCRIPTOR& descriptor, auto& ctx) noexcept -> decltype(ctx.out()) {
    return std::format_to(ctx.out(),
                          "[USB_DEVICE_DESCRIPTOR\n"
                          "    bLength:            {}\n"
                          "    bDescriptorType:    {}\n"
                          "    bcdUSB:             {}\n"
                          "    bDeviceClass:       {}\n"
                          "    bDeviceSubClass:    {}\n"
                          "    bDeviceProtocol:    {}\n"
                          "    bMaxPacketSize0:    {}\n"
                          "    idVendor:           {:#x}\n"
                          "    idProduct:          {:#x}\n"
                          "    bcdDevice:          {}\n"
                          "    iSerialNumber:      {}\n"
                          "    bNumConfigurations: {}]",
                          descriptor.bLength,
                          descriptor.bDescriptorType,
                          descriptor.bcdUSB,
                          descriptor.bDeviceClass,
                          descriptor.bDeviceSubClass,
                          descriptor.bDeviceProtocol,
                          descriptor.bMaxPacketSize0,
                          descriptor.idVendor,
                          descriptor.idProduct,
                          descriptor.bcdDevice,
                          descriptor.iSerialNumber,
                          descriptor.bNumConfigurations);
}

namespace lj {
    EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL event_io_device_control;
    EVT_WDF_DEVICE_PREPARE_HARDWARE    event_prepare_hardware;
    EVT_WDF_DEVICE_D0_ENTRY            event_device_entry;
    EVT_WDF_DEVICE_D0_EXIT             event_device_exit;

    auto init_device_context(WDFDEVICE device) -> Expected<void> {
        auto ctx = GetDeviceContext(device);
        *ctx     = Device_context {};

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

        LoggedTry(lj::win_call(WdfMemoryCreate,
                               &attributes,
                               NonPagedPoolNx,
                               POOL_TAG,
                               64,
                               &ctx->output_report_memory,
                               std::bit_cast<PVOID*>(&report_buffer)),
                  "Failed to allocate hid report buffer memory!");

        // std::memcpy(report_buffer, stdr::data(hid::DEFAULT_OUTPUT_REPORT), sizeof(hid::DEFAULT_OUTPUT_REPORT));

        Return {};
    }

#pragma code_seg("PAGED")

    _Use_decl_annotations_ auto event_device_add(_In_ WDFDRIVER, _Inout_ PWDFDEVICE_INIT device_init) -> NTSTATUS {
        PAGED_CODE();

        lj::dlog("Event device add!");

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

        LoggedTryOr(init_device_context(device), monadic::unwrap(), "Failed to device context!");

        auto        ctx                = GetDeviceContext(device);
        const auto& default_controller = CONTROLLERS_TYPE.at("pro_controller");

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

            LoggedTryOr(lj::win_call(WdfIoQueueCreate, device, &queue_config, &queue_attributes, &ctx->manual_queue),
                        monadic::unwrap(),
                        "Failed to create io queue!");
            lj::ilog("Manual io queue successfully created!");

            auto queue_ctx        = GetQueueContext(ctx->manual_queue);
            queue_ctx->queue      = ctx->manual_queue;
            queue_ctx->device_ctx = ctx;
        }

        LoggedTryOr(lj::win_call(WdfDeviceCreateDeviceInterface, device, &DEVICE_INTERFACE_GUID.fmtid, nullptr),
                    monadic::unwrap(),
                    "Failed to expose device interface!");

        return STATUS_SUCCESS;
    }

#pragma code_seg()

#pragma code_seg("PAGED")

    _Use_decl_annotations_ auto event_io_device_control(_In_ WDFQUEUE   queue,
                                                        _In_ WDFREQUEST request,
                                                        _In_ usize      output_buffer_size,
                                                        _In_ usize      input_buffer_size,
                                                        _In_ ULONG      io_control_code) -> void {
        PAGED_CODE();

        lj::dlog("event_io_device_control Called!");

        auto queue_ctx  = GetQueueContext(queue);
        auto device_ctx = queue_ctx->device_ctx;

        auto request_completed = true;
        auto status            = NTSTATUS { STATUS_SUCCESS };

        const auto update_status = [&status](auto&& error) noexcept { status = error.value(); };

        switch (io_control_code) {
            case IOCTL_HID_GET_DEVICE_DESCRIPTOR: {
                LoggedTryOr(hid::get_device_descriptor(request, device_ctx->hid_descriptor),
                            update_status,
                            "Failed to get device descriptor!");
            } break;

            case IOCTL_HID_GET_DEVICE_ATTRIBUTES: {
                LoggedTryOr(hid::get_device_attributes(request, device_ctx->hid_attributes),
                            update_status,
                            "Failed to get device attributes!");
            } break;

            case IOCTL_HID_GET_REPORT_DESCRIPTOR: {
                LoggedTryOr(hid::get_report_descriptor(request, device_ctx->report_descriptor),
                            update_status,
                            "Failed to get report descriptor");
            } break;

            case IOCTL_HID_READ_REPORT: {
                wlog("IOCTL_HID_READ_REPORT not supported {}", input_buffer_size);
                status = STATUS_NOT_IMPLEMENTED;
                // status = ReadReport(queue_ctx, request, &request_completed);
            } break;

            case IOCTL_HID_WRITE_REPORT: {
                status = STATUS_NOT_IMPLEMENTED;
                wlog("IOCTL_HID_WRITE_REPORT not supported {}", output_buffer_size);
                // status = WriteReport(queueContext, Request);
            } break;

            case IOCTL_HID_GET_STRING: {
                wlog("IOCTL_HID_GET_STRING not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            case IOCTL_HID_GET_INDEXED_STRING: {
                wlog("IOCTL_HID_GET_INDEXED_STRING not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            case IOCTL_HID_DEVICERESET_NOTIFICATION: {
                wlog("IOCTL_HID_DEVICERESET_NOTIFICATION not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            case IOCTL_HID_ACTIVATE_DEVICE: {
                wlog("IOCTL_HID_ACTIVTE_DEVICE not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            case IOCTL_HID_DEACTIVATE_DEVICE: {
                wlog("IOCTL_HID_DEACTIVATE_DEVICE not supported");
                status = STATUS_NOT_IMPLEMENTED;
            } break;

            case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST: {
                wlog("IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST not supported");
                status = STATUS_NOT_IMPLEMENTED;
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

        if (request_completed) WdfRequestComplete(request, status);
    }

#pragma code_seg()

#pragma code_seg("PAGED")

    _Use_decl_annotations_ auto event_device_cleanup(_In_ WDFOBJECT device) -> void {
        PAGED_CODE();

        lj::dlog("Cleanup up device {}", std::bit_cast<uptr>(device));

        // EventWriteUnloadObject(device);
    }

#pragma code_seg()

    ////////////////////////////////////////
    ////////////////////////////////////////
    inline auto wide_to_ascii(wstring_view input) -> string {
        [[maybe_unused]]
        auto state = std::mbstate_t {};
        [[maybe_unused]]
        auto output = string {};

        auto count = WideCharToMultiByte(CP_ACP, 0, stdr::data(input), stdr::size(input), nullptr, 0, nullptr, nullptr);
        output.resize(count);

        WideCharToMultiByte(CP_UTF8,
                            0,
                            stdr::data(input),
                            stdr::size(input),
                            stdr::data(output),
                            stdr::size(output),
                            nullptr,
                            nullptr);

        // // #if defined(STORMKIT_COMPILER_MSVC)
        // for (const auto& c : input) [[maybe_unused]]
        //     auto _ = std::c16rtomb(stdr::data(output), narrow<char16_t>(c), &state);

        return output;
    }

    _Use_decl_annotations_ auto event_prepare_hardware(WDFDEVICE device, WDFCMRESLIST, WDFCMRESLIST) -> NTSTATUS {
        auto status = STATUS_SUCCESS;

        auto ctx = GetDeviceContext(device);

        // Init usb device

        if (ctx->usb.device == nullptr) {
            auto init_config = WDF_USB_DEVICE_CREATE_CONFIG {};
            WDF_USB_DEVICE_CREATE_CONFIG_INIT(&init_config, 0x602);

            LoggedTryOr(lj::win_call(WdfUsbTargetDeviceCreateWithParameters,
                                     device,
                                     &init_config,
                                     WDF_NO_OBJECT_ATTRIBUTES,
                                     &ctx->usb.device),
                        monadic::unwrap(),
                        "Failed to create USB device context!");
        }

        WdfUsbTargetDeviceGetDeviceDescriptor(ctx->usb.device, &ctx->usb.descriptor);

        ctx->vendor_id  = ctx->usb.descriptor.idVendor;
        ctx->product_id = ctx->usb.descriptor.idProduct;

        // TODO read about USB interfaces to ensure correct usage
        auto select_config = WDF_USB_DEVICE_SELECT_CONFIG_PARAMS {};
        WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(&select_config, 0, nullptr);

        LoggedTryOr(lj::win_call(WdfUsbTargetDeviceSelectConfig, ctx->usb.device, WDF_NO_OBJECT_ATTRIBUTES, &select_config),
                    monadic::unwrap(),
                    "Failed to configure USB device!");

        ctx->usb.interface = select_config.Types.SingleInterface.ConfiguredUsbInterface;

        lj::ilog("Usb attached, PID: {}, VID: {}", ctx->vendor_id, ctx->product_id);

        // get product name if available
        const auto result = lj::win_call(WdfUsbTargetDeviceAllocAndQueryString,
                                         ctx->usb.device,
                                         WDF_NO_OBJECT_ATTRIBUTES,
                                         &ctx->usb.product_string,
                                         nullptr,
                                         ctx->usb.descriptor.iProduct,
                                         0x409);
        if (result.has_value()) {
            auto size           = 0_usize;
            auto memory_buffer  = WdfMemoryGetBuffer(ctx->usb.product_string, &size);
            ctx->product_string = wide_to_ascii({ std::bit_cast<const wchar_t*>(memory_buffer), (size / sizeof(wchar_t)) });
            // WDF_DEVICE_PROPERTY_DATA_INIT(&)
        } else
            lj::wlog("Failed to get product string from USB device!\n    error: {}", result.error());

        return status;
    }

    _Use_decl_annotations_ auto event_device_entry(WDFDEVICE device, WDF_POWER_DEVICE_STATE) -> NTSTATUS {
        auto ctx = GetDeviceContext(device);

        lj::ilog("{} connected! (USB)", ctx->product_string);

        // auto LoggedTryOr(lj::win_call(WdfIoTargetStart,device, &queue_config, &queue_attributes, &ctx->manual_queue), );
        // if (not NT_SUCCESS(status)) {
        //     lj::elog("Failed to start interrupt read pipe: {}", narrow<Ntstatus>(status));
        //     return status;
        // }

        // init sequence
        // initialize USB
        constexpr auto transport = hid::Transport::USB;
        LoggedTryOr(usb::send_command(ctx->usb, hid::commands::init::initialize_usb(transport)),
                    monadic::unwrap(),
                    "Failed to initialize USB link!");
        LoggedTryOr(usb::send_command(ctx->usb, hid::commands::UNKNOWN_COMMAND_0x07),
                    monadic::unwrap(),
                    "Unknown Command (0x07) failed!");
        LoggedTryOr(usb::send_command(ctx->usb, hid::commands::leds::all_leds_off(transport)),
                    monadic::unwrap(),
                    "Failed to clear LEDs state!");
        LoggedTryOr(usb::send_command(ctx->usb, hid::commands::SET_FEATURE_MASK),
                    monadic::unwrap(),
                    "Failed to set feature mask!");
        LoggedTryOr(usb::send_command(ctx->usb, hid::commands::UNKNOWN_COMMAND_0x11),
                    monadic::unwrap(),
                    "Unknown command (0x11) failed!");
        LoggedTryOr(usb::send_command(ctx->usb, hid::commands::RESET_VIBRATION_STATE),
                    monadic::unwrap(),
                    "Failed to reset vibration state!");
        LoggedTryOr(usb::send_command(ctx->usb, hid::commands::NFC_UNKNOWN_COMMAND),
                    monadic::unwrap(),
                    "Unknown command (NFC) failed!");
        LoggedTryOr(usb::send_command(ctx->usb, hid::commands::init::enable_usb_hid_report()),
                    monadic::unwrap(),
                    "Failed to enable HID reports!");
        LoggedTryOr(usb::send_command(ctx->usb, hid::commands::init::select_input_report(transport, 0x05_b)),
                    monadic::unwrap(),
                    "Failed to enable HID reports!");
        LoggedTryOr(usb::send_command(ctx->usb, hid::commands::leds::set_player_1(transport)),
                    monadic::unwrap(),
                    "Failed to setup player LED");

        ilog("{} initialized!", ctx->product_string);

        return STATUS_SUCCESS;
    }

    _Use_decl_annotations_ auto event_device_exit(WDFDEVICE device, WDF_POWER_DEVICE_STATE) -> NTSTATUS {
        auto ctx = GetDeviceContext(device);

        lj::ilog("{} disconnected! (USB)", ctx->product_string);
        return STATUS_SUCCESS;
    }
} // namespace lj
