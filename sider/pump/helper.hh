//
//
//

#ifndef SIDER_PUMP_HELPER_HH
#define SIDER_PUMP_HELPER_HH

#include <tuple>

namespace sider::pump {
    inline
    auto
    tuple_to_tie(auto& t) {
        return std::apply([](auto &...v) mutable { return std::tie(v...); }, t);
    }
}

#endif //SIDER_PUMP_HELPER_HH
