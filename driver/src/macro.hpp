#ifndef LESSER_JOY_MACRO_HPP
#define LESSER_JOY_MACRO_HPP

#define Try(m)                                       \
    ({                                               \
        auto res = (m);                              \
        if (not res.has_value()) [[unlikely]]        \
            return Unexpected { move(res).error() }; \
        static_cast<decltype(res)&&>(res).value();   \
    })

#endif
