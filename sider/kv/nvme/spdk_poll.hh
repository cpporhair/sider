//

//

#ifndef SIDER_KV_NVME_SPDK_POLL_HH
#define SIDER_KV_NVME_SPDK_POLL_HH

#include "spdk/log.h"

namespace sider::kv::nvme {
    static
    auto
    disconnect_cb(spdk_nvme_qpair *qpair, void *ctx){
    }

    auto
    poll(spdk_nvme_poll_group *group) {
        auto res = spdk_nvme_poll_group_process_completions(group, 0, disconnect_cb);
        if (res < 0)
            SPDK_ERRLOG("error,%ld",res);
    }
}

#endif //SIDER_KV_NVME_SPDK_POLL_HH
