export module lesserjoy.constants;

import std;
import frozen;

import stormkit.core;

using namespace stormkit;

export namespace lj {
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
} // namespace lj
