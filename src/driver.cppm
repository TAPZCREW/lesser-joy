module;

#include <ntddk.h>
#include <wdf.h>

export module nsw2ble;

import log;

import nsw2ble.device;

export namespace nsw2ble {

  auto driver_main(DRIVER_OBJECT* driver_object, UNICODE_STRING* registry_path) -> NTSTATUS;

} // namespace nsw2ble
