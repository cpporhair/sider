//

//

#ifndef SIDER_KV_NVME_CHOSE_SCHEDULER_HH
#define SIDER_KV_NVME_CHOSE_SCHEDULER_HH

#include "sider/kv/runtime/scheduler_objects.hh"

namespace sider::kv::nvme {

    std::atomic<uint64_t> pos;

    inline
    auto
    chose_scheduler(uint32_t ssd_index) {
        return runtime::nvme_schedulers_per_ssd[ssd_index]
            .list[spdk_get_ticks() % runtime::nvme_schedulers_per_ssd[ssd_index].list.size()];
    }
}

#endif //SIDER_KV_NVME_CHOSE_SCHEDULER_HH
