module;

#include <ntddk.h>
#include <wdf.h>

module nsw2ble;

import log;

import nsw2ble.device;

namespace nsw2ble {
  auto driver_main(DRIVER_OBJECT* driver_object, UNICODE_STRING* registry_path) -> NTSTATUS {
    debug_logln("Driver loaded");

    WDF_DRIVER_CONFIG       driver_config;
    WDF_DRIVER_CONFIG_INIT(&driver_config, device_add);
    driver_config.DriverInitFlags = WdfDriverInitNoDispatchOverride;

    WDF_OBJECT_ATTRIBUTES       attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    
    WDFDRIVER driver;
    auto status = WdfDriverCreate(
      driver_object,
      registry_path,
      &attributes,
      &driver_config,
      &driver
    );

    if (not NT_SUCCESS(status)) {
        // return status;
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
  }
} // namespace nsw2ble

