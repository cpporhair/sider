//

//

#ifndef SIDER_KV_FS_CHOSE_SCHEDULER_HH
#define SIDER_KV_FS_CHOSE_SCHEDULER_HH

#include "sider/kv/runtime/scheduler_objects.hh"

namespace sider::kv::fs {
    inline
    auto
    chose_scheduler() {
        return runtime::fs_schedulers.list[spdk_get_ticks() % runtime::fs_schedulers.list.size()];
    }

    inline
    auto
    chose_scheduler(uint32_t ssd_index) {
        return runtime::fs_schedulers.list[ssd_index];
    }
}

#endif //SIDER_KV_FS_CHOSE_SCHEDULER_HH
