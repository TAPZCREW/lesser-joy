module;

#include "windows.hpp"

#include <stormkit/core/try_expected.hpp>

module lesserjoy.device;

import lesserjoy.log;
import lesserjoy.wdf;

namespace lj {
    ////////////////////////////////////////
    ////////////////////////////////////////
    _Use_decl_annotations_ auto event_driver_cleanup(_In_ WDFOBJECT driver) -> void {
        lj::dlog("Driver successfully cleanup up!");
    }
} // namespace lj

#pragma code_seg("INIT")

extern "C++" {
    extern "C" DRIVER_INITIALIZE DriverEntry;

    ////////////////////////////////////////
    ////////////////////////////////////////
    extern "C" _Use_decl_annotations_ auto DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path)
      -> NTSTATUS {
        lj::ilog("Initializing lesserjoy driver...");

        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.EvtCleanupCallback = lj::event_driver_cleanup;

        auto config = WDF_DRIVER_CONFIG {};
        WDF_DRIVER_CONFIG_INIT(&config, lj::event_device_add);

        CustomLoggedTryOr(lj::win_call(WdfDriverCreate, driver_object, registry_path, &attributes, &config, WDF_NO_HANDLE),
                          monadic::unwrap(),
                          lj::elog,
                          "Failed to initialize lessjoy driver!{}");

        lj::ilog("Driver successfully initialized!");

        return 0;
    }
}

#pragma code_seg()
