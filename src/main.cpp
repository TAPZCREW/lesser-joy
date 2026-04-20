#include <ntddk.h>
#include <wdf.h>

import nsw2ble;

extern "C" auto DriverEntry(DRIVER_OBJECT* driver_object, UNICODE_STRING* registry_path) -> NTSTATUS {
    return nsw2ble::driver_main(driver_object, registry_path);
}
