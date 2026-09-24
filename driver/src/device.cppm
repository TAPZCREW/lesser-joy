module;

#include "windows.hpp"

#include "usb.hpp"

export module lesserjoy.device;

import std;

import stormkit.core;

import lesserjoy.constants;
import lesserjoy.common;

using namespace stormkit;

namespace stdr = std::ranges;

export namespace lj {
    STORMKIT_PUSH_WARNINGS
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(device_context, get_device_context)
    STORMKIT_POP_WARNINGS

    STORMKIT_PUSH_WARNINGS
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(queue_context, get_queue_context)
    STORMKIT_POP_WARNINGS

    EVT_WDF_DRIVER_DEVICE_ADD      event_device_add;
    EVT_WDF_OBJECT_CONTEXT_CLEANUP event_device_cleanup;
} // namespace lj
