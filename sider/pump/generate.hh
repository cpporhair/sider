//
//
//

#ifndef SIDER_PUMP_GENERATE_HH
#define SIDER_PUMP_GENERATE_HH

#include "sider/coro/coro.hh"

#include "./for_each.hh"
#include "./on.hh"

namespace sider::pump {
    inline constexpr _for_each::fn generate{};
    inline constexpr _for_each::fn stream{};
    inline constexpr _for_each::fn range{};

    auto
    loop(size_t n) {
        return generate(std::views::iota(size_t(0), n));
    }

    auto
    as_stream(auto&& a) {
        return stream(coro::make_view_able(__fwd__(a)));
    }
}

#endif //SIDER_PUMP_GENERATE_HH
