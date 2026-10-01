module;

#include "windows.hpp"

#include "usb.hpp"

export module lesserjoy.common;

import std;

import stormkit.core;

import lesserjoy.constants;

using namespace stormkit;

export namespace lj {
    using clock = std::chrono::high_resolution_clock;

    namespace hid {
        enum class output_report_source : u8 {
            DRIVER_HIGH_PRIORITY = 0,
            DRIVER_LOW_PRIORITY  = 1,
            DRIVER_XINPUTHID     = 2,
        };

        using command_report_buffer      = array<byte, INPUT_REPORT_SIZE>;
        using input_report_buffer        = array<byte, INPUT_REPORT_SIZE>;
        using input_report_buffer_scaled = array<byte, INPUT_REPORT_SIZE_SCALED>;
        using output_report_buffer       = array<byte, OUTPUT_REPORT_SIZE>;

        using report_descriptor = array_view<const byte>;

        struct input_report {
            input_report_buffer buffer = {};
            usize               size;
        };

        using output_report = output_report_buffer;
    } // namespace hid

    namespace usb {
        struct context;

        struct input_report {
            clock::time_point timestamp;

            usize                           size;
            hid::input_report_buffer_scaled buffer;
        };

        struct continuous_reader {
            // heap_ptr<std::mutex> mutex;
            // input_report         last_input_report;
            locked<input_report> last_input_report;
        };

        struct context {
            WDFUSBDEVICE device = nullptr;

            struct endpoint {
                WDFUSBINTERFACE interface = nullptr;

                WDFUSBPIPE in_pipe  = nullptr;
                WDFUSBPIPE out_pipe = nullptr;
            };

            endpoint hid;
            endpoint command;

            USB_DEVICE_DESCRIPTOR descriptor = {};

            WDFMEMORY product_string = nullptr;

            u16 min_x = 500;
            u16 max_x = 3500;
            u16 min_y = 500;
            u16 max_y = 3500;

            continuous_reader continuous_reader = {};
        };
    } // namespace usb

    namespace ble {
        struct context {};
    } // namespace ble
} // namespace lj
