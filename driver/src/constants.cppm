module;

#define WIN32_NO_STATUS
#include <stormkit/core/platform/windows.hpp>
#undef WIN32_NO_STATUS
#include <devpropdef.h>

#include <wdf.h>

export module lesserjoy.constants;

import std;
import frozen;

import stormkit.core;

using namespace stormkit;

export {
    namespace lj {
        struct Controller_type {
            string_view name;
            string_view vendor;
            u16         vid;
            u16         pid;
            string_view product_string;
            string_view manufacturer_string;
        };

        inline constexpr auto CONTROLLERS_TYPE = frozen::unordered_map<frozen::string, Controller_type, 1> {
            { "pro_controller",
             { .name                = "Nintendo Switch 2 Pro Controller",
                .vendor              = "Nintendo",
                .vid                 = 0x057E,
                .pid                 = 0x2069,
                .product_string      = "Pro Controller",
                .manufacturer_string = "Nintendo Co., Ltd." } },
        };

        inline constexpr auto DEVICE_INTERFACE_GUID = DEVPROPKEY {
            .fmtid = { 0xb65fc751, 0x8225, 0x4f9b, { 0xba, 0x85, 0x97, 0x62, 0x7c, 0xc0, 0x42, 0xb8 } },
            .pid   = 0xe5
        };

        inline constexpr auto POOL_TAG = u32 { 'lyoj' };
    } // namespace lj

    constexpr auto format_as(NTSTATUS status, auto& ctx) noexcept -> decltype(ctx.out()) {
        std::format_to(ctx, "{:#x}", static_cast<u32>(status));
    }
}
