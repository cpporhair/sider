//

//

#ifndef SIDER_KV_BATCH_CHOSE_SCHEDULER_HH
#define SIDER_KV_BATCH_CHOSE_SCHEDULER_HH

#include "sider/kv/runtime/scheduler_objects.hh"

namespace sider::kv::batch {
    inline
    auto
    chose_scheduler() {
        return runtime::batch_schedulers.list[0];
    }
}

#endif //SIDER_KV_BATCH_CHOSE_SCHEDULER_HH
