module;

#include <ntddk.h>
#include <wdf.h>

export module nsw2ble.device;

export namespace nsw2ble {

  auto device_add(WDFDRIVER driver, WDFDEVICE_INIT* device_init) -> NTSTATUS;

} // namespace nsw2ble
