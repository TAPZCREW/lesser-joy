module;

#include "wdk/wdfobject_macros.hpp"

#include <stormkit/core/platform_macro.hpp>

export module lesserjoy:driver;

import std;

import stormkit.core;
import stormkit.platform.win32;

import wdk.wdf;

import :constants;

using namespace stormkit;

export namespace lj {
    using slot = u8;

    struct driver_context {
        locked<array<slot, drive_max_slots>> slots = {};
    };

    STORMKIT_PUSH_WARNINGS
#pragma clang diagnostic ignored "-Wduplicate-decl-specifier"
    using Pdriver_context = driver_context*;
    WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(driver_context, get_driver_context)
    STORMKIT_POP_WARNINGS
} // namespace lj
