module;

#define WIN32_NO_STATUS
#include <stormkit/core/platform/windows.hpp>
#undef WIN32_NO_STATUS
#include <devpropdef.h>
#include <ntstatus.h>
#include <wdf.h>

export module lesserjoy.entrypoint;

import std;

import stormkit.core;
import stormkit.log;

import lesserjoy.log;

using namespace stormkit;

module: private;

extern "C" DRIVER_INITIALIZE DriverEntry;

namespace lj {
    EVT_WDF_DRIVER_DEVICE_ADD event_device_add;
}

auto logger = Heap<lj::KernelLogger> {};

extern "C" _Use_decl_annotations_ auto DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path)
  -> NTSTATUS {
    OutputDebugStringA("[lesserjoy] calling DriverEntry\n");

    logger = log::Logger::allocate_logger_instance<lj::KernelLogger>();
    lj::ilog("Initializing lesserjoy driver...");

    auto attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    auto config = WDF_DRIVER_CONFIG {};
    WDF_DRIVER_CONFIG_INIT(&config, lj::event_device_add);

    auto status = WdfDriverCreate(driver_object, registry_path, &attributes, &config, WDF_NO_HANDLE);
    if (not NT_SUCCESS(status)) lj::ilog("Failed to initialize lesserjoy driver! status: {:x}", status);
    else
        lj::ilog("lesserjoy: driver successfully initialized!");

    return status;
}

// extern "C" _Use_decl_annotations_ auto APIENTRY DllMain(HMODULE module, DWORD ul_reason_for_call, LPVOID) -> BOOL {

//     DisableThreadLibraryCalls(module);

//     logger->write(log::Severity::INFO, log::Module {}, "lesserjoy starting ...");

//     return TRUE;
// }
