
#include <simpleble/SimpleBLE.h>
#include <stormkit/core/try_expected.hpp>

import std;
import frozen;
import stormkit.core;

namespace stk  = stormkit;
namespace stdr = std::ranges;

using hrclock = std::chrono::high_resolution_clock;
using namespace std::chrono_literals;
using namespace stk::literals;

enum class BLEException {
    NO_ADAPTER,
    BLUETOOTH_DISABLED,
    DEVICE_NOT_FOUND,
    NOT_CONNECTED,
    NOT_CONNECTABLE,
    SERVICE_NOT_FOUND,
    CHARACTERISTIC_NOT_FOUND,
    DESCRIPTOR_NOT_FOUND,
    OPERATION_NOT_SUPPORTED,
    OPERATION_FAILED,
    RESPONSE_TIMEOUT,
    RESPONSE_UNEXPECTED,
    WINRT_EXCEPTION,
    CORE_BLUETOOTH_EXCEPTION,
    UNKNOWN_EXCEPTION
};

constexpr auto KNOWN_ADDRESSES = std::array {
    SimpleBLE::BluetoothAddress{ "98:e2:55:bd:9f:20" },
};

auto service_name(SimpleBLE::BluetoothUUID uuid) -> std::optional<std::string_view> {
    if (uuid == SimpleBLE::BluetoothUUID { "00001800-0000-1000-8000-00805f9b34fb" }) {
        return "GAP";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "00001801-0000-1000-8000-00805f9b34fb" }) {
        return "GATT";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "00c5af5d-1964-4e30-8f51-1956f96bd280" }) {
        return "Service Control";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "ab7de9be-89fe-49ad-828f-118f09df7fd0" }) {
        return "Nintendo SW2";
    }

    return std::nullopt;
}

auto characteristic_name(SimpleBLE::BluetoothUUID uuid) -> std::optional<std::string_view> {
    // see https://github.com/ndeadly/switch2_controller_research/blob/master/bluetooth_interface.md
    if (uuid == SimpleBLE::BluetoothUUID { "00002a00-0000-1000-8000-00805f9b34fb" }) {
        return "Device Name";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "00002a01-0000-1000-8000-00805f9b34fb" }) {
        return "Appearance";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "00c5af5d-1964-4e30-8f51-1956f96bd282" }) {
        return "Service Enable";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "3dacbc7e-6955-40b5-8eaf-6f9809e8b379" }) {
        return "(Pro) HD Rumble + Command";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "4147423d-fdae-4df7-a4f7-d23e5df59f8d" }) {
        return "Large Command / Firmware Update";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "506d9f7d-4278-4e95-a549-326ba77657e0" }) {
        return "(Pro) Extended Command Response";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "649d4ac9-8eb7-4e6c-af44-1ea54fe5f005" }) {
        return "Command";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "ab7de9be-89fe-49ad-828f-118f09df7fd2" }) {
        return "HID Input";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "7492866c-ec3e-4619-8258-32755ffcc0f8" }) {
        return "(Pro) HID Input";
    }
    // if (uuid == SimpleBLE::BluetoothUUID { "ab7de9be-89fe-49ad-828f-118f09df7fde" }) {
    //     return "Unknown";
    // }
    // if (uuid == SimpleBLE::BluetoothUUID { "ab7de9be-89fe-49ad-828f-118f09df7fdf" }) {
    //     return "Unknown";
    // }
    if (uuid == SimpleBLE::BluetoothUUID { "c765a961-d9d8-4d36-a20a-5315b111836a" }) {
        return "Command Response";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "cc483f51-9258-427d-a939-630c31f72b05" }) {
        return "(Pro) HID Output";
    }
    // if (uuid == SimpleBLE::BluetoothUUID { "d3bd69d2-841c-4241-ab15-f86f406d2a80" }) {
    //     return "Unknown";
    // }
    if (uuid == SimpleBLE::BluetoothUUID { "7492866c-ec3e-4619-8258-32755ffcc0f9" }) {
        return "(Pro) Headset Audio + HID Input";
    }

    return std::nullopt;
}

auto descriptor_name(SimpleBLE::BluetoothUUID uuid) -> std::optional<std::string_view> {
    if (uuid == SimpleBLE::BluetoothUUID { "00002902-0000-1000-8000-00805f9b34fb" }) {
        return "Client Characteristic Configuration";
    }
    if (uuid == SimpleBLE::BluetoothUUID { "679d5510-5a24-4dee-9557-95df80486ecb" }) {
        return "Set report rate?";
    }
    return std::nullopt;
}

auto show(SimpleBLE::Safe::Peripheral controller) -> void {
    std::println("[{}] {} (type: {})",
                 controller.identifier().value_or("unknown"),
                 controller.address().value_or("unknown"),
                 static_cast<int>(controller.address_type().value_or(SimpleBLE::BluetoothAddressType::UNSPECIFIED)));

    std::println("    RSSI: {}, Tx Power: {} dBm, MTU: {}",
                 controller.rssi().value_or(0),
                 controller.tx_power().value_or(0),
                 controller.mtu().value_or(0));

    std::println("    Paired: {}, Connected: {}",
                 controller.is_paired().value_or(false),
                 controller.is_connected().value_or(false));

    if (auto manufacturer_data = controller.manufacturer_data(); manufacturer_data and not manufacturer_data->empty())
    for (auto& [manufacturer_id, data] : *manufacturer_data) {
        std::println("    Manufacturer ID: {}", manufacturer_id);
        std::println("    Manufacturer data: {}", data);
    }
    else
        std::println("    No manufacturer data");

    // std::println("(S): Service, (C): Characteristic, (D): Descriptor");

    if (auto services = controller.services(); services and not services->empty())
    for (auto& service : *services) {
        if (auto name = service_name(service.uuid()); name) {
            std::println("    {}", *name);
        } else {
            std::println("    (S) {}", service.uuid());
        }

        std::println("        -> {}", service.data());
        for (auto& characteristic : service.characteristics()) {
            if (auto name = characteristic_name(characteristic.uuid()); name) {
                std::println("        {} {}", *name, characteristic.capabilities());
            }
            else {
                std::println("        (C) {} {}", characteristic.uuid(), characteristic.capabilities());
            }

            if (characteristic.can_read()) {
                if (auto data = controller.read(service.uuid(), characteristic.uuid()); data) {
                    if (characteristic.uuid() == SimpleBLE::BluetoothUUID { "00002a00-0000-1000-8000-00805f9b34fb" }) {
                        std::println("            -> {}", *data | stdr::to<std::string>());
                    }
                    else {
                        std::println("            -> {}", *data);
                    }
                }
            }

            for (auto& descriptor : characteristic.descriptors()) {
                if (auto name = descriptor_name(descriptor.uuid()); name) {
                    std::println("            {}", *name);
                }
                else {
                    std::println("            (D) {}", descriptor.uuid());
                }

                if (characteristic.can_read()) {
                    if (auto data = controller.read(service.uuid(), characteristic.uuid(), descriptor.uuid()); data) {
                        std::println("                -> {}", *data);
                    }
                }
            }
        }
    }
    else
        std::println("    No service available");
}

auto get_adapter() -> std::expected<SimpleBLE::Safe::Adapter, BLEException> {
    if (not SimpleBLE::Safe::Adapter::bluetooth_enabled().value_or(false)) {
        return std::unexpected{ BLEException::BLUETOOTH_DISABLED };
    }

    auto adapters = SimpleBLE::Safe::Adapter::get_adapters();
    if (not adapters or adapters->empty()) {
        return std::unexpected{ BLEException::NO_ADAPTER };
    }

    return adapters->at(0);
}

auto find_controller(auto scan_duration) -> std::expected<SimpleBLE::Safe::Peripheral, BLEException> {
    auto adapter = Try(get_adapter());
    std::println("Using adapter: {} [{}]", adapter.identifier().value_or(""), adapter.address().value_or("unknown"));

    auto controller = std::optional<SimpleBLE::Safe::Peripheral>{};

    // adapter.set_callback_on_scan_found([adapter, controller] (SimpleBLE::Safe::Peripheral peripheral){
    //     if (auto a = peripheral.address(); a and stdr::contains(KNOWN_ADDRESSES, *a)) {
    //         controller = peripheral;
    //         adapter.scan_stop();
    //     }
    // });

    std::println("Scanning devices for {}...", scan_duration);
    adapter.scan_for(std::chrono::duration_cast<std::chrono::milliseconds>(scan_duration).count());

    auto controllers = adapter.scan_get_results();
    if (not controllers or controllers->empty()) {
        return std::unexpected{ BLEException::DEVICE_NOT_FOUND };
    }

    // TODO use stdr algorithms, or even better use adapter's scan callbacks
    for (auto peripheral : *controllers) {
        if (auto a = peripheral.address(); a and stdr::contains(KNOWN_ADDRESSES, *a)) {
            controller = peripheral;
            break;
        }
    }

    if (not controller) {
        return std::unexpected{ BLEException::DEVICE_NOT_FOUND };
    }

    return *controller;
}

auto connect(SimpleBLE::Safe::Peripheral controller) -> std::expected<void, BLEException> {
    if (controller.is_connected().value_or(false)) {
        return {};
    }

    if (not controller.is_connectable().value_or(false)) {
        return std::unexpected{ BLEException::NOT_CONNECTABLE };
    }

    if (not controller.connect()) {
        return std::unexpected{ BLEException::NOT_CONNECTED };
    }

    return {};
}

auto disconnect(SimpleBLE::Safe::Peripheral controller) -> std::expected<void, BLEException> {
    if (not controller.is_connected().value_or(false)) {
        return {};
    }

    if (not controller.disconnect()) {
        return std::unexpected{ BLEException::UNKNOWN_EXCEPTION };
    }

    return {};
}

auto enable_sw2(SimpleBLE::Safe::Peripheral controller) -> std::expected<void, BLEException> {
    if (not controller.is_connected().value_or(false)) {
        return std::unexpected{ BLEException::NOT_CONNECTED };
    }

    if (not controller.write_request("00c5af5d-1964-4e30-8f51-1956f96bd280",
                                     "00c5af5d-1964-4e30-8f51-1956f96bd282",
                                     { 0x01, 0x00 })
    ) {
        return std::unexpected{ BLEException::OPERATION_FAILED };
    }

    return {};
}

auto enable_command_response(SimpleBLE::Safe::Peripheral controller) -> std::expected<void, BLEException> {
    if (not controller.is_connected().value_or(false)) {
        return std::unexpected{ BLEException::NOT_CONNECTED };
    }

    if (not controller.write("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                             "506d9f7d-4278-4e95-a549-326ba77657e0",
                             "00002902-0000-1000-8000-00805f9b34fb",
                             { 0x01, 0x00 })
    ) {
        return std::unexpected{ BLEException::OPERATION_FAILED };
    }

    return {};
}

auto notify_command_response(SimpleBLE::Safe::Peripheral controller, std::function<void(SimpleBLE::ByteArray)> cb) -> std::expected<void, BLEException> {
    if (not controller.is_connected().value_or(false)) {
        return std::unexpected{ BLEException::NOT_CONNECTED };
    }

    if (not controller.notify("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                              "506d9f7d-4278-4e95-a549-326ba77657e0",
                              cb)
    ) {
        return std::unexpected{ BLEException::OPERATION_FAILED };
    }

    return {};
}

auto unnotify_command_response(SimpleBLE::Safe::Peripheral controller) -> std::expected<void, BLEException> {
    if (not controller.unsubscribe("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                                   "506d9f7d-4278-4e95-a549-326ba77657e0")
    ) {
        return std::unexpected{ BLEException::OPERATION_FAILED };
    }

    return {};
}

auto send_command(SimpleBLE::Safe::Peripheral controller, SimpleBLE::ByteArray bytes) -> std::expected<void, BLEException> {
    if (not controller.write_command("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                                     "649d4ac9-8eb7-4e6c-af44-1ea54fe5f005",
                                     bytes)
    ) {
        return std::unexpected{ BLEException::OPERATION_FAILED };
    }

    return {};
}

auto subscribe_input(SimpleBLE::Safe::Peripheral controller, auto cb, auto rate = 0x8500) -> std::expected<void, BLEException> {
    if (not controller.is_connected().value_or(false)) {
        return std::unexpected{ BLEException::NOT_CONNECTED };
    }

    // if (not controller.write("ab7de9be-89fe-49ad-828f-118f09df7fd0",
    //                          "7492866c-ec3e-4619-8258-32755ffcc0f8",
    //                          "679d5510-5a24-4dee-9557-95df80486ecb",
    //                          { static_cast<std::uint8_t>(rate >> 8), static_cast<std::uint8_t>(rate >> 0) })
    // ) {
    //     return std::unexpected{ BLEException::OPERATION_FAILED };
    // }

    if (not controller.write("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                             "7492866c-ec3e-4619-8258-32755ffcc0f8",
                             "00002902-0000-1000-8000-00805f9b34fb",
                             { 0x01, 0x00 })
    ) {
        return std::unexpected{ BLEException::OPERATION_FAILED };
    }

    if (not controller.notify("ab7de9be-89fe-49ad-828f-118f09df7fd0",
                              "7492866c-ec3e-4619-8258-32755ffcc0f8",
                              cb)
    ) {
        return std::unexpected{ BLEException::OPERATION_FAILED };
    }

    return {};
}

template <class Callback>
struct Defered { Callback cb; ~Defered() { cb(); } };

// struct Notifier {
//     static auto Default(auto controller, auto uids, auto cb) -> std::expected<Notifier, BLEException> {
//         if (not controller.notify(std::get<0>(uids), std::get<1>(uids), cb)) {
//             return std::unexpected{ BLEException::OPERATION_FAILED };
//         }

//         return { controller, uids };
//     }

//     ~Notifier() {
//         controller.unsubscribe(std::get<0>(uids), std::get<1>(uids))
//     }

// private:
//     SimpleBLE::Safe::Peripheral controller;
//     std::tuple<SimpleBLE::BluetoothUUID, SimpleBLE::BluetoothUUID> uids;
// };

// struct CommandEndpoint {
//     static auto Default(auto controller) -> std::expected<CommandEndpoint, BLEException> {
//         TryDiscard(enable_command_response(controller));

//         auto command_response = Try(Notifier::Default(
//             controller,
//             { "ab7de9be-89fe-49ad-828f-118f09df7fd0", "506d9f7d-4278-4e95-a549-326ba77657e0" },
//             std::bind_front
//         ));

//         return {
//             controller,
//             command_response,
//         };
//     }

// private:
//     SimpleBLE::Safe::Peripheral controller;
//     Notifier command_response;
//     std::optional<SimpleBLE::ByteArray> response;
// };

auto initialize(SimpleBLE::Safe::Peripheral controller) -> std::expected<void, BLEException> {
    TryDiscard(enable_sw2(controller));
    TryDiscard(enable_command_response(controller));

    auto response = std::optional<SimpleBLE::ByteArray>{};
    TryDiscard(notify_command_response(controller, [&response](auto bytes) {
        // std::println("Command response: {:n:02x}", bytes);
        response = bytes;
    }));
    auto unsubscribe = Defered{[controller] {
        unnotify_command_response(controller);
    }};
    auto wait_response = [&response](auto timeout, auto sleep_interval) {
        auto start = hrclock::now();
        while (not response
               and std::chrono::duration_cast<decltype(timeout)>(hrclock::now() - start) < timeout) {
            std::this_thread::sleep_for(sleep_interval);
        }
    };
    auto send_command_expect_response = [controller, &response, &wait_response](SimpleBLE::ByteArray command, SimpleBLE::ByteArray expected) -> std::expected<void, BLEException> {
        response = std::nullopt;
        TryDiscard(send_command(controller,  command));
        if (wait_response(1s, 1ms); not response) {
            std::println("Timeout response!");
            return std::unexpected{ BLEException::RESPONSE_TIMEOUT };
        }
        if (not stdr::equal(*response | stdr::views::drop_while([](auto b) { return b == 0; }), expected)) {
            std::println("Expected response: {:n:02x}", expected);
            return std::unexpected{ BLEException::RESPONSE_UNEXPECTED };
        }

        return {};
    };

    // Initialize
    // TryDiscard(send_command_expect_response({ 0x07, 0x91, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 },
    //                                         { 0x07, 0x01, 0x01, 0x01, 0x10, 0x78, 0x00, 0x00, 0x00 }));

    // TryDiscard(send_command(controller, { 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x30, 0x01, 0x00 })); // Flash memory - Read 0x40 bytes from 0x00013000
    // //         expected:                { 0x02, 0x01, 0x01, 0x04, 0x10, 0x78, 0x00, 0x00, 0x00 }

    // TryDiscard(send_command(controller, { 0x16, 0x91, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00 })); // Unknown
    // TryDiscard(send_command(controller, { 0x15, 0x91, 0x01, 0x01, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x02, 0x81, 0xeb, 0x3a, 0xeb, 0xf1, 0x48, 0x80, 0xeb, 0x3a, 0xeb, 0xf1, 0x48 })); // Pairing
    // TryDiscard(send_command(controller, { 0x15, 0x91, 0x01, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00, 0x35, 0x03, 0xe9, 0x29, 0x82, 0x87, 0x71, 0x24, 0xbe, 0xa8, 0x0c, 0x66, 0x46, 0x15, 0x83, 0x4b })); // Pairing
    // TryDiscard(send_command(controller, { 0x15, 0x01 /* ? */, 0x01, 0x02, 0x10, 0x78, 0x00, 0x00, 0x01, 0x13, 0x4c, 0x97, 0xf5, 0x11, 0xb9, 0xb6, 0xdd, 0x4d, 0x86, 0xfd, 0x40, 0xf5, 0x36, 0xe9, 0xed })); // Pairing
    // TryDiscard(send_command(controller, { 0x15, 0x91, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00 })); // Pairing

     // Vibration Sample
    TryDiscard(send_command_expect_response({ 0x0a, 0x91, 0x01, 0x02, 0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 },
                                            { 0x0a, 0x01, 0x01, 0x02, 0x10, 0x78, 0x00, 0x00 }));
    // Set LED pattern
    TryDiscard(send_command_expect_response({ 0x09, 0x91, 0x01, 0x07, 0x00, 0x08, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                                            { 0x09, 0x01, 0x01, 0x07, 0x10, 0x78, 0x00, 0x00 }));
    
    // Enable Feature Mask
    TryDiscard(send_command_expect_response({ 0x0c, 0x91, 0x01, 0x02, 0x00, 0x04, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00 },
                                            { 0x0c, 0x01, 0x01, 0x02, 0x10, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

    // TryDiscard(send_command(controller, { 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x40, 0x7e, 0x00, 0x00, 0x80, 0x30, 0x01, 0x00 })); // Flash memory - Read 0x40 bytes from 0x00013080
    // TryDiscard(send_command(controller, { 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x40, 0x7e, 0x00, 0x00, 0xc0, 0x30, 0x01, 0x00 })); // Flash memory - Read 0x40 bytes from 0x000130c0
    // TryDiscard(send_command(controller, { 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x40, 0x7e, 0x00, 0x00, 0x40, 0xc0, 0x1f, 0x00 })); // Flash memory - Read 0x40 bytes from 0x001fc040
    // TryDiscard(send_command(controller, { 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x10, 0x7e, 0x00, 0x00, 0x40, 0x30, 0x01, 0x00 })); // Flash memory - Read 0x10 bytes from 0x00013040
    // TryDiscard(send_command(controller, { 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x18, 0x7e, 0x00, 0x00, 0x00, 0x31, 0x01, 0x00 })); // Flash memory - Read 0x18 bytes from 0x00013100
    // TryDiscard(send_command(controller, { 0x11, 0x91, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00 })); // Unknown
    // TryDiscard(send_command(controller, { 0x02, 0x91, 0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x20, 0x7e, 0x00, 0x00, 0x60, 0x30, 0x01, 0x00 })); // Flash memory - Read 0x20 bytes from 0x00013060

     // Vibration Data
    // TryDiscard(send_command_expect_response({ 0x0a, 0x91, 0x01, 0x08, 0x00, 0x14, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x35, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    //                                         { 0x0a, 0x01, 0x01, 0x08, 0x10, 0x78, 0x00, 0x00 }));

    // Enable Feature Flags
    TryDiscard(send_command_expect_response({ 0x0c, 0x91, 0x01, 0x04, 0x00, 0x04, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00 },
                                            { 0x0c, 0x01, 0x01, 0x04, 0x10, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

    return {};
}

auto animate() -> void {
    for (;;) {
        std::this_thread::sleep_for(1s);
    }
}

auto run_app() -> std::expected<void, BLEException> {
    auto controller = Try(find_controller(2s));
    TryDiscard(connect(controller));
    auto discon = Defered{[controller]{
        disconnect(controller);
    }};
    std::println("Connected: {} [{}]", controller.identifier().value_or(""), controller.address().value_or("unknown"));

    TryDiscard(initialize(controller));
    std::println("Initialized.");

    // show(controller);

    TryDiscard(subscribe_input(controller, [](auto bytes) {
        if (not stdr::empty(bytes) and bytes[0] % 020 != 0) {
            return;
        }

        std::println("HID Input: {:n:02x} ...", bytes);
    }, 0xffff));
    std::println("Subscribed.");

    for (;;) {
        std::this_thread::sleep_for(1s);
    }

    return {};
}

auto main() -> int {
    if (auto res = run_app(); not res)
        return static_cast<int>(res.error());
    return EXIT_SUCCESS;
}
