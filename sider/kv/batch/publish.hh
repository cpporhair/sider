//

//

#ifndef SIDER_KV_BATCH_PUBLISH_HH
#define SIDER_KV_BATCH_PUBLISH_HH

#include "sider/kv/runtime/scheduler_objects.hh"

#include "./scheduler.hh"
#include "./chose_scheduler.hh"

namespace sider::kv::batch {
    auto
    publish(data::batch* b) {
        return chose_scheduler()->publish(b);
    }
}

#endif //SIDER_KV_BATCH_PUBLISH_HH
