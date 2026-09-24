module;

#include "windows.hpp"

#include <stormkit/core/try_expected.hpp>

export module lesserjoy.wdf;

import std;
import stormkit.core;

import lesserjoy.constants;
import lesserjoy.log;

using namespace stormkit;
using namespace stormkit::literals;

namespace stdr = std::ranges;

export namespace lj {
    template<typename Func, typename... Args>
    auto win_call(Func&& func, Args&&... args) noexcept -> system_result<void>;

    auto wdf_memory_allocate(usize size, WDFOBJECT parent = WDF_NO_HANDLE) noexcept
      -> system_result<std::pair<WDFMEMORY, array_view<byte>>>;

    auto fill_wdf_request_memory(WDFREQUEST request, array_view<const byte> data) noexcept -> system_result<void>;
    auto get_wdf_request_memory(WDFREQUEST request) noexcept -> system_result<dynarray<byte>>;

    auto fill_wdf_memory(WDFMEMORY memory, array_view<const byte> data) noexcept -> system_result<void>;
    auto get_wdf_memory(WDFMEMORY memory) noexcept -> system_result<dynarray<byte>>;
    auto get_wdf_memory(WDFMEMORY memory, array_view<byte> buffer) noexcept -> system_result<array_view<byte>>;
} // namespace lj

////////////////////////////////////////////////////////////////////
///                      IMPLEMENTATION                          ///
////////////////////////////////////////////////////////////////////

namespace lj {
    ////////////////////////////////////////
    ////////////////////////////////////////
    template<typename Func, typename... Args>
    inline auto win_call(Func&& func, Args&&... args) noexcept -> system_result<void> {
        auto expected = system_result<void> {};

        const auto status = std::forward<Func>(func)(std::forward<Args>(args)...);
        if (not NT_SUCCESS(status)) expected = std::unexpected { error_code::from_ntstatus(status) };

        return expected;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto wdf_memory_allocate(usize size, WDFOBJECT parent) noexcept -> system_result<std::pair<WDFMEMORY, array_view<byte>>> {
        auto attributes = WDF_OBJECT_ATTRIBUTES {};
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = parent;

        auto memory     = WDFMEMORY {};
        auto buffer_ptr = PVOID { nullptr };

        CustomLoggedTry(lj::win_call(WdfMemoryCreate, &attributes, NonPagedPoolNx, DRIVER_POOL_TAG, size, &memory, &buffer_ptr),
                        dlog,
                        "WdfMemoryCreate failed!");

        return { std::make_pair(memory, array_view<byte> { std::bit_cast<byte*>(buffer_ptr), size }) };
    }
} // namespace lj
