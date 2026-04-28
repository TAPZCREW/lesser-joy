module;

#define _VCRUNTIME_DISABLED_WARNINGS
#include <wdm.h>

#include <stdint.h>

export module allocation;

inline constexpr auto POOL_TAG_STR = "yoj-ressel";

export extern "C++" {
    using nullptr_t = decltype(nullptr);

    auto __cdecl operator new(size_t)->void*;
    auto __cdecl operator new[](size_t)->void*;

    constexpr auto __cdecl operator new(size_t, void*)->void*;
    constexpr auto __cdecl operator new[](size_t, void*)->void*;

    auto __cdecl operator delete(void*)->void;
    auto __cdecl operator delete[](void*)->void;
    auto __cdecl operator delete(void*, size_t)->void;
    auto __cdecl operator delete[](void*, size_t)->void;

    template<typename T>
    class heap {
      public:
        heap() : ptr_ { nullptr } {}

        explicit heap(nullptr_t) : ptr_ { nullptr } {}

        explicit heap(T* ptr) : ptr_ { ptr } {}

        ~heap() {
            if (ptr_ != nullptr) delete ptr_;
        }

        heap(const heap&)                    = delete;
        auto operator=(const heap&) -> heap& = delete;

        heap(heap&& other) : ptr_ { exchange(other.ptr_, nullptr) } {}

        auto operator=(heap&& other) -> heap& {
            if (&other == this) return *this;

            ptr_ = exchange(other.ptr_, nullptr);

            return *this;
        }

        operator T*() { return ptr_; }

        operator const T*() const { return as_const(ptr_); }

        auto operator->(this auto& self) { return self.ptr_; }

        auto get() -> T* { return ptr_; }

      private:
        T* ptr_;
    };

    template<typename T, typename... Args>
    auto allocate(Args && ... args) -> heap<T> {
        return heap<T> { new T { forward<Args>(args)... } };
    }
}

extern "C++" {
    constexpr auto operator new(size_t, void* ptr) -> void* {
        return ptr;
    }

    constexpr auto operator new[](size_t size, void* ptr) -> void* {
        return ptr;
    }
}

module :private;

namespace {
    const auto POOL_TAG = static_cast<ULONG>(reinterpret_cast<ULONGLONG>(POOL_TAG_STR));
}

extern "C++" {
    auto operator new(size_t size) -> void* {
        return ExAllocatePoolZero(POOL_TYPE::NonPagedPool, size, POOL_TAG);
    }

    auto operator new[](size_t size) -> void* {
        return ExAllocatePoolZero(POOL_TYPE::NonPagedPool, size, POOL_TAG);
    }

    auto operator delete(void* ptr) -> void {
        if (ptr == nullptr) return;

        ExFreePoolWithTag(ptr, POOL_TAG);
    }

    auto operator delete[](void* ptr) -> void {
        if (ptr == nullptr) return;

        ExFreePoolWithTag(ptr, POOL_TAG);
    }

    auto operator delete(void* ptr, size_t) -> void {
        if (ptr == nullptr) return;

        ExFreePoolWithTag(ptr, POOL_TAG);
    }

    auto operator delete[](void* ptr, size_t) -> void {
        if (ptr == nullptr) return;

        ExFreePoolWithTag(ptr, POOL_TAG);
    }
}
