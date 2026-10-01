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
    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::create(_In_ WDFDRIVER driver, _Inout_ PWDFDEVICE_INIT device_init) -> NTSTATUS {
        // initialize filter
        WdfFdoInitSetFilter(device_init);

        // Create and fill the PnP power callbacks configuration
        auto power_callbacks = WDF_PNPPOWER_EVENT_CALLBACKS {};
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&power_callbacks);
        power_callbacks.EvtDevicePrepareHardware = prepare_hardware;
        power_callbacks.EvtDeviceReleaseHardware = release_hardware;
        power_callbacks.EvtDeviceD0Entry         = device_entry;
        power_callbacks.EvtDeviceD0Exit          = device_exit;

        // set the Pnp power callback
        WdfDeviceInitSetPnpPowerEventCallbacks(device_init, &power_callbacks);

        // Create and fill the device attributes
        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, device_context);
        // attributes.ParentObject       = driver;
        attributes.EvtDestroyCallback = destroy;

        // create the actual device
        auto device = WDFDEVICE {};
        LoggedTryOr(lj::win_call(WdfDeviceCreate, &device_init, &attributes, &device),
                    monadic::map(monadic::unwrap(), monadic::as<NTSTATUS>()),
                    "Failed to create device!");

        // construct the device class into the raw memory that WDF allocated
        // via placement new
        auto device_ctx = get_device_context(device);
        expects(device_ctx != nullptr);
        new (device_ctx) device_context { device };

        // create and expose device hid interface
        LoggedTryOr(lj::win_call(WdfDeviceCreateDeviceInterface, device, &DEV_INTERFACE_HID_GUID, nullptr),
                    monadic::map(monadic::unwrap(), monadic::as<NTSTATUS>()),
                    "Failed to expose device interface!");

        queue_context::create(device);

        return STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::destroy(_In_ WDFOBJECT device) noexcept -> void {
        lj::dlog("Destroying device {}", std::bit_cast<uptr>(device));

        // Manually call the destructor because contextes are constructed with placement new
        auto ctx = get_device_context(device);
        expects(ctx != nullptr);
        ctx->~device_context();
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::prepare_hardware() noexcept -> system_result<void> {
        // currently we only support the pro controller
        // so it's hardcoded
        constexpr auto DEFAULT_CONTROLLER = CONTROLLERS_TYPE.at("pro_controller");

        vendor_id_  = DEFAULT_CONTROLLER.vid;
        product_id_ = DEFAULT_CONTROLLER.pid;

        hid_attributes_.Size      = sizeof(HID_DEVICE_ATTRIBUTES);
        hid_attributes_.VendorID  = DEFAULT_CONTROLLER.vid;
        hid_attributes_.ProductID = DEFAULT_CONTROLLER.pid;
        hid_descriptor_           = hid::DEFAULT_DESCRIPTOR;
        report_descriptor_        = hid::DEFAULT_REPORT_DESCRIPTOR;

        // setup hid output report
        stdr::copy(hid::DEFAULT_OUTPUT_REPORT, stdr::begin(output_report_));

        // initialize usb context
        lj::ilog("{} attached (USB), PID: {:#x}, VID: {:#x}", product_string_, vendor_id_, product_id_);
        transport_ = usb::context {};
        LoggedTry(usb::init_context(*this), "Device prepare hardware failed!");

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::release_hardware() noexcept -> system_result<void> {
        // LoggedTryOr(usb::init_context(*ctx, device),
        //             monadic::map(monadic::unwrap(), monadic::as<NTSTATUS>()),
        //             "Device prepare hardware failed!");
        // lj::ilog("{} attached (USB), PID: {:#x}, VID: {:#x}", ctx->product_string, ctx->vendor_id, ctx->product_id);
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::device_entry() noexcept -> system_result<void> {
        LoggedTry(usb::event_device_entry(*this), "Device entry failed!");
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::device_exit() noexcept -> system_result<void> {
        LoggedTry(usb::event_device_exit(*this), "Device exit failed!");
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::prepare_hardware(_In_ WDFDEVICE device, _In_ WDFCMRESLIST, _In_ WDFCMRESLIST) noexcept -> NTSTATUS {
        auto ctx = get_device_context(device);
        expects(ctx != nullptr);

        LoggedTryOr(ctx->prepare_hardware(),
                    monadic::map(monadic::unwrap(), monadic::as<NTSTATUS>()),
                    "Device prepare hardware failed!");

        return STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::release_hardware(_In_ WDFDEVICE device, _In_ WDFCMRESLIST) noexcept -> NTSTATUS {
        auto ctx = get_device_context(device);
        expects(ctx != nullptr);

        LoggedTryOr(ctx->release_hardware(),
                    monadic::map(monadic::unwrap(), monadic::as<NTSTATUS>()),
                    "Device release hardware failed!");

        return STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::device_entry(_In_ WDFDEVICE device, _In_ WDF_POWER_DEVICE_STATE) noexcept -> NTSTATUS {
        auto ctx = get_device_context(device);
        expects(ctx != nullptr);

        LoggedTryOr(ctx->device_entry(), monadic::map(monadic::unwrap(), monadic::as<NTSTATUS>()), "Device entry failed!");

        return STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::device_exit(_In_ WDFDEVICE device, _In_ WDF_POWER_DEVICE_STATE) noexcept -> NTSTATUS {
        auto ctx = get_device_context(device);
        expects(ctx != nullptr);

        LoggedTryOr(ctx->device_exit(), monadic::map(monadic::unwrap(), monadic::as<NTSTATUS>()), "Device exit failed!");

        return STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto queue_context::create(_In_ WDFDEVICE device) noexcept -> NTSTATUS {
        // Create and fill the io queue configuration and attributes
        auto config = WDF_IO_QUEUE_CONFIG {};
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&config, WdfIoQueueDispatchParallel);
        config.PowerManaged       = WdfTrue;
        config.EvtIoDeviceControl = io_control;

        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, queue_context);
        attributes.ParentObject       = device;
        attributes.EvtDestroyCallback = queue_context::destroy;

        // create the actual io queue
        auto queue = WDFQUEUE { nullptr };
        LoggedTryOr(lj::win_call(WdfIoQueueCreate, device, &config, &attributes, &queue),
                    monadic::map(monadic::unwrap(), monadic::as<NTSTATUS>()),
                    "Failed to create io queue!");

        // construct the device class into the raw memory that WDF allocated
        // via placement new
        auto queue_ctx = get_queue_context(queue);
        expects(queue_ctx != nullptr);
        new (queue_ctx) queue_context { get_device_context(device), queue };

        return STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto queue_context::io_control(WDFQUEUE   queue,
                                   WDFREQUEST request,
                                   usize      output_buffer_size,
                                   usize      input_buffer_size,
                                   u32        io_control_code,
                                   tag) const noexcept -> void {
        auto request_completed = true;
        auto status            = NTSTATUS { STATUS_SUCCESS };

        struct AtExit {
            AtExit(const bool& completed_, WDFREQUEST request_, const NTSTATUS& status_) noexcept
                : completed { completed_ }, request { request_ }, status { status_ } {}

            ~AtExit() noexcept {
                if (completed) WdfRequestComplete(request, status);
            }

            const bool&     completed;
            WDFREQUEST      request;
            const NTSTATUS& status;
        } _ { request_completed, request, status };

        const auto update_status = [&status](auto&& error) noexcept { status = error.value(); };

        const auto& device_ctx = *device_ctx_;

        switch (io_control_code) {
            case IOCTL_HID_GET_DEVICE_DESCRIPTOR: {
                LoggedTryOr(hid::ioctl::get_device_descriptor(request, device_ctx.hid_descriptor()),
                            update_status,
                            "IOCTL Failed to get device descriptor!");
            } break;

            case IOCTL_HID_GET_DEVICE_ATTRIBUTES: {
                LoggedTryOr(hid::ioctl::get_device_attributes(request, device_ctx.hid_attributes()),
                            update_status,
                            "IOCTL Failed to get device attributes!");
            } break;

            case IOCTL_HID_GET_REPORT_DESCRIPTOR: {
                LoggedTryOr(hid::ioctl::get_report_descriptor(request, device_ctx.report_descriptor()),
                            update_status,
                            "IOCTL Failed to get report descriptor!");
            } break;

            case IOCTL_HID_READ_REPORT: {
                LoggedTryOr(hid::ioctl::read_report(request, device_ctx.usb_ctx()),
                            update_status,
                            "IOCTL Failed to read report!");
            } break;

            case IOCTL_HID_WRITE_REPORT: {
                LoggedTryOr(hid::ioctl::write_report(request, device_ctx.output_report()),
                            update_status,
                            "IOCTL Failed to write report!");
            } break;

            case IOCTL_HID_GET_STRING: {
                LoggedTryOr(hid::ioctl::get_string(request, device_ctx.product_string(), device_ctx.product_string()),
                            update_status,
                            "IOCTL Failed to get string!");
            } break;

            case IOCTL_HID_GET_INDEXED_STRING: {
                LoggedTryOr(hid::ioctl::get_indexed_string(request, device_ctx.serial_string()),
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
    auto queue_context::destroy(_In_ WDFOBJECT queue) noexcept -> void {
        lj::dlog("Destroying queue {}", std::bit_cast<uptr>(queue));

        // Manually call the destructor because contextes are constructed with placement new
        auto ctx = get_queue_context(queue);
        expects(ctx != nullptr);
        ctx->~queue_context();
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto queue_context::io_control(_In_ WDFQUEUE   queue,
                                   _In_ WDFREQUEST request,
                                   _In_ usize      output_buffer_size,
                                   _In_ usize      input_buffer_size,
                                   _In_ ULONG      io_control_code) noexcept -> void {
        auto queue_ctx = get_queue_context(queue);
        expects(queue_ctx != nullptr);

        queue_ctx->io_control(queue, request, output_buffer_size, input_buffer_size, as<u32>(io_control_code), tag {});
    }
} // namespace lj
