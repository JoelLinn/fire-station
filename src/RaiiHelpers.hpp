#pragma once

namespace RAII {
template <auto Func>
struct DeleterFunc {
    template <typename T>
    constexpr void operator()(T *obj) const {
        if (obj) {
            Func(obj);
        }
    }
};
} // namespace RAII