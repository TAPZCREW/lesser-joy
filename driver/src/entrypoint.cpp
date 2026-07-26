#include "windows.hpp"

import std;

import stormkit.core;
import stormkit.log;

import lesserjoy.log;
import lesserjoy.device;
import lesserjoy.ntstatus;

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

    auto result = lj::win_call(WdfDriverCreate, driver_object, registry_path, &attributes, &config, nullptr);
    if (not result) {
        lj::elog("Failed to initialize lessjoy driver! status: {}", result.error());
        return result.error().value();
    }

    lj::ilog("driver successfully initialized!");

    return 0;
}

#pragma code_seg()

#pragma code_seg("PAGED")

namespace lj {
    _Use_decl_annotations_ auto event_driver_cleanup(_In_ WDFOBJECT driver) -> void {
        PAGED_CODE();

        lj::dlog("Cleanup up driver at {}", std::bit_cast<uptr>(driver));
    }
} // namespace lj

#pragma code_seg()

extern "C" __declspec(dllexport) auto APIENTRY DllMain(HMODULE module, DWORD, LPVOID) -> BOOL {
    if (not logger) logger = log::Logger::allocate_logger_instance<lj::KernelLogger>();

    lj::ilog("Calling DllMain...");

    DisableThreadLibraryCalls(module);

    return TRUE;
}
