module;

#include "windows.hpp"

export module lesserjoy.ntstatus;

import std;

import stormkit.core;

using namespace stormkit;

export {
    namespace lj {
        template<typename T>
        using Expected = core::Expected<T, system_error2::nt_code>;

        template<typename Func, typename... Args>
        auto win_call(Func&& func, Args&&... args) noexcept -> Expected<void> {
            auto expected = Expected<void> {};

            const auto status = std::forward<Func>(func)(std::forward<Args>(args)...);
            if (not NT_SUCCESS(status)) expected = std::unexpected<system_error2::nt_code> { std::in_place, status };

            return expected;
        }
    } // namespace lj
}
