module;

#include "contract_macro.hpp"
#include <stormkit/core/try_expected.hpp>

export module lesserjoy;

import std;

import stormkit.core;
import stormkit.platform.win32;
import stormkit.log;

import wdk.wdf;
import wdk.ntstatus;

import :device;
import :log;
import :wdf;

module: private;

using namespace stormkit;

namespace lj {
    ////////////////////////////////////////
    ////////////////////////////////////////
    auto __stdcall event_driver_cleanup(wdk::WDFOBJECT driver) -> void {
        lj::dlog("Driver successfully cleanup up!");
    }
} // namespace lj

auto logger = heap_ptr<lj::kernel_logger> {};

////////////////////////////////////////
////////////////////////////////////////
extern "C" __declspec(dllexport) auto __stdcall DllMain(win32::HMODULE module, win32::DWORD, win32::LPVOID) -> win32::BOOL {
    win32::DisableThreadLibraryCalls(module);

    return win32::TRUE;
}

#pragma code_seg("INIT")

////////////////////////////////////////
////////////////////////////////////////
extern "C" auto __stdcall DriverEntry(wdk::PDRIVER_OBJECT driver_object, wdk::PUNICODE_STRING registry_path) -> wdk::NTSTATUS {
    if (not logger) {
        logger = log::logger::allocate_logger_instance<lj::kernel_logger>();
        WDK_EXPECTS_NTSTATUS(logger != nullptr);
    }

    static constexpr auto severity_mask = log::severity::info
                                          | log::severity::error
                                          | log::severity::fatal
                                          | log::severity::warning
#ifdef LESSERJOY_DEBUG_MODE
                                          | log::severity::debug
#endif
      ;
    logger->set_severity_mask(severity_mask);

    lj::ilog("Initializing lesserjoy driver...");

    auto attributes = wdk::WDF_OBJECT_ATTRIBUTES {};
    wdk::WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = lj::event_driver_cleanup;

    auto config = wdk::WDF_DRIVER_CONFIG {};
    wdk::WDF_DRIVER_CONFIG_INIT(&config, lj::device_context::create);

    CustomLoggedTryOr(lj::win_call(wdk::WdfDriverCreate, driver_object, registry_path, &attributes, &config, wdk::WDF_NO_HANDLE),
                      monadic::unwrap(),
                      lj::elog,
                      "Failed to initialize lessjoy driver! {}");

    lj::ilog("Driver successfully initialized!");

    return 0;
}

#pragma code_seg()
