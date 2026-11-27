module;

#include <stormkit/core/try_expected.hpp>

#include "contract_macro.hpp"

export module lesserjoy:wdf;

import std;
import stormkit.core;
import stormkit.platform.win32;

import wdk.wdf;
import wdk.ntstatus;

import :constants;
import :log;

using namespace stormkit;
using namespace stormkit::literals;

namespace stdr = std::ranges;

export namespace lj {
    template<typename Func, typename... Ts>
    auto win_call(Func&& func, Ts&&... args) noexcept -> system_result<void>;

    auto wdf_memory_allocate(usize size, wdk::WDFOBJECT parent = wdk::WDF_NO_HANDLE) noexcept
      -> system_result<std::pair<wdk::WDFMEMORY, array_view<byte>>>;

    auto fill_wdf_request_memory(wdk::WDFREQUEST request, array_view<const byte> data) noexcept -> system_result<void>;
    auto get_wdf_request_memory(wdk::WDFREQUEST request) noexcept -> system_result<array_view<const byte>>;

    auto fill_wdf_memory(wdk::WDFMEMORY memory, array_view<const byte> data) noexcept -> system_result<void>;
    auto get_wdf_memory(wdk::WDFMEMORY memory) noexcept -> system_result<dynarray<byte>>;
    auto get_wdf_memory(wdk::WDFMEMORY memory, array_view<byte> buffer) noexcept -> system_result<array_view<byte>>;

    template<meta::negate<meta::is, void> T, auto Get_context>
    auto destroy_ctx(wdk::WDFOBJECT object) noexcept -> void;
} // namespace lj

////////////////////////////////////////////////////////////////////
///                      IMPLEMENTATION                          ///
////////////////////////////////////////////////////////////////////

namespace lj {
    ////////////////////////////////////////
    ////////////////////////////////////////
    template<typename Func, typename... Ts>
    inline auto win_call(Func&& func, Ts&&... args) noexcept -> system_result<void> {
        auto expected = system_result<void> {};

        const auto status = std::forward<Func>(func)(std::forward<Ts>(args)...);
        if (not wdk::NtSuccess(status)) expected = std::unexpected { error_code::from_ntstatus(status) };

        return expected;
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto wdf_memory_allocate(usize size, wdk::WDFOBJECT parent) noexcept
      -> system_result<std::pair<wdk::WDFMEMORY, array_view<byte>>> {
        auto attributes = wdk::WDF_OBJECT_ATTRIBUTES {};
        wdk::WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = parent;

        auto memory     = wdk::WDFMEMORY {};
        auto buffer_ptr = raw_ptr<void> { nullptr };

        CustomLoggedTry(lj::win_call(wdk::WdfMemoryCreate,
                                     &attributes,
                                     wdk::NonPagedPoolNx,
                                     drive_pool_tag,
                                     size,
                                     &memory,
                                     &buffer_ptr),
                        dlog,
                        "WdfMemoryCreate failed!");

        return { std::make_pair(memory, array_view<byte> { reinterpret_cast<byte*>(buffer_ptr), size }) };
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    template<meta::negate<meta::is, void> T, auto Get_context>
    auto destroy_ctx(wdk::WDFOBJECT object) noexcept -> void {
        lj::dlog("Destroying {}", std::bit_cast<uptr>(object));

        // Manually call the destructor because contextes are constructed with placement new
        auto ctx = Get_context(object);
        WDK_EXPECTS_VOID(ctx != nullptr);
        ctx->~T();
    }

} // namespace lj
