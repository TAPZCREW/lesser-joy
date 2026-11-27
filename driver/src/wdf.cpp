module;

#include <stormkit/core/try_expected.hpp>

module lesserjoy;

namespace lj {
    ////////////////////////////////////////
    ////////////////////////////////////////
    auto fill_wdf_request_memory(wdk::WDFREQUEST request, array_view<const byte> data) noexcept -> system_result<void> {
        auto memory = wdk::WDFMEMORY {};
        CustomLoggedTry(lj::win_call(wdk::WdfRequestRetrieveOutputMemory, request, &memory),
                        elog,
                        "WdfRequestRetrieveOutputMemory failed!");

        Try(fill_wdf_memory(memory, data));
        wdk::WdfRequestSetInformation(request, stdr::size(data));

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_wdf_request_memory(wdk::WDFREQUEST request) noexcept -> system_result<array_view<const byte>> {
        auto ptr    = raw_ptr<void> { nullptr };
        auto length = 0_usize;

        CustomLoggedTry(lj::win_call(wdk::WdfRequestRetrieveInputBuffer, request, 1, &ptr, &length),
                        elog,
                        "WdfRequestRetrieveInputBuffer failed!");

        return {
            array_view { reinterpret_cast<const byte*>(ptr), length }
        };
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto fill_wdf_memory(wdk::WDFMEMORY memory, array_view<const byte> data) noexcept -> system_result<void> {
        auto size = 0_usize;
        wdk::WdfMemoryGetBuffer(memory, &size);
        if (size < stdr::size(data)) {
            lj::elog("fill_wdf_memory: target memory too small! Size {}, expects {}\n", size, stdr::size(data));
            return std::unexpected<system_error2::nt_code> { std::in_place, wdk::STATUS_INVALID_BUFFER_SIZE };
        }

        CustomLoggedTry(lj::win_call(wdk::WdfMemoryCopyFromBuffer,
                                     memory,
                                     0,
                                     const_cast<byte*>(stdr::data(data)),
                                     stdr::size(data)),
                        elog,
                        "WdfMemoryCopyFromBuffer failed!");

        return {};
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_wdf_memory(wdk::WDFMEMORY memory) noexcept -> system_result<dynarray<byte>> {
        auto size = 0_usize;
        wdk::WdfMemoryGetBuffer(memory, &size);

        auto buffer = dynarray<byte> {};
        if (size > 0) {
            buffer.resize(size, 0x00_b);

            CustomLoggedTry(lj::win_call(wdk::WdfMemoryCopyToBuffer, memory, 0, stdr::data(buffer), stdr::size(buffer)),
                            elog,
                            "WdfMemoryCopyToBuffer failed!");
        }

        return { std::move(buffer) };
    }

    ////////////////////////////////////////
    ////////////////////////////////////////
    auto get_wdf_memory(wdk::WDFMEMORY memory, array_view<byte> buffer) noexcept -> system_result<array_view<byte>> {
        auto size = 0_usize;
        wdk::WdfMemoryGetBuffer(memory, &size);

        if (stdr::size(buffer) < size) {
            lj::elog("get_wdf_memory: target buffer too small! Size {}, expects {}\n", stdr::size(buffer), size);
            return std::unexpected<system_error2::nt_code> { std::in_place, wdk::STATUS_INVALID_BUFFER_SIZE };
        }

        CustomLoggedTry(lj::win_call(wdk::WdfMemoryCopyToBuffer, memory, 0, stdr::data(buffer), size),
                        elog,
                        "WdfMemoryCopyFromBuffer failed!");

        if (stdr::size(buffer) > size) return { buffer.subspan(0, size) };

        return { std::move(buffer) };
    }
} // namespace lj
