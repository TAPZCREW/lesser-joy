module;

#include <string.h>

export module string;

import utilities;
import allocation;

export namespace lj {
    class string {
      public:
        string() = default;
        template<size_t N>
        string(const char (*str)[N]);
        string(const char* str);
        string(const char* str, size_t size);

        ~string();

        string(const string&)                    = delete;
        auto operator=(const string&) -> string& = delete;

        string(string&&);
        auto operator=(string&&) -> string&;

        auto operator[](this auto& self, size_t i) -> decltype(auto);

        auto resize(size_t new_size) -> void;
        auto reserve(size_t capacity) -> void;
        auto shrink_to_fit() -> void;

        auto push_back(char value) -> char&;
        auto pop_back() -> void;

        auto begin(this auto& self) -> decltype(auto);
        auto cbegin() const -> const char*;

        auto end(this auto& self) -> decltype(auto);
        auto cend() const -> const char*;

        auto size() const -> size_t;
        auto capacity() const -> size_t;
        auto empty() const -> bool;

      private:
        size_t size_     = 0;
        size_t capacity_ = 0;

        char* data_ = nullptr;
    };
} // namespace lj

namespace lj {
    template<size_t N>
    string::string(const char (*str)[N]) : size_ { N }, capacity_ { size_ } {
        data_ = new char[size_ + 1];
        memcpy(data_, str, size_ + 1);

        data_[size_] = '\0';
    }

    auto string::begin(this auto& self) -> decltype(auto) {
        return forward<decltype(self)&>(self).data_;
    }

    auto string::end(this auto& self) -> decltype(auto) {
        return forward<decltype(self)&>(self).data_ + forward<decltype(self)&>(self).size_;
    }

    auto string::operator[](this auto& self, size_t i) -> decltype(auto) {
        return forward<decltype(self)&>(self).data_[i];
    }
} // namespace lj

module :private;

namespace lj {
    string::string(const char* str) {
        size_     = strlen(str);
        capacity_ = size_;

        data_ = new char[size_ + 1];
        memcpy(data_, str, size_ + 1);

        data_[size_] = '\0';
    }

    string::string(const char* str, size_t size) : size_ { size }, capacity_ { size_ } {
        data_ = new char[size_ + 1];

        memcpy(data_, str, size_);

        data_[size_] = '\0';
    }

    string::string(string&& other)
        : size_ { exchange(other.size_, 0) },
          capacity_ { exchange(other.capacity_, 0) },
          data_ { exchange(other.data_, nullptr) } {
    }

    auto string::operator=(string&& other) -> string& {
        if (&other == this) [[unlikely]]
            return *this;

        size_     = exchange(other.size_, 0);
        capacity_ = exchange(other.capacity_, 0);
        data_     = exchange(other.data_, nullptr);

        return *this;
    }

    string::~string() {
        if (data_ != nullptr) delete[] data_;

        size_     = 0;
        capacity_ = 0;
        data_     = nullptr;
    }

    auto string::resize(size_t new_size) -> void {
        if (new_size == size_) return;

        if (new_size <= capacity_) {
            size_ = new_size;
            return;
        }

        reserve(new_size);

        if (new_size > size_) {
            auto diff = new_size - size_;
            for (auto i = size_t { 0 }; i < diff; ++i) { new (&data_[size_ + i]) char {}; }
        }

        size_ = new_size;

        data_[size_] = '\0';
    }

    auto string::reserve(size_t capacity) -> void {
        if (capacity <= capacity_) return;

        auto old = data_;

        data_ = new char[capacity + 1];

        if (old != nullptr) {
            memcpy(data_, old, size_);

            delete[] old;
        }

        capacity_ = capacity;
    }

    auto string::shrink_to_fit() -> void {
        if (capacity_ >= size_) {
            auto old = data_;

            data_ = new char[size_ + 1];

            if (old != nullptr) {
                memcpy(data_, old, size_);

                delete[] old;
            }

            capacity_ = size_;

            data_[size_] = '\0';
        }
    }

    auto string::push_back(char value) -> char& {
        reserve(size_ + 1);
        auto& at = data_[size_];
        at       = value;
        size_ += 1;
        data_[size_] = '\0';

        return at;
    }

    auto string::pop_back() -> void {
        if (size_ == 0) return;

        data_[size_ - 1] = '\0';

        size_ -= 1;
    }

    auto string::cbegin() const -> const char* {
        return data_;
    }

    auto string::cend() const -> const char* {
        return data_ + size_;
    }

    auto string::size() const -> size_t {
        return size_;
    }

    auto string::capacity() const -> size_t {
        return capacity_;
    }

    auto string::empty() const -> bool {
        return size_ == 0;
    }
} // namespace lj
