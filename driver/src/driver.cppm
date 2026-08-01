module;

#include "windows.hpp"

export module lesserjoy.driver;

import std;

import stormkit.core;

import lesserjoy.constants;

using namespace stormkit;

export namespace lj {
    using slot = u8;

    struct Driver_context {
        Locked<array<slot, DRIVER_MAX_SLOTS>> slots = {};
    };

    STORMKIT_PUSH_WARNINGS
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
    using PDriver_context = Driver_context*;
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Driver_context, GetDriverContext)
    STORMKIT_POP_WARNINGS

    EVT_WDF_OBJECT_CONTEXT_CLEANUP event_driver_cleanup;
} // namespace lj
