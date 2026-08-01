module;

#include "windows.hpp"

#include "usb.hpp"

#include <stormkit/core/try_expected.hpp>

#undef move

module lesserjoy.usb;

import lesserjoy.device;
import lesserjoy.hid;
import lesserjoy.log;
import lesserjoy.constants;

namespace stdv = std::views;

namespace lj::usb {
    EVT_WDF_REQUEST_COMPLETION_ROUTINE    event_request_completion_routine;
    EVT_WDF_USB_READER_COMPLETION_ROUTINE event_usb_pipe_reader_complete;

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto wide_to_ascii(wstring_view input) -> string {
        [[maybe_unused]]
        auto state  = std::mbstate_t {};
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

        return output;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto init_context(Device_context& ctx, WDFDEVICE device) noexcept -> Expected<void> {
        // initialize usb context
        ctx.transport = usb::Context {};

        auto& usb = as<usb::Context>(ctx.transport);

        auto init_config = WDF_USB_DEVICE_CREATE_CONFIG {};
        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&init_config, USBD_CLIENT_CONTRACT_VERSION_602);
        LoggedTry(lj::win_call(WdfUsbTargetDeviceCreateWithParameters,
                               device,
                               &init_config,
                               WDF_NO_OBJECT_ATTRIBUTES,
                               &usb.device),
                  "Failed to create USB device context!");

        WdfUsbTargetDeviceGetDeviceDescriptor(usb.device, &usb.descriptor);

        ctx.vendor_id  = usb.descriptor.idVendor;
        ctx.product_id = usb.descriptor.idProduct;

        // get usb interface
        // TODO read about USB interfaces to ensure correct usage
        auto select_config = WDF_USB_DEVICE_SELECT_CONFIG_PARAMS {};
        WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(&select_config, 0, nullptr);

        LoggedTry(lj::win_call(WdfUsbTargetDeviceSelectConfig, usb.device, WDF_NO_OBJECT_ATTRIBUTES, &select_config),
                  "Failed to configure USB device!");

        usb.hid.interface     = WdfUsbTargetDeviceGetInterface(usb.device, 0);
        usb.command.interface = WdfUsbTargetDeviceGetInterface(usb.device, 1);

        // get product name if available
        const auto result = lj::win_call(WdfUsbTargetDeviceAllocAndQueryString,
                                         usb.device,
                                         WDF_NO_OBJECT_ATTRIBUTES,
                                         &usb.product_string,
                                         nullptr,
                                         usb.descriptor.iProduct,
                                         0x409);
        if (result.has_value()) {
            auto size          = 0_usize;
            auto memory_buffer = WdfMemoryGetBuffer(usb.product_string, &size);
            ctx.product_string = wide_to_ascii({ std::bit_cast<const wchar_t*>(memory_buffer), (size / sizeof(wchar_t)) });
        } else
            lj::wlog("Failed to get product string from USB device!\n    error: {}", result.error());

        // retrieve pipe handles for USB I/O
        auto endpoints = array { &usb.hid, &usb.command };
        for (const auto [i, endpoint_] : stdv::enumerate(endpoints)) {
            auto& endpoint = *endpoint_;
            for (auto pipe_index : range(WdfUsbInterfaceGetNumConfiguredPipes(endpoint.interface))) {
                auto pipe_info = WDF_USB_PIPE_INFORMATION {};
                WDF_USB_PIPE_INFORMATION_INIT(&pipe_info);

                const auto pipe = WdfUsbInterfaceGetConfiguredPipe(endpoint.interface, pipe_index, &pipe_info);

                // TODO investigate why this is necessery
                WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);

                dlog("Pipe {}, Type: {} In: {} Out: {}",
                     pipe_index,
                     narrow<u8>(pipe_info.PipeType),
                     WdfUsbTargetPipeIsInEndpoint(pipe),
                     WdfUsbTargetPipeIsOutEndpoint(pipe));

                const auto type = (i == 0) ? WdfUsbPipeTypeInterrupt : WdfUsbPipeTypeBulk;

                if (pipe_info.PipeType == type and WdfUsbTargetPipeIsInEndpoint(pipe)) endpoint.in_pipe = pipe;
                if (pipe_info.PipeType == type and WdfUsbTargetPipeIsOutEndpoint(pipe)) endpoint.out_pipe = pipe;
            }

            if (not endpoint.in_pipe or not endpoint.out_pipe) {
                elog("Failed to get endpoint pipes! in_pipe: {}, out_pipe: {}", endpoint.in_pipe, endpoint.out_pipe);
                Return std::unexpected<system_error2::nt_code> { STATUS_INVALID_DEVICE_STATE };
            }
        }

        auto config = WDF_USB_CONTINUOUS_READER_CONFIG {};

        // prepare continuous USB reader
        usb.continuous_reader.sync             = allocate_unsafe<Continuous_reader::Sync>();
        usb.continuous_reader.sync->stop_token = usb.continuous_reader.sync->stop_source.get_token();

        usb.continuous_reader.pending_input_reports.reserve(100);
        WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&config,
                                              event_usb_pipe_reader_complete,
                                              std::bit_cast<WDFCONTEXT>(&usb),
                                              hid::INPUT_REPORT_SIZE);
        CustomLoggedTry(lj::win_call(WdfUsbTargetPipeConfigContinuousReader, usb.hid.in_pipe, &config),
                        dlog,
                        "WdfUsbTargetPipeConfigContinuousReader failed!");
        Return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto event_device_entry(Device_context& ctx) noexcept -> Expected<void> {
        auto& usb = as<usb::Context>(ctx.transport);

        using enum hid::feature_select::Feature_flag;

        constexpr auto ENABLED_FEATURES
          // = BUTTON_STATE | ANALOG_STICKS;
          = narrow<hid::feature_select::Feature_flag>(0x27_u8);

        // start continuous USB reader
        auto io_target = WdfUsbTargetPipeGetIoTarget(usb.hid.in_pipe);
        LoggedTry(lj::win_call(WdfIoTargetStart, io_target), "Failed to start USB read pipe!");
        dlog("USB continuous reader started !");

        // send init sequence
        LoggedTry((hid::send_command_validate<hid::init::Initialize_usb_command<Transport::USB>>(usb)),
                  "Failed to initialize USB link!");
        LoggedTry((hid::send_command_validate<hid::feature_select::Set_feature_mask_command<Transport::USB>>(usb,
                                                                                                             ENABLED_FEATURES)),
                  "Failed to set feature mask!");
        LoggedTry((hid::send_command_validate<hid::feature_select::Enable_features_command<Transport::USB>>(usb,
                                                                                                            ENABLED_FEATURES)),
                  "Failed to enable features!");
        LoggedTry((hid::send_command_validate<hid::leds::Set_player_1_command<Transport::USB>>(usb)),
                  "Failed to setup player LED!");
        LoggedTry((hid::send_command_validate<
                    hid::init::Select_input_report_command<Transport::USB>>(usb, hid::init::Input_report_id::ALT_PROCON_2)),
                  "Failed to select input report!");

        ilog("{} initialized! (USB)", ctx.product_string);

        Return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto event_device_exit(Device_context& ctx) noexcept -> Expected<void> {
        auto& usb = as<usb::Context>(ctx.transport);

        // stop continueous reader
        usb.continuous_reader.sync->stop_source.request_stop();

        auto io_target = WdfUsbTargetPipeGetIoTarget(usb.command.in_pipe);
        WdfIoTargetStop(io_target, WdfIoTargetCancelSentIo);
        dlog("USB continuous reader stopped (command)!");

        io_target = WdfUsbTargetPipeGetIoTarget(usb.hid.in_pipe);
        WdfIoTargetStop(io_target, WdfIoTargetCancelSentIo);
        dlog("USB continuous reader stopped (hid)!");

        ilog("{} disconnected! (USB)", ctx.product_string);

        Return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto send_data(Context& ctx, array_view<const byte> payload) noexcept -> Expected<void> {
        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        auto target  = WdfUsbTargetDeviceGetIoTarget(ctx.device);
        auto request = WDFREQUEST {};
        CustomLoggedTry(lj::win_call(WdfRequestCreate, &attributes, target, &request), dlog, "WdfRequestCreate failed!");

        auto&& [memory, write_buffer] = CustomLoggedTry(wdf_memory_allocate(stdr::size(payload), request),
                                                        dlog,
                                                        "Failed to allocate memory for USB send payload!");
        stdr::copy(payload, stdr::begin(write_buffer));

        CustomLoggedTry(lj::win_call(WdfUsbTargetPipeFormatRequestForWrite, ctx.command.out_pipe, request, memory, WDF_NO_HANDLE),
                        dlog,
                        "WdfUsbTargetPipeFormatRequestForWrite failed!");

        WdfRequestSetCompletionRoutine(request, event_request_completion_routine, WDF_NO_HANDLE);

        if (WdfRequestSend(request, target, WDF_NO_SEND_OPTIONS) == FALSE) {
            const auto status = WdfRequestGetStatus(request);
            dlog("WdfRequestSend failed!\n    reason: {:#x}", narrow<cpp::ULong>(status));
            return std::unexpected<system_error2::nt_code> { std::in_place, status };
        }

        dlog("request {:#x}, {::#x} sent",
             request,
             array_view<const u8> { std::bit_cast<const u8*>(stdr::data(payload)), stdr::size(payload) });

        Return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto send_data_sync(const Context& usb, array_view<const byte> payload) noexcept -> Expected<void> {
        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        auto memory_descriptor = WDF_MEMORY_DESCRIPTOR {};
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memory_descriptor, std::bit_cast<PVOID>(stdr::data(payload)), stdr::size(payload));

        auto written = cpp::ULong { 0 };
        CustomLoggedTry(lj::win_call(WdfUsbTargetPipeWriteSynchronously,
                                     usb.command.out_pipe,
                                     nullptr,
                                     nullptr,
                                     &memory_descriptor,
                                     &written),
                        dlog,
                        "WdfUsbTargetPipeWriteSynchronously failed!");

        dlog("Sent {::#x}", array_view<const u8> { std::bit_cast<const u8*>(stdr::data(payload)), stdr::size(payload) });

        Return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_data_sync(const Context& usb) noexcept -> Expected<hid::Command_report_buffer> {
        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

        auto report = hid::Command_report_buffer {};

        auto memory_descriptor = WDF_MEMORY_DESCRIPTOR {};
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memory_descriptor, std::bit_cast<PVOID>(stdr::data(report)), stdr::size(report));

        auto readed = cpp::ULong { 0 };
        CustomLoggedTry(lj::win_call(WdfUsbTargetPipeReadSynchronously,
                                     usb.command.out_pipe,
                                     nullptr,
                                     nullptr,
                                     &memory_descriptor,
                                     &readed),
                        dlog,
                        "WdfUsbTargetPipeReadSynchronously failed!");

        dlog("Received {::#x}", array_view<const u8> { std::bit_cast<const u8*>(stdr::data(report)), stdr::size(report) });

        Return { std::move(report) };
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto send_control_request(const Context& ctx, byte request, byte value, byte index, array_view<const byte> data) noexcept
      -> Expected<void> {
        auto options = WDF_REQUEST_SEND_OPTIONS {};
        WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_TIMEOUT);

        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options, WDF_REL_TIMEOUT_IN_SEC(3));

        auto control_setup_packet = WDF_USB_CONTROL_SETUP_PACKET {};
        WDF_USB_CONTROL_SETUP_PACKET_INIT(&control_setup_packet,
                                          WDF_USB_BMREQUEST_DIRECTION::BmRequestHostToDevice,
                                          WDF_USB_BMREQUEST_RECIPIENT::BmRequestToDevice,
                                          narrow<u8>(request),
                                          narrow<u8>(value),
                                          narrow<u8>(index));

        auto memory_descriptor = WDF_MEMORY_DESCRIPTOR {};
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memory_descriptor, std::bit_cast<PVOID>(stdr::data(data)), stdr::size(data));

        CustomLoggedTry(lj::win_call(WdfUsbTargetDeviceSendControlTransferSynchronously,
                                     ctx.device,
                                     WDF_NO_HANDLE,
                                     &options,
                                     &control_setup_packet,
                                     &memory_descriptor,
                                     nullptr),
                        dlog,
                        "WdfUsbTargetDeviceSendControlTransferSynchronously failed!");

        Return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    _Use_decl_annotations_ auto event_usb_pipe_reader_complete(WDFUSBPIPE, WDFMEMORY memory, usize count, WDFCONTEXT data)
      -> void {
        if (data == nullptr or count == 0) return;

        auto& usb = *std::bit_cast<Context*>(data);
        auto& ctx = usb.continuous_reader;

        auto report = hid::Input_report_buffer {};
        CustomLoggedDiscardTryOr(get_wdf_memory(memory, report), monadic::noop(), dlog, "Failed to get USB data!");

        dlog("Received input report {::#x}", array_view<const u8> { std::bit_cast<const u8*>(stdr::data(report)), count });

        ctx.last_input_report.write([&report_ = report, count](auto& report) mutable noexcept {
            report = Input_report { Clock::now(), count, std::move(report_) };
        });
        // {
        //     auto  lock          = std::unique_lock { ctx.sync->input_report_mutex };
        //     auto& input_reports = ctx.pending_input_reports;
        //     input_reports.emplace_back(Clock::now(), count, std::move(report));

        //    stdr::sort(input_reports, [](const auto& first, const auto& second) static noexcept {
        //        return first.timestamp < second.timestamp;
        //    });
        // }

        // ctx.sync->new_input_report_available.notify_all();
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    _Use_decl_annotations_ auto event_request_completion_routine(WDFREQUEST request,
                                                                 WDFIOTARGET,
                                                                 PWDF_REQUEST_COMPLETION_PARAMS,
                                                                 WDFCONTEXT) -> void {
        dlog("request {:#x} completed", request);
        WdfObjectDelete(request);
    }
} // namespace lj::usb
