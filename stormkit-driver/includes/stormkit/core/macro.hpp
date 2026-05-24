#ifndef LESSER_JOY_MACRO_HPP
#define LESSER_JOY_MACRO_HPP

#define Try(m)                                       \
    ({                                               \
        auto res = (m);                              \
        if (not res.has_value()) [[unlikely]]        \
            return Unexpected { move(res).error() }; \
        static_cast<decltype(res)&&>(res).value();   \
    })

#define NT_Try(m)                                \
    ({                                           \
        auto status = (m);                       \
        if (not NT_SUCCESS(status)) [[unlikely]] \
            return Unexpected { status };        \
    })

#define Assert(m, e)                 \
    ({                               \
        auto status = (m);           \
        if (not status) [[unlikely]] \
            return Unexpected { e }; \
    })
#endif
