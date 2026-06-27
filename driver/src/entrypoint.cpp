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

extern "C" _Use_decl_annotations_ auto DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path)
  -> NTSTATUS {
    auto _ = log::Logger::create_logger_instance<lj::KernelLogger>();

    lj::dlog("Initializing lesserjoy driver...");

    auto attributes = WDF_OBJECT_ATTRIBUTES {};
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    auto config = WDF_DRIVER_CONFIG {};
    WDF_DRIVER_CONFIG_INIT(&config, lj::event_device_add);

    auto status = WdfDriverCreate(driver_object, registry_path, &attributes, &config, WDF_NO_HANDLE);

    if (not NT_SUCCESS(status)) lj::dlog("Failed to initialize lesserjoy driver! status: {:x}", status);
    else
        lj::dlog("lesserjoy: driver successfully initialized!");

    return status;
}
