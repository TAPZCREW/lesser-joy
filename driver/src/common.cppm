export module lesserjoy:common;

import std;

import stormkit.core;

import wdk.wdf;
import wdk.usb;

import :constants;

using namespace stormkit;

export namespace lj {
    using clock = std::chrono::high_resolution_clock;

    namespace hid {
        enum class output_report_source : u8 {
            driver_high_priority = 0,
            driver_low_priority  = 1,
            driver_xinputhid     = 2,
        };

        using command_report_buffer = array<byte, input_report_size>;
        using input_report_buffer   = array<byte, input_report_size>;

        using report_descriptor = array_view<const byte>;

        struct input_report {
            input_report_buffer buffer = {};
            usize               size   = 0u;
        };
    } // namespace hid

    namespace usb {
        struct context;

        struct input_report {
            clock::time_point timestamp;

            usize                    size = 0u;
            hid::input_report_buffer buffer;
        };

        struct context {
            wdk::WDFUSBDEVICE device = nullptr;

            struct endpoint {
                wdk::WDFUSBINTERFACE interface = nullptr;

                wdk::WDFUSBPIPE in_pipe  = nullptr;
                wdk::WDFUSBPIPE out_pipe = nullptr;
            };

            endpoint hid;
            endpoint command;

            wdk::USB_DEVICE_DESCRIPTOR descriptor = {};

            wdk::WDFMEMORY product_string = nullptr;

            u16 min_x = 500;
            u16 max_x = 3500;
            u16 min_y = 500;
            u16 max_y = 3500;

            struct {
                locked<input_report> last_input_report;
            } continuous_reader = {};
        };
    } // namespace usb

    namespace ble {
        struct context {};
    } // namespace ble
} // namespace lj
