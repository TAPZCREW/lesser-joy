#include <ntddk.h>

#include <wdf.h>

#include <stdlib.h>
#include <string.h>

import device;
import log;
import utilities;
import allocation;
import inplace_array;
import string;
import utilities;

extern "C" auto     DriverEntry(DRIVER_OBJECT* driver_object, UNICODE_STRING* registry_path) -> NTSTATUS;
extern "C" NTSTATUS Bus_EvtDeviceAdd(IN WDFDRIVER, IN PWDFDEVICE_INIT);

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, Bus_EvtDeviceAdd)
#endif

namespace lj {
    auto driver_exit(DRIVER_OBJECT* driver_object) {
        debug_logln("lesser-joy stopped!");
    }
} // namespace lj

extern "C" auto DriverEntry(DRIVER_OBJECT* driver_object, UNICODE_STRING* registry_path) -> NTSTATUS {
    lj::debug_logln("Initializing lesser-joy driver...");

    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    auto attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    auto config = WDF_DRIVER_CONFIG {};
    WDF_DRIVER_CONFIG_INIT(&config, Bus_EvtDeviceAdd);

    auto driver = WDFDRIVER {};

    auto status = WdfDriverCreate(driver_object, registry_path, &attributes, &config, &driver);

    driver_object->DriverUnload = lj::driver_exit;

    if (not NT_SUCCESS(status)) KdPrint(("Failed to initialize lesser-joy driver! status: 0x%x\n", status));

    lj::debug_logln("lesser-joy driver successfully initialized!");

    return status;
}

extern "C" NTSTATUS Bus_EvtDeviceAdd(IN WDFDRIVER, IN PWDFDEVICE_INIT DeviceInit) {
    PAGED_CODE();

    lj::debug_logln("lesser-joy: creating virtual device!");

    return 0;
}
