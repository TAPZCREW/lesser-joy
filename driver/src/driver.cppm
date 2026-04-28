module;

#include <ntddk.h>

#include <wdf.h>

export module device;

import log;
import result;
import dyn_array;
import utilities;

struct SwitchController {
    int a = 0;
};

export namespace lj {
    auto find_switch_controllers() -> result<dyn_array<SwitchController>, NTSTATUS>;
}

module :private;

namespace lj {
    auto find_switch_controllers() -> result<dyn_array<SwitchController>, NTSTATUS> {
        auto out = dyn_array<SwitchController> {};

        for (auto i = 0; i < 10; ++i) out.push_back({ .a = i });

        return out;
    }
} // namespace lj
