module;

#include <string.h>

export module inplace_array;

import meta;
import allocation;
import utilities;
import log;

export namespace lj {
    template<typename T, size_t Capacity>
    class inplace_array {
        static constexpr auto CAPACITY = Capacity;

      public:
        inplace_array();
        ~inplace_array();

        inplace_array(const inplace_array&)                    = delete;
        auto operator=(const inplace_array&) -> inplace_array& = delete;

        inplace_array(inplace_array&&);
        auto operator=(inplace_array&&) -> inplace_array&;

        auto resize(size_t new_size) -> void;

        auto push_back(const T& value) -> T&;
        auto push_back(T&& value) -> T&;

        template<typename... Args>
        auto emplace_back(Args&&... args) -> T&;

        auto pop_back() -> void;

        auto begin(this auto& self) -> decltype(auto);
        auto cbegin() const -> decltype(auto);

        auto end(this auto& self) -> decltype(auto);
        auto cend() const -> decltype(auto);

        auto           size() const -> size_t;
        constexpr auto capacity() const -> size_t;
        auto           empty() const -> bool;

      private:
        size_t size_ = 0;

        byte* data_[Capacity * sizeof(T)];
    };
} // namespace lj

namespace lj {
    template<typename T, size_t Capacity>
    inplace_array<T, Capacity>::inplace_array() = default;

    template<typename T, size_t Capacity>
    inplace_array<T, Capacity>::~inplace_array() {
        for (auto i = size_t { 0 }; i < size_; ++i) {
            auto ptr = launder(bit_cast<T*>(&data_[i * sizeof(T)]));
            ptr->~T();
        }

        size_ = 0;
    }

    template<typename T, size_t Capacity>
    inplace_array<T, Capacity>::inplace_array(inplace_array&& other) : size_ { exchange(other.size_, 0) } {
        if (size_ > 0) {
            if constexpr (meta::IsTriviallyCopyable<T>) memcpy(data_, other.data_, size_ * sizeof(T));
            else {
                for (auto i = size_t { 0 }; i < size_; ++i) {
                    const auto index = size_ * sizeof(T);
                    auto       at    = &data_[index];
                    if constexpr (meta::IsMovable<T>) {
                        auto& other_ = *launder(bit_cast<T*>(&other.data_[i * sizeof(T)]));
                        new (at) T { other_ };

                    } else {
                        const auto& other_ = *launder(bit_cast<const T*>(&other.data_[i * sizeof(T)]));
                        new (at) T { other_ };
                    }
                }
            }
        }
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::operator=(inplace_array&& other) -> inplace_array& {
        if (&other == this) [[unlikely]]
            return *this;

        size_ = exchange(other.size_, 0);
        if (size_ > 0) {
            if constexpr (meta::IsTriviallyCopyable<T>) memcpy(data_, other.data_, size_ * sizeof(T));
            else {
                for (auto i = size_t { 0 }; i < size_; ++i) {
                    const auto index = size_ * sizeof(T);
                    auto       at    = &data_[index];
                    if constexpr (meta::IsMovable<T>) {
                        auto& other_ = *launder(bit_cast<T*>(&other.data_[i * sizeof(T)]));
                        new (at) T { other_ };

                    } else {
                        const auto& other_ = *launder(bit_cast<const T*>(&other.data_[i * sizeof(T)]));
                        new (at) T { other_ };
                    }
                }
            }
        }

        return *this;
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::resize(size_t new_size) -> void {
        if (new_size == size_) return;

        if (new_size > size_) {
            auto diff = new_size - size_;
            for (auto i = size_t { 0 }; i < diff; ++i) { new (&data_[size_ + i]) T {}; }
        }

        size_ = new_size;
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::push_back(const T& value) -> T& {
        const auto index = size_ * sizeof(T);
        auto       at    = &data_[index];

        auto ptr = new (at) T { move(value) };
        return *ptr;
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::push_back(T&& value) -> T& {
        const auto index = size_ * sizeof(T);
        auto       at    = &data_[index];

        auto ptr = new (at) T { move(value) };
        return *ptr;
    }

    template<typename T, size_t Capacity>
    template<typename... Args>
    auto inplace_array<T, Capacity>::emplace_back(Args&&... args) -> T& {
        const auto index = size_ * sizeof(T);
        auto       at    = &data_[index];

        auto ptr = new (at) T { forward(args)... };
        return *ptr;
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::pop_back() -> void {
        if (size_ == 0) return;

        const auto index = (size_ - 1) * sizeof(T);
        auto       at    = &data_[index];

        auto ptr = launder(bit_cast<T*>(at));
        ptr->~T();

        size_ -= 1;
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::begin(this auto& self) -> decltype(auto) {
        using OutIt = meta::ForwardConst<decltype(self), T>*;
        return launder(bit_cast<OutIt>(&self.data_[0]));
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::cbegin() const -> decltype(auto) {
        if (data_ == nullptr) return data_;

        return launder(bit_cast<const T*>(&data_[0]));
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::end(this auto& self) -> decltype(auto) {
        using OutIt = meta::ForwardConst<decltype(self), T>*;
        if (self.data_ == nullptr) return static_cast<OutIt>(nullptr);

        return launder(bit_cast<OutIt>(&self.data_[0]) + self.size_);
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::cend() const -> decltype(auto) {
        if (data_ == nullptr) return data_;

        return launder(bit_cast<const T*>(&data_[0]) + size_);
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::size() const -> size_t {
        return size_;
    }

    template<typename T, size_t Capacity>
    constexpr auto inplace_array<T, Capacity>::capacity() const -> size_t {
        return CAPACITY;
    }

    template<typename T, size_t Capacity>
    auto inplace_array<T, Capacity>::empty() const -> bool {
        return size_ == 0;
    }
} // namespace lj
