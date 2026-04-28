module;

#include <string.h>

#include <ntddk.h>

#include <wdf.h>

export module string_view;

import string;

export namespace lj {
    class String_view {
      public:
        template<size_t N>
        constexpr String_view(const char (*str)[N]);
        constexpr String_view(const char* str);
        String_view(const string& str);
        constexpr String_view(const char* str, size_t n);

        constexpr auto size() const -> size_t;
        constexpr auto data() const -> const char*;

        constexpr auto operator[](size_t i) const -> size_t;

      private:
        const char* m_data = nullptr;
        size_t      m_size = 0;
    };
} // namespace lj

namespace lj {
    template<size_t N>
    constexpr String_view::String_view(const char (*str)[N]) : m_data { str }, m_size { N } {
    }

    constexpr String_view::String_view(const char* str) : m_data { str }, m_size { 0 } {
        if consteval {
            auto c = str;
            while (*c != '\0') {
                m_size += 1;
                ++c;
            }
        } else {
            m_size = strlen(m_data);
        }
    }

    constexpr String_view::String_view(const char* str, size_t n) : m_data { str }, m_size { n } {
    }

    String_view::String_view(const string& str) : m_data { str.cbegin() }, m_size { str.size() } {
    }

    constexpr auto String_view::size() const -> size_t {
        return m_size;
    }

    constexpr auto String_view::data() const -> const char* {
        return m_data;
    }

    constexpr auto String_view::operator[](size_t i) const -> size_t {
        if not consteval {
            if (i >= m_size) {
                DbgPrint("OOB access to str ");
                DbgPrint(m_data);
                DbgPrint(" with i = "); // TODO implement int to str
            }
        }
        return m_data[i];
    }
} // namespace lj
