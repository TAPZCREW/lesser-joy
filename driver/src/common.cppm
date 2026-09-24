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

        using command_report_buffer = array<byte, INPUT_REPORT_SIZE>;
        using input_report_buffer   = array<byte, INPUT_REPORT_SIZE>;
        using output_report_buffer  = array<byte, OUTPUT_REPORT_SIZE>;

        using Report_descriptor = array_view<const byte>;

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

            usize                    size;
            hid::input_report_buffer buffer;
        };

        struct continuous_reader {
            bool started = false;

            struct Sync {
                std::stop_source stop_source;
                std::stop_token  stop_token;

                std::mutex              input_report_mutex;
                std::condition_variable new_input_report_available;
            };

            heap_ptr<Sync> sync;

            std::vector<input_report> pending_input_reports = {};

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

            continuous_reader continuous_reader = {};
        };
    } // namespace usb

    namespace ble {
        struct context {};
    } // namespace ble

    struct device_context {
        WDFDEVICE device = nullptr;
        WDFQUEUE  default_queue;

        HID_DESCRIPTOR         hid_descriptor    = {};
        hid::Report_descriptor report_descriptor = {};
        HID_DEVICE_ATTRIBUTES  hid_attributes    = {};

        hid::output_report output_report = {};

        std::variant<std::monostate, usb::context, ble::context> transport = {};

        u16 vendor_id  = 0;
        u16 product_id = 0;

        string product_string = {};
        string serial_string  = {};

        // locked<std::queue<hid::Raw_input_report>> reports;
    };

    using Pdevice_context = device_context*;

    struct queue_context {
        WDFQUEUE queue = nullptr;

        device_context* device_ctx = nullptr;
    };

    using Pqueue_context = queue_context*;
} // namespace lj
