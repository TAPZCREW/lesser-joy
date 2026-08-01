module;

#include "windows.hpp"

#include <stormkit/core/try_expected.hpp>

module lesserjoy.wdf;

namespace lj {
    ////////////////////////////////////////
    ////////////////////////////////////////
    auto fill_wdf_request_memory(WDFREQUEST request, array_view<const byte> data) noexcept -> Expected<void> {
        auto memory = WDFMEMORY {};
        CustomLoggedTry(lj::win_call(WdfRequestRetrieveOutputMemory, request, &memory),
                        dlog,
                        "WdfRequestRetrieveOutputMemory failed!");

        Try(fill_wdf_memory(memory, data));
        WdfRequestSetInformation(request, stdr::size(data));

        Return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_wdf_request_memory(WDFREQUEST request) noexcept -> Expected<dynarray<byte>> {
        auto memory = WDFMEMORY {};
        CustomLoggedTry(lj::win_call(WdfRequestRetrieveOutputMemory, request, &memory),
                        dlog,
                        "WdfRequestRetrieveOutputMemory failed!");

        Return get_wdf_memory(memory);
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto fill_wdf_memory(WDFMEMORY memory, array_view<const byte> data) noexcept -> Expected<void> {
        auto size = 0_usize;
        WdfMemoryGetBuffer(memory, &size);
        if (size < stdr::size(data)) {
            lj::dlog("fill_wdf_memory: target memory too small! Size {}, expects {}\n", size, stdr::size(data));
            Return std::unexpected<system_error2::nt_code> { std::in_place, STATUS_INVALID_BUFFER_SIZE };
        }

        CustomLoggedTry(lj::win_call(WdfMemoryCopyFromBuffer, memory, 0, bit_cast<void*>(stdr::data(data)), stdr::size(data)),
                        dlog,
                        "WdfMemoryCopyFromBuffer failed!");

        Return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_wdf_memory(WDFMEMORY memory) noexcept -> Expected<dynarray<byte>> {
        auto size = 0_usize;
        WdfMemoryGetBuffer(memory, &size);

        auto buffer = dynarray<byte> {};
        buffer.resize(size, 0x00_b);

        CustomLoggedTry(lj::win_call(WdfMemoryCopyToBuffer, memory, 0, bit_cast<void*>(stdr::data(buffer)), stdr::size(buffer)),
                        dlog,
                        "WdfMemoryCopyFromBuffer failed!");

        Return { std::move(buffer) };
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_wdf_memory(WDFMEMORY memory, array_view<byte> buffer) noexcept -> Expected<array_view<byte>> {
        auto size = 0_usize;
        WdfMemoryGetBuffer(memory, &size);

        if (stdr::size(buffer) < size) {
            lj::dlog("get_wdf_memory: target buffer too small! Size {}, expects {}\n", stdr::size(buffer), size);
            Return std::unexpected<system_error2::nt_code> { std::in_place, STATUS_INVALID_BUFFER_SIZE };
        }

        CustomLoggedTry(lj::win_call(WdfMemoryCopyToBuffer, memory, 0, bit_cast<void*>(stdr::data(buffer)), size),
                        dlog,
                        "WdfMemoryCopyFromBuffer failed!");

        if (stdr::size(buffer) > size) return { buffer.subspan(0, size) };

        Return { std::move(buffer) };
    }
} // namespace lj
