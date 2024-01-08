//

//

#ifndef SIDER_KV_BATCH_START_HH
#define SIDER_KV_BATCH_START_HH

#include "sider/kv/runtime/scheduler_objects.hh"
#include "./chose_scheduler.hh"

namespace sider::kv::batch {
    auto
    start(data::batch* b) {
        return chose_scheduler()->start_batch(b);
    }
}

#endif //SIDER_KV_BATCH_START_HH
