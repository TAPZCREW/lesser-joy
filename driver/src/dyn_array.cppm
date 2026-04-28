module;

#include <string.h>

export module dyn_array;

import meta;
import allocation;
import utilities;

export namespace lj {
    template<typename T>
    class dyn_array {
      public:
        dyn_array();
        ~dyn_array();

        dyn_array(const dyn_array&)                    = delete;
        auto operator=(const dyn_array&) -> dyn_array& = delete;

        dyn_array(dyn_array&&);
        auto operator=(dyn_array&&) -> dyn_array&;

        auto resize(size_t new_size) -> void;
        auto reserve(size_t capacity) -> void;
        auto shrink_to_fit() -> void;

        auto push_back(const T& value) -> T&;
        auto push_back(T&& value) -> T&;

        template<typename... Args>
        auto emplace_back(Args&&... args) -> T&;

        auto pop_back() -> void;

        auto begin(this auto& self) -> decltype(auto);
        auto cbegin() const -> const T*;

        auto end(this auto& self) -> decltype(auto);
        auto cend() const -> const T*;

        auto size() const -> size_t;
        auto capacity() const -> size_t;
        auto empty() const -> bool;

      private:
        auto reallocate(size_t capacity);

        size_t size_     = 0;
        size_t capacity_ = 0;

        byte* data_ = nullptr;
    };
} // namespace lj

namespace lj {
    template<typename T>
    dyn_array<T>::dyn_array() = default;

    template<typename T>
    dyn_array<T>::~dyn_array() {
        for (auto i = size_t { 0 }; i < size_; ++i) {
            auto ptr = launder(bit_cast<T*>(&data_[i * sizeof(T)]));
            ptr->~T();
        }

        delete[] data_;

        size_     = 0;
        capacity_ = 0;
        data_     = nullptr;
    }

    template<typename T>
    dyn_array<T>::dyn_array(dyn_array&& other)
        : size_ { exchange(other.size_, 0) },
          capacity_ { exchange(other.capacity_, 0) },
          data_ { exchange(other.data_, nullptr) } {
    }

    template<typename T>
    auto dyn_array<T>::operator=(dyn_array&& other) -> dyn_array& {
        if (&other == this) [[unlikely]]
            return *this;

        size_     = exchange(other.size_, 0);
        capacity_ = exchange(other.capacity_, 0);
        data_     = exchange(other.data_, nullptr);

        return *this;
    }

    template<typename T>
    auto dyn_array<T>::resize(size_t new_size) -> void {
        if (new_size == size_) return;

        if (new_size <= capacity_) {
            size_ = new_size;
            return;
        }

        reserve(new_size);

        if (new_size > size_) {
            auto diff = new_size - size_;
            for (auto i = size_t { 0 }; i < diff; ++i) { new (&data_[size_ + i]) T {}; }
        }

        size_ = new_size;
    }

    template<typename T>
    auto dyn_array<T>::reserve(size_t capacity) -> void {
        if (capacity <= capacity_) return;

        auto old = data_;

        data_ = new byte[capacity * sizeof(T)];

        if (old != nullptr) {
            if constexpr (meta::IsTriviallyCopyable<T>) memcpy(data_, old, size_ * sizeof(T));
            else {
                for (auto i = size_t { 0 }; i < size_; ++i) {
                    const auto index = size_ * sizeof(T);
                    auto       at    = &data_[index];
                    if constexpr (meta::IsMovable<T>) {
                        auto& other = *launder(bit_cast<T*>(&old[i * sizeof(T)]));
                        new (at) T { other };

                    } else {
                        const auto& other = *launder(bit_cast<const T*>(&old[i * sizeof(T)]));
                        new (at) T { other };
                    }
                }
            }

            delete[] old;
        }

        capacity_ = capacity;
    }

    template<typename T>
    auto dyn_array<T>::shrink_to_fit() -> void {
        if (capacity_ >= size_) {
            auto old = data_;

            data_ = new byte[size_ * sizeof(T)];

            if (old != nullptr) {
                if constexpr (meta::IsTriviallyCopyable<T>) memcpy(data_, old, size_ * sizeof(T));
                else {
                    for (auto i = size_t { 0 }; i < size_; ++i) {
                        const auto index = size_ * sizeof(T);
                        auto       at    = &data_[index];
                        if constexpr (meta::IsMovable<T>) {
                            auto& other = *launder(bit_cast<T*>(&old[i * sizeof(T)]));
                            new (at) T { other };

                        } else {
                            const auto& other = *launder(bit_cast<const T*>(&old[i * sizeof(T)]));
                            new (at) T { other };
                        }
                    }
                }

                delete[] old;
            }

            capacity_ = size_;
        }
    }

    template<typename T>
    auto dyn_array<T>::push_back(const T& value) -> T& {
        const auto index = size_ * sizeof(T);
        reserve(size_ + 1);
        auto at = &data_[index];

        auto ptr = new (at) T { move(value) };
        size_ += 1;
        return *ptr;
    }

    template<typename T>
    auto dyn_array<T>::push_back(T&& value) -> T& {
        const auto index = size_ * sizeof(T);
        reserve(size_ + 1);
        auto at = &data_[index];

        auto ptr = new (at) T { move(value) };
        size_ += 1;
        return *ptr;
    }

    template<typename T>
    template<typename... Args>
    auto dyn_array<T>::emplace_back(Args&&... args) -> T& {
        const auto index = size_ * sizeof(T);
        reserve(size_ + 1);
        auto at = &data_[index];

        auto ptr = new (at) T { forward(args)... };
        size_ += 1;
        return *ptr;
    }

    template<typename T>
    auto dyn_array<T>::pop_back() -> void {
        if (size_ == 0) return;

        const auto index = (size_ - 1) * sizeof(T);
        auto       at    = &data_[index];

        auto ptr = launder(bit_cast<T*>(at));
        ptr->~T();

        size_ -= 1;
    }

    template<typename T>
    auto dyn_array<T>::begin(this auto& self) -> decltype(auto) {
        using OutIt = meta::ForwardConst<decltype(self), T>*;
        if (self.data_ == nullptr) return static_cast<OutIt>(nullptr);

        return launder(bit_cast<OutIt>(self.data_));
    }

    template<typename T>
    auto dyn_array<T>::cbegin() const -> const T* {
        if (data_ == nullptr) return data_;

        return launder(bit_cast<const T*>(data_));
    }

    template<typename T>
    auto dyn_array<T>::end(this auto& self) -> decltype(auto) {
        using OutIt = meta::ForwardConst<decltype(self), T>*;
        if (self.data_ == nullptr) return static_cast<OutIt>(nullptr);

        return launder(bit_cast<OutIt>(self.data_) + self.size_);
    }

    template<typename T>
    auto dyn_array<T>::cend() const -> const T* {
        if (data_ == nullptr) return data_;

        return launder(bit_cast<const T*>(data_) + size_);
    }

    template<typename T>
    auto dyn_array<T>::size() const -> size_t {
        return size_;
    }

    template<typename T>
    auto dyn_array<T>::capacity() const -> size_t {
        return capacity_;
    }

    template<typename T>
    auto dyn_array<T>::empty() const -> bool {
        return size_ == 0;
    }
} // namespace lj
