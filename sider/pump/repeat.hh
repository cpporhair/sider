#ifndef SIDER_PUMP_REPEAT_HH
#define SIDER_PUMP_REPEAT_HH

#include <ranges>

#include "sider/pump/helper.hh"

#include "./op_tuple_builder.hh"
#include "./for_each.hh"

namespace sider::pump {
    auto
    repeat(uint32_t count) {
        return for_each(std::views::iota(uint32_t(0),count));
    }

    auto
    forever() {
        return for_each(std::views::iota(uint32_t(0)));
    }
}

#endif //SIDER_PUMP_REPEAT_HH
