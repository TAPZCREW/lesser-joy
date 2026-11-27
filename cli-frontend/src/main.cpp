#include <stormkit/core/contract_macro.hpp>
#include <stormkit/core/try_expected.hpp>
#include <stormkit/main/main_macro.hpp>

import std;

import stormkit.core;
import stormkit.log;
import stormkit.platform.win32;
import stormkit.platform.win32.hid;
import stormkit.platform.win32.cfgmgr32;

import lj.cli.shell;
import lj.cli.log;
import lj.cli.hid;
import lj.cli.common;

using namespace stormkit;
using namespace std::literals;

namespace stdr = std::ranges;
namespace stdv = std::views;

namespace {
    constexpr auto exit_failure = []<typename T>(T&& error) static noexcept {
        if constexpr (meta::is<meta::to_plain_type<T>, lj::hid_status_code>) {
            lj::elog("Caught hid error {}!", lj::hid_error());
        } else
            lj::elog("Caught error {}!", error);
        return -1;
    };
} // namespace

auto main(array_view<const string_view> args) -> int {
    log::parse_args(args);

    auto logger = log::logger::create_logger_instance<log::console_logger>();

    lj::ilog("Searching for Switch 2 pro controller ...");

    auto entries            = dynarray<lj::hid_device_entry> {};
    auto showed_waiting_msg = false;
    do {
        TryInOr(entries, lj::enumerate_switch2_controllers(), exit_failure);
        if (stdr::empty(entries)) {
            if (not showed_waiting_msg) {
                lj::ilog("Not found! Waiting for Switch 2 pro controller connection ...");
                showed_waiting_msg = true;
            }
            std::this_thread::yield();
            std::this_thread::sleep_for(1s);
        }
    } while (stdr::empty(entries));

    lj::ilog("Found: ");
    auto i = 0u;
    for (const auto entry : entries) {
        lj::ilog("\n{} - {}\n    bus:              {}\n    serial number:    {}\n    release number:   {}\n    "
                 "interface number: {}\n    path:             {}",
                 green_text_style | i++,
                 entry.product_string,
                 yellow_text_style | entry.bus,
                 yellow_text_style | entry.serial_number,
                 yellow_text_style | entry.release_number,
                 yellow_text_style | entry.interface_number,
                 yellow_text_style | entry.path);
    }

    TryOr(lj::shell(std::move(entries)), exit_failure);

    return 0;
}
