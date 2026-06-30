#define WIN32_NO_STATUS
#include <stormkit/core/platform/windows.hpp>
#undef WIN32_NO_STATUS
#include <devpropdef.h>
#include <ntstatus.h>
#include <wdf.h>

import std;

import stormkit.core;
import stormkit.log;

import lesserjoy.log;
import lesserjoy.device;

using namespace stormkit;

extern "C" DRIVER_INITIALIZE DriverEntry;

auto logger = heap_ptr<lj::KernelLogger> {};

namespace lj {
    EVT_WDF_OBJECT_CONTEXT_CLEANUP event_driver_cleanup;
}

#pragma code_seg("INIT")

_Use_decl_annotations_ auto DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path) -> NTSTATUS {
    lj::ilog("Initializing lesserjoy driver...");

    auto attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = lj::event_driver_cleanup;

    auto config = WDF_DRIVER_CONFIG {};
    WDF_DRIVER_CONFIG_INIT(&config, lj::event_device_add);

    auto status = WdfDriverCreate(driver_object, registry_path, &attributes, &config, WDF_NO_HANDLE);
    if (not NT_SUCCESS(status)) lj::ilog("Failed to initialize lesserjoy driver! status: {:x}", status);
    else
        lj::ilog("lesserjoy: driver successfully initialized!");

    return status;
}

#pragma code_seg()

#pragma code_seg("PAGED")

namespace lj {
    auto event_driver_cleanup(_In_ WDFOBJECT driver) -> void {
        PAGED_CODE();

        lj::dlog("Cleanup up driver at {:#x}", std::bit_cast<uptr>(driver));
    }
} // namespace lj

#pragma code_seg()

extern "C" __declspec(dllexport) auto APIENTRY DllMain(HMODULE module, DWORD, LPVOID) -> BOOL {
    if (not logger) logger = log::Logger::allocate_logger_instance<lj::KernelLogger>();

    lj::ilog("Calling DllMain...");

    DisableThreadLibraryCalls(module);
    return TRUE;
}
