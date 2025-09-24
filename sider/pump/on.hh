//
//
//

#ifndef PUMP_ON_HH
#define PUMP_ON_HH

#include "./flat.hh"
#include "./then.hh"

namespace pump {
    template <typename sender_t>
    auto
    on(sender_t&& s) {
        return then([s = __fwd__(s)](auto&& ...v) mutable {
                return __fwd__(s)
                    >> forward_value(__fwd__(v)...);
            })
            >> flat();
    }

    template <typename sender_t, typename bind_back_t>
    auto
    on(sender_t&& s, bind_back_t&& b) {
        return __fwd__(b) >> on(__fwd__(s));
    }
}

#endif //MONISM_ON_HH
