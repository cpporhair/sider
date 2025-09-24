//
//
//

#ifndef PUMP_HELPER_HH
#define PUMP_HELPER_HH

#include <tuple>

namespace pump {
    inline
    auto
    tuple_to_tie(auto& t) {
        return std::apply([](auto &...v) mutable { return std::tie(v...); }, t);
    }
}

#endif //PUMP_HELPER_HH
