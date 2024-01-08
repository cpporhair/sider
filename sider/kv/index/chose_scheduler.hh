//

//

#ifndef SIDER_KV_INDEX_CHOSE_SCHEDULER_HH
#define SIDER_KV_INDEX_CHOSE_SCHEDULER_HH

#include "sider/kv/runtime/scheduler_objects.hh"

namespace sider::kv::index {
    inline
    auto
    chose_scheduler(const data::data_file* f) {
        uint32_t res = f->key_seed();
        return runtime::index_schedulers.list[res % runtime::index_schedulers.list.size()];
    }

    inline
    auto
    chose_scheduler(const data::slice* k) {
        uint32_t res = 0;
        memcpy(&res, k->ptr, k->len < sizeof(uint32_t) ? k->len : sizeof(uint32_t));
        return runtime::index_schedulers.list[res % runtime::index_schedulers.list.size()];
    }
}

#endif //SIDER_KV_INDEX_CHOSE_SCHEDULER_HH
