//
//
//

#ifndef SIDER_PUMP_OP_POLLER_HH
#define SIDER_PUMP_OP_POLLER_HH

#include <atomic>

namespace sider::pump {
    namespace poller {
        struct
        op_poller_base {
            std::atomic<uint32_t> door_bell = 0;
        };

        template <typename op_t, typename prev_t>
        struct
        op_poller {
            op_t op;
            prev_t prev;
            auto
            poll() {
                prev.done();

                return op.value();
            }
        };
    }
}

#endif //SIDER_PUMP_OP_POLLER_HH
