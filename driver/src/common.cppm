module;

#include "windows.hpp"

#include "usb.hpp"

export module lesserjoy.common;

import std;

import stormkit.core;

import lesserjoy.constants;

using namespace stormkit;

export namespace lj {
    using Clock = std::chrono::high_resolution_clock;

    namespace hid {
        enum class Output_report_source : u8 {
            DRIVER_HIGH_PRIORITY = 0,
            DRIVER_LOW_PRIORITY  = 1,
            DRIVER_XINPUTHID     = 2,
        };

        using Command_report_buffer = array<byte, INPUT_REPORT_SIZE>;
        using Input_report_buffer   = array<byte, INPUT_REPORT_SIZE>;
        using Output_report_buffer  = array<byte, OUTPUT_REPORT_SIZE>;

        using Report_descriptor = array_view<const byte>;

        struct Input_report {
            Input_report_buffer buffer = {};
            usize               size;
        };

        using Output_report = Output_report_buffer;
    } // namespace hid

    namespace usb {
        struct Context;

        struct Input_report {
            Clock::time_point timestamp;

            usize                    size;
            hid::Input_report_buffer buffer;
        };

        struct Continuous_reader {
            bool started = false;

            struct Sync {
                std::stop_source stop_source;
                std::stop_token  stop_token;

                std::mutex              input_report_mutex;
                std::condition_variable new_input_report_available;
            };

            heap_ptr<Sync> sync;

            std::vector<Input_report> pending_input_reports = {};

            Locked<Input_report> last_input_report;
        };

        struct Context {
            WDFUSBDEVICE device = nullptr;

            struct Endpoint {
                WDFUSBINTERFACE interface = nullptr;

                WDFUSBPIPE in_pipe  = nullptr;
                WDFUSBPIPE out_pipe = nullptr;
            };

            Endpoint hid;
            Endpoint command;

            USB_DEVICE_DESCRIPTOR descriptor = {};

            WDFMEMORY product_string = nullptr;

            Continuous_reader continuous_reader = {};
        };
    } // namespace usb

    namespace ble {
        struct Context {};
    } // namespace ble

    struct Device_context {
        WDFDEVICE device = nullptr;
        WDFQUEUE  default_queue;

        HID_DESCRIPTOR         hid_descriptor    = {};
        hid::Report_descriptor report_descriptor = {};
        HID_DEVICE_ATTRIBUTES  hid_attributes    = {};

        hid::Output_report output_report = {};

        std::variant<std::monostate, usb::Context, ble::Context> transport = {};

        u16 vendor_id  = 0;
        u16 product_id = 0;

        string product_string = {};
        string serial_string  = {};

        // Locked<std::queue<hid::Raw_input_report>> reports;
    };

    using PDevice_context = Device_context*;

    struct Queue_context {
        WDFQUEUE queue = nullptr;

        Device_context* device_ctx = nullptr;
    };

    using PQueue_context = Queue_context*;
} // namespace lj
