//

//

#ifndef SIDER_KV_BATCH_ALLOCATE_PUT_ID_HH
#define SIDER_KV_BATCH_ALLOCATE_PUT_ID_HH

#include "sider/kv/runtime/scheduler_objects.hh"
#include "./scheduler.hh"
#include "./chose_scheduler.hh"

namespace sider::kv::batch {
    auto
    allocate_put_id(data::batch* b) {
        return chose_scheduler()->allocate_put_snapshot(b);
    }
}

#endif //SIDER_KV_BATCH_ALLOCATE_PUT_ID_HH
