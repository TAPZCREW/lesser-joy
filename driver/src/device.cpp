module;

#include <stormkit/core/try_expected.hpp>

#include "contract_macro.hpp"
#include "wdk/wdfobject_macros.hpp"

module lesserjoy;

import :wdf;
import :log;
import :hid;
import :transport.usb;

using namespace stormkit;
using namespace stormkit::literals;

namespace stdr = std::ranges;

namespace lj {
    // how long an output-report ioctl waits for the controller to accept the write before the consumer is
    // told it failed: rumble consumers care about the answer, not about the USB queue accepting the request.
    static constexpr auto output_report_send_timeout_ms = 1000_u32;

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::create(wdk::WDFDRIVER driver, wdk::PWDFDEVICE_INIT device_init) -> wdk::NTSTATUS {
        // initialize filter
        wdk::WdfFdoInitSetFilter(device_init);

        // Create and fill the PnP power callbacks configuration
        auto power_callbacks = wdk::WDF_PNPPOWER_EVENT_CALLBACKS {};
        wdk::WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&power_callbacks);
        power_callbacks.EvtDevicePrepareHardware = prepare_hardware;
        power_callbacks.EvtDeviceReleaseHardware = release_hardware;
        power_callbacks.EvtDeviceD0Entry         = device_entry;
        power_callbacks.EvtDeviceD0Exit          = device_exit;

        // set the Pnp power callback
        wdk::WdfDeviceInitSetPnpPowerEventCallbacks(device_init, &power_callbacks);

        // Create and fill the device context attributes
        auto attributes = wdk::WDF_OBJECT_ATTRIBUTES {};
        wdk::WdfObjectAttributesInitContextType(attributes, _WDF_device_context_TYPE_INFO);
        attributes.EvtDestroyCallback = destroy_ctx<device_context, get_device_context>;

        // create the actual device
        auto device = wdk::WDFDEVICE {};
        LoggedTryOr(lj::win_call(wdk::WdfDeviceCreate, &device_init, &attributes, &device),
                    monadic::map(monadic::unwrap(), monadic::as<wdk::NTSTATUS>()),
                    "Failed to create device!");

        // construct the device class into the raw memory that wdk::WDF allocated
        // via placement new
        auto device_ctx = get_device_context(device);
        WDK_ASSERT_NTSTATUS(device_ctx != nullptr);
        new (device_ctx) device_context { device };

        // create and expose device hid interface
        LoggedTryOr(lj::win_call(wdk::WdfDeviceCreateDeviceInterface, device, &dev_interface_hid_guid, nullptr),
                    monadic::map(monadic::unwrap(), monadic::as<wdk::NTSTATUS>()),
                    "Failed to expose device interface!");
        LoggedTryOr(lj::win_call(queue_context::create, device),
                    monadic::map(monadic::unwrap(), monadic::as<wdk::NTSTATUS>()),
                    "Failed to create queue context!");

        return wdk::STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::prepare_hardware() noexcept -> system_result<void> {
        // currently we only support the pro controller
        // so it's hardcoded
        static constexpr auto default_controller = controllers_type.at("pro_controller");

        vendor_id_  = default_controller.vid;
        product_id_ = default_controller.pid;

        hid_attributes_.Size      = sizeof(wdk::HID_DEVICE_ATTRIBUTES);
        hid_attributes_.VendorID  = default_controller.vid;
        hid_attributes_.ProductID = default_controller.pid;
        hid_descriptor_           = hid::default_descriptor;
        report_descriptor_        = hid::default_report_descriptor;
        product_string_           = default_controller.product_string;
        manufacturer_string_      = default_controller.manufacturer_string;

        // set BusReportedDeviceDesc so joy.cpl shows the profile name
        // DEVPKEY_Device_BusReportedDeviceDesc = {540b947e-8b40-45bc-a8a2-6a0b894cbda2}
        constexpr auto bus_desc_key = wdk::DEVPROPKEY {
            { 0x540b947e, 0x8b40, 0x45bc, { 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2 } },
            4
        };

        auto prop_data = wdk::WDF_DEVICE_PROPERTY_DATA {};
        wdk::WDF_DEVICE_PROPERTY_DATA_INIT(&prop_data, &bus_desc_key);
        prop_data.Lcid = 0x00; // LOCALE_NEUTRAL
        auto _         = wdk::WdfDeviceAssignProperty(device_,
                                                      &prop_data,
                                                      wdk::DEVPROP_TYPE_STRING,
                                                      stdr::size(product_string_) + 1, // +1 to take account of null terminator
                                                      stdr::data(product_string_));

        // initialize usb context
        LoggedTryTo(transport, transport::usb_context::create(*this), "Failed to allocate usb context!");
        transport_ = std::move(transport);

        lj::ilog("{} attached (USB), PID: {:#x}, VID: {:#x}", product_string_, vendor_id_, product_id_);

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::release_hardware() noexcept -> system_result<void> {
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::device_entry() noexcept -> system_result<void> {
        if (is_usb()) { LoggedTry(usb_ctx().device_entry(), "Device entry failed!"); }
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::device_exit() noexcept -> system_result<void> {
        if (is_usb()) { LoggedTry(usb_ctx().device_exit(), "Device exit failed!"); }
        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::prepare_hardware(wdk::WDFDEVICE device, wdk::WDFCMRESLIST, wdk::WDFCMRESLIST) noexcept -> wdk::NTSTATUS {
        auto ctx = get_device_context(device);
        WDK_EXPECTS_NTSTATUS(ctx != nullptr);

        LoggedTryOr(ctx->prepare_hardware(),
                    monadic::map(monadic::unwrap(), monadic::as<wdk::NTSTATUS>()),
                    "Device prepare hardware failed!");

        return wdk::STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::release_hardware(wdk::WDFDEVICE device, wdk::WDFCMRESLIST) noexcept -> wdk::NTSTATUS {
        auto ctx = get_device_context(device);
        WDK_EXPECTS_NTSTATUS(ctx != nullptr);

        CustomLoggedTryOr(ctx->release_hardware(),
                          monadic::map(monadic::unwrap(), monadic::as<wdk::NTSTATUS>()),
                          lj::elog,
                          "Device release hardware failed!");

        return wdk::STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::device_entry(wdk::WDFDEVICE device, wdk::WDF_POWER_DEVICE_STATE) noexcept -> wdk::NTSTATUS {
        auto ctx = get_device_context(device);
        WDK_EXPECTS_NTSTATUS(ctx != nullptr);

        CustomLoggedTryOr(ctx->device_entry(),
                          monadic::map(monadic::unwrap(), monadic::as<wdk::NTSTATUS>()),
                          lj::elog,
                          "Device entry failed!");

        return wdk::STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto device_context::device_exit(wdk::WDFDEVICE device, wdk::WDF_POWER_DEVICE_STATE) noexcept -> wdk::NTSTATUS {
        auto ctx = get_device_context(device);
        WDK_EXPECTS_NTSTATUS(ctx != nullptr);

        CustomLoggedTryOr(ctx->device_exit(),
                          monadic::map(monadic::unwrap(), monadic::as<wdk::NTSTATUS>()),
                          lj::elog,
                          "Device exit failed!");

        return wdk::STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto queue_context::create(wdk::WDFDEVICE device) noexcept -> wdk::NTSTATUS {
        // Create and fill the io queue configuration and attributes
        auto config = wdk::WDF_IO_QUEUE_CONFIG {};
        wdk::WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&config, wdk::WDF_IO_QUEUE_DISPATCH_TYPE::WdfIoQueueDispatchParallel);
        config.PowerManaged       = wdk::WDF_TRI_STATE::WdfTrue;
        config.EvtIoDeviceControl = io_control;

        // Create and fill the queue context attributes
        auto attributes = wdk::WDF_OBJECT_ATTRIBUTES {};
        wdk::WdfObjectAttributesInitContextType(attributes, _WDF_queue_context_TYPE_INFO);
        attributes.ParentObject       = device;
        attributes.EvtDestroyCallback = destroy_ctx<queue_context, get_queue_context>;

        // create the actual io queue
        auto queue = wdk::WDFQUEUE { nullptr };
        CustomLoggedTryOr(lj::win_call(wdk::WdfIoQueueCreate, device, &config, &attributes, &queue),
                          monadic::map(monadic::unwrap(), monadic::as<wdk::NTSTATUS>()),
                          lj::elog,
                          "Failed to create io queue!");

        // construct the device class into the raw memory that wdk::WDF allocated
        // via placement new
        auto queue_ctx = get_queue_context(queue);
        WDK_ASSERT_NTSTATUS(queue_ctx != nullptr);
        new (queue_ctx) queue_context { get_device_context(device), queue };

        return wdk::STATUS_SUCCESS;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto queue_context::io_control(wdk::WDFQUEUE   queue,
                                   wdk::WDFREQUEST request,
                                   usize           output_buffer_size,
                                   usize           input_buffer_size,
                                   u32             io_control_code,
                                   tag) noexcept -> void {
        auto request_completed = true;
        auto status            = wdk::NTSTATUS { wdk::STATUS_SUCCESS };

        auto _ = at_scope_exit_do([&request_completed, request, &status] noexcept {
            if (request_completed) wdk::WdfRequestComplete(request, status);
        });

        const auto update_status = [&status](auto&& error) noexcept { status = error.value(); };

        auto& device_ctx = *device_ctx_;

        // both write iocontrols funnel into the same USB pipe. without this guard usb_ctx() would
        // std::get a variant that still holds std::monostate -- throwing inside a noexcept function is
        // std::terminate inside the UMDF host, and every consumer that opens the device would hit it
        // whenever prepare_hardware failed (for example interface_count < 2).
        const auto write_report_or_refuse = [&](wdk::WDFREQUEST request) noexcept {
            if (not device_ctx.is_usb()) {
                wlog("{}: no USB transport attached!", lj::hid::ioctl_name(io_control_code));
                status = wdk::STATUS_INVALID_DEVICE_STATE;
                return;
            }

            // the feature mask that gates rumble is set once at D0Entry, and a reset by the HID stack
            // drops it silently. the next output report is the last moment before a consumer gives up
            // on rumble, so re-arm here -- and answer the report either way, because a failed re-arm is
            // not the consumer's fault.
            if (auto& usb = device_ctx.usb_ctx(); usb.needs_rearm()) {
                if (not usb.rearm_features()) wlog("Failed to re-arm the feature mask!");
            }

            // the bytes to send arrive in the request's input buffer. HidD_SetOutputReport -- what hidapi,
            // joy.cpl and Steam call for rumble -- is documented as "Send an output report to a HID device"
            // (hidsdi.h:272), so answering this ioctl without forwarding the payload means the controller
            // never hears about the rumble at all, however correct the vibration command path is.
            const auto buffer = get_wdf_request_memory(request);
            if (not buffer or stdr::size(*buffer) < hid::output_report_size) {
                wlog("{}: input buffer too small for an output report!", lj::hid::ioctl_name(io_control_code));
                status = wdk::STATUS_INVALID_PARAMETER;
                return;
            }

            if (const auto report_id = (*buffer)[0]; report_id != as<byte>(hid::output_report_id))
                wlog("{}: consumer sent report id {:#x}, the descriptor declares {:#x} -- sending anyway",
                     lj::hid::ioctl_name(io_control_code),
                     as<u8>(report_id),
                     as<u8>(hid::output_report_id));

            // bounded on purpose: the consumer gets a real answer about the controller rather than
            // "the USB queue accepted the request".
            CustomLoggedTryOr(device_ctx.usb_ctx().send_data_bounded(*buffer,
                                                                     transport::usb_context::endpoint_type::hid,
                                                                     output_report_send_timeout_ms),
                              update_status,
                              lj::elog,
                              "IOCTL Failed to send the output report!");
        };

        // METHOD_BUFFERED iocontrols (hidclass.h:141-149) carry a plain ULONG. these two helpers read and
        // answer one, refusing with a logged status instead of trusting the caller's buffer.
        const auto read_ulong = [&](wdk::WDFREQUEST request) noexcept -> std::optional<win32::ULONG> {
            const auto buffer = get_wdf_request_memory(request);
            if (not buffer or stdr::size(*buffer) < sizeof(win32::ULONG)) return std::nullopt;

            return *reinterpret_cast<const win32::ULONG*>(stdr::data(*buffer));
        };

        const auto write_ulong = [&](wdk::WDFREQUEST request, win32::ULONG value) noexcept {
            auto payload                                          = array<byte, sizeof(win32::ULONG)> {};
            *reinterpret_cast<win32::ULONG*>(stdr::data(payload)) = value;

            CustomLoggedTryOr(fill_wdf_request_memory(request, payload),
                              update_status,
                              lj::elog,
                              "IOCTL Failed to fill the output buffer!");
        };

        switch (io_control_code) {
            case wdk::IOCTL_HID_GET_DEVICE_DESCRIPTOR: {
                dlog("IOCTL_HID_GET_DEVICE_DESCRIPTOR");
                CustomLoggedTryOr(hid::ioctl::get_device_descriptor(request, device_ctx.hid_descriptor()),
                                  update_status,
                                  lj::elog,
                                  "IOCTL Failed to get device descriptor!");
            } break;

            case wdk::IOCTL_HID_GET_DEVICE_ATTRIBUTES: {
                dlog("IOCTL_HID_GET_DEVICE_ATTRIBUTES");
                CustomLoggedTryOr(hid::ioctl::get_device_attributes(request, device_ctx.hid_attributes()),
                                  update_status,
                                  lj::elog,
                                  "IOCTL Failed to get device attributes!");
            } break;

            case wdk::IOCTL_HID_GET_REPORT_DESCRIPTOR: {
                dlog("IOCTL_HID_GET_REPORT_DESCRIPTOR");
                CustomLoggedTryOr(hid::ioctl::get_report_descriptor(request, device_ctx.report_descriptor()),
                                  update_status,
                                  lj::elog,
                                  "IOCTL Failed to get report descriptor!");
            } break;

            case wdk::IOCTL_HID_GET_INPUT_REPORT: {
                dlog("IOCTL_HID_GET_INPUT_REPORT");

                // class-level twin of IOCTL_UMDF_HID_GET_INPUT_REPORT: answer it the same way instead
                // of refusing, otherwise non-UMDF readers see a device that cannot give back a report.
                request_completed = false;
                if (device_ctx.is_usb()) {
                    CustomLoggedTryOr(hid::ioctl::read_report(request, device_ctx.usb_ctx()),
                                      update_status,
                                      lj::elog,
                                      "OCTL Failed to read report!");
                    request_completed = true;
                }
            } break;

            case wdk::IOCTL_HID_READ_REPORT: {
                request_completed = false;
                if (device_ctx.is_usb()) {
                    CustomLoggedTryOr(hid::ioctl::read_report(request, device_ctx.usb_ctx()),
                                      update_status,
                                      lj::elog,
                                      "OCTL Failed to read report!");
                    request_completed = true;
                }
            } break;

            case wdk::IOCTL_HID_WRITE_REPORT: {
                dlog("IOCTL_HID_WRITE_REPORT");
                write_report_or_refuse(request);
            } break;

            case wdk::IOCTL_HID_GET_STRING: {
                // usb_ctx() is only valid once prepare_hardware succeeded, so the accessors below have
                // to check is_usb() first -- otherwise this is the same std::terminate hazard as the write
                // path.
                const auto& product_string = [&device_ctx] noexcept -> const string& {
                    if (device_ctx.is_usb() and device_ctx.usb_ctx().has_product_string())
                        return device_ctx.usb_ctx().product_string();

                    return device_ctx.product_string();
                }();

                const auto& serial_string = [&device_ctx] noexcept -> const string& {
                    if (device_ctx.is_usb() and device_ctx.usb_ctx().has_serial_string())
                        return device_ctx.usb_ctx().serial_string();

                    static constinit auto empty_serial = string { "NO SERIAL" };
                    return empty_serial;
                }();

                const auto& manufacturer_string = device_ctx.manufacturer_string();

                CustomLoggedTryOr(hid::ioctl::get_string(request, manufacturer_string, product_string, serial_string),
                                  update_status,
                                  lj::elog,
                                  "IOCTL Failed to get string!");
            } break;

            case wdk::IOCTL_HID_GET_MANUFACTURER_STRING: {
                dlog("IOCTL_HID_GET_MANUFACTURER_STRING");
                CustomLoggedTryOr(hid::ioctl::get_wide_string(request, device_ctx.manufacturer_string()),
                                  update_status,
                                  lj::elog,
                                  "IOCTL Failed to get manufacturer string!");
            } break;

            case wdk::IOCTL_HID_GET_PRODUCT_STRING: {
                dlog("IOCTL_HID_GET_PRODUCT_STRING");
                CustomLoggedTryOr(hid::ioctl::get_wide_string(request, device_ctx.product_string()),
                                  update_status,
                                  lj::elog,
                                  "IOCTL Failed to get product string!");
            } break;

            case wdk::IOCTL_HID_GET_SERIALNUMBER_STRING: {
                dlog("IOCTL_HID_GET_SERIALNUMBER_STRING");
                CustomLoggedTryOr(hid::ioctl::get_wide_string(request, device_ctx.serial_string()),
                                  update_status,
                                  lj::elog,
                                  "IOCTL Failed to get serial number string!");
            } break;

            case wdk::IOCTL_HID_GET_INDEXED_STRING: {
                const auto& product_string = [&device_ctx] noexcept -> const string& {
                    if (device_ctx.is_usb() and device_ctx.usb_ctx().has_product_string())
                        return device_ctx.usb_ctx().product_string();

                    return device_ctx.product_string();
                }();

                CustomLoggedTryOr(hid::ioctl::get_indexed_string(request, product_string),
                                  update_status,
                                  lj::elog,
                                  "IOCTL Failed to get indexed string!");
            } break;

            case wdk::IOCTL_HID_DEVICERESET_NOTIFICATION: {
                dlog("IOCTL_HID_DEVICERESET_NOTIFICATION");

                // the feature mask that gates rumble is set once at D0Entry, so a reset by the HID stack
                // silently drops it and rumble stops working with no log. remember it and re-arm lazily
                // on the next output report instead of refusing the notification.
                if (device_ctx.is_usb()) device_ctx.usb_ctx().mark_needs_rearm();
                status = wdk::STATUS_SUCCESS; // handled: lazily, on the next write
            } break;

            case wdk::IOCTL_HID_ACTIVATE_DEVICE: {
                // wlog("IOCTL_HID_ACTIVATE_DEVICE not supported!");
                // status = wdk::STATUS_NOT_IMPLEMENTED;
            } break;

            case wdk::IOCTL_HID_DEACTIVATE_DEVICE: {
                dlog("IOCTL_HID_DEACTIVATE_DEVICE");

                // asymmetric with IOCTL_HID_ACTIVATE_DEVICE above, which already succeeds: consumers such
                // as joy.cpl and hidapi deactivate on teardown, and a NOT_IMPLEMENTED here is noise they
                // cannot distinguish from a real failure.
                status = wdk::STATUS_SUCCESS;
            } break;

            case wdk::IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST: {
                wlog("IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST not supported!");
                status = wdk::STATUS_NOT_IMPLEMENTED;
            } break;

            case wdk::IOCTL_UMDF_GET_PHYSICAL_DESCRIPTOR: {
                wlog("IOCTL_UMDF_GET_PHYSICAL_DESCRIPTOR not supported");
                status = wdk::STATUS_NOT_IMPLEMENTED;
            } break;

            case wdk::IOCTL_UMDF_HID_GET_FEATURE: {
                // see IOCTL_HID_GET_FEATURE below: refusing is correct only while the report descriptor
                // declares no feature report at all.
                wlog("{}: no feature report is declared in the report descriptor", lj::hid::ioctl_name(io_control_code));
                status = wdk::STATUS_NOT_IMPLEMENTED;
            } break;

            case wdk::IOCTL_UMDF_HID_GET_INPUT_REPORT: {
                request_completed = false;
                if (device_ctx.is_usb()) {
                    CustomLoggedTryOr(hid::ioctl::read_report(request, device_ctx.usb_ctx()),
                                      update_status,
                                      lj::elog,
                                      "OCTL Failed to read report!");
                    request_completed = true;
                }
            } break;

            case wdk::IOCTL_UMDF_HID_SET_FEATURE: {
                // refusing keeps the mask we set at D0Entry authoritative: a consumer cannot turn rumble
                // off behind our back, but it also cannot turn it on for us. see IOCTL_HID_SET_FEATURE below.
                wlog("{}: no feature report is declared in the report descriptor", lj::hid::ioctl_name(io_control_code));
                status = wdk::STATUS_NOT_IMPLEMENTED;
            } break;

            case wdk::IOCTL_UMDF_HID_SET_OUTPUT_REPORT: {
                dlog("IOCTL_UMDF_HID_SET_OUTPUT_REPORT");
                write_report_or_refuse(request);
            } break;

            case wdk::IOCTL_HID_SET_OUTPUT_REPORT: {
                dlog("IOCTL_HID_SET_OUTPUT_REPORT");

                // METHOD_IN_DIRECT (hidclass.h:156): the report arrives in the input buffer, exactly like
                // the UMDF twin, so the same implementation answers both.
                write_report_or_refuse(request);
            } break;

            case wdk::IOCTL_HID_GET_OUTPUT_REPORT: {
                wlog("{}: nothing to echo back -- the driver keeps no copy of the last output report",
                     lj::hid::ioctl_name(io_control_code));
                status = wdk::STATUS_NOT_IMPLEMENTED;
            } break;

            case wdk::IOCTL_HID_GET_DRIVER_CONFIG: {
                dlog("IOCTL_HID_GET_DRIVER_CONFIG");

                auto config = wdk::HID_DRIVER_CONFIG { .Size           = sizeof(wdk::HID_DRIVER_CONFIG),
                                                       .RingBufferSize = device_ctx.ring_buffer_size() };

                CustomLoggedTryOr(fill_wdf_request_memory(request, bytes_of(config)),
                                  update_status,
                                  lj::elog,
                                  "IOCTL Failed to get driver config!");
            } break;

            case wdk::IOCTL_HID_SET_DRIVER_CONFIG: {
                dlog("IOCTL_HID_SET_DRIVER_CONFIG");

                const auto buffer = get_wdf_request_memory(request);
                if (not buffer or stdr::size(*buffer) < sizeof(wdk::HID_DRIVER_CONFIG)) {
                    wlog("IOCTL_HID_SET_DRIVER_CONFIG: input buffer too small!");
                    status = wdk::STATUS_INVALID_PARAMETER;
                    break;
                }

                const auto& incoming = *reinterpret_cast<const wdk::HID_DRIVER_CONFIG*>(stdr::data(*buffer));
                device_ctx.set_ring_buffer_size(incoming.RingBufferSize);
            } break;

            case wdk::IOCTL_HID_GET_POLL_FREQUENCY_MSEC: {
                dlog("IOCTL_HID_GET_POLL_FREQUENCY_MSEC");
                write_ulong(request, device_ctx.poll_frequency_msec());
            } break;

            case wdk::IOCTL_HID_SET_POLL_FREQUENCY_MSEC: {
                dlog("IOCTL_HID_SET_POLL_FREQUENCY_MSEC");

                const auto value = read_ulong(request);
                if (not value) {
                    wlog("IOCTL_HID_SET_POLL_FREQUENCY_MSEC: input buffer too small!");
                    status = wdk::STATUS_INVALID_PARAMETER;
                    break;
                }

                device_ctx.set_poll_frequency_msec(*value);
            } break;

            case wdk::IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS: {
                dlog("IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS");
                write_ulong(request, device_ctx.num_input_buffers());
            } break;

            case wdk::IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS: {
                dlog("IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS");

                const auto value = read_ulong(request);
                if (not value or *value == 0) {
                    wlog("IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS: invalid buffer count!");
                    status = wdk::STATUS_INVALID_PARAMETER;
                    break;
                }

                device_ctx.set_num_input_buffers(*value);
            } break;

            case wdk::IOCTL_HID_GET_COLLECTION_INFORMATION: {
                dlog("IOCTL_HID_GET_COLLECTION_INFORMATION");

                // this is how the HID-games stack and joy.cpl identify the device: without it the
                // controller is present but unclassifiable, so it never shows up as a gamepad.
                auto info = wdk::HID_COLLECTION_INFORMATION {
                    .DescriptorSize = 0, // no collection descriptor is served
                    .Polled         = win32::TRUE,
                    .Reserved1      = { 0 },
                    .VendorID       = device_ctx.vendor_id(),
                    .ProductID      = device_ctx.product_id(),
                    .VersionNumber  = 0x0002
                };

                CustomLoggedTryOr(fill_wdf_request_memory(request, bytes_of(info)),
                                  update_status,
                                  lj::elog,
                                  "IOCTL Failed to get collection information!");
            } break;

            case wdk::IOCTL_HID_ENABLE_WAKE_ON_SX: {
                dlog("IOCTL_HID_ENABLE_WAKE_ON_SX");

                const auto value = read_ulong(request);
                if (not value) {
                    wlog("IOCTL_HID_ENABLE_WAKE_ON_SX: input buffer too small!");
                    status = wdk::STATUS_INVALID_PARAMETER;
                    break;
                }

                device_ctx.set_wake_on_sx(*value != 0);
            } break;

            case wdk::IOCTL_HID_SET_S0_IDLE_TIMEOUT: {
                dlog("IOCTL_HID_SET_S0_IDLE_TIMEOUT");

                const auto value = read_ulong(request);
                if (not value) {
                    wlog("IOCTL_HID_SET_S0_IDLE_TIMEOUT: input buffer too small!");
                    status = wdk::STATUS_INVALID_PARAMETER;
                    break;
                }

                device_ctx.set_s0_idle_timeout(*value);
            } break;

            case wdk::IOCTL_HID_FLUSH_QUEUE: {
                dlog("IOCTL_HID_FLUSH_QUEUE");

                // HidD_FlushQueue: there is no queue of our own to drain -- the continuous reader keeps
                // only the last input report -- so acknowledge instead of refusing.
                status = wdk::STATUS_SUCCESS;
            } break;

            case wdk::IOCTL_HID_ENABLE_SECURE_READ: {
                dlog("IOCTL_HID_ENABLE_SECURE_READ");

                const auto value = read_ulong(request);
                if (not value) {
                    wlog("IOCTL_HID_ENABLE_SECURE_READ: input buffer too small!");
                    status = wdk::STATUS_INVALID_PARAMETER;
                    break;
                }

                device_ctx.set_secure_read(*value != 0);
            } break;

            case wdk::IOCTL_HID_DISABLE_SECURE_READ: {
                dlog("IOCTL_HID_DISABLE_SECURE_READ");
                device_ctx.set_secure_read(false);
            } break;

            case wdk::IOCTL_HID_GET_FEATURE:
            case wdk::IOCTL_HID_SET_FEATURE: {
                // honest refusal: the report descriptor declares no feature report, so the OS derives
                // HIDP_CAPS::FeatureReportByteLength == 0 and there is no legal buffer size to answer into.
                // supporting these means adding a feature report to the descriptor, not just a handler.
                wlog("{}: no feature report is declared in the report descriptor", lj::hid::ioctl_name(io_control_code));
                status = wdk::STATUS_NOT_IMPLEMENTED;
            } break;

            default: {
                // 24 iocontrols used to land here silently. when a game fails to see the controller there
                // is nothing to correlate, so name the code the consumer actually asked for.
                wlog("{} (code {:#x}) not handled", lj::hid::ioctl_name(io_control_code), io_control_code);
                status = wdk::STATUS_NOT_IMPLEMENTED;
            } break;
        }
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto queue_context::io_control(wdk::WDFQUEUE   queue,
                                   wdk::WDFREQUEST request,
                                   usize           output_buffer_size,
                                   usize           input_buffer_size,
                                   win32::ULONG    io_control_code) noexcept -> void {
        auto queue_ctx = get_queue_context(queue);
        WDK_EXPECTS_VOID(queue_ctx != nullptr);

        queue_ctx->io_control(queue, request, output_buffer_size, input_buffer_size, as<u32>(io_control_code), tag {});
    }
} // namespace lj
