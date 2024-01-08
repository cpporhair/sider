//

//

#ifndef SIDER_KV_INDEX_GET_HH
#define SIDER_KV_INDEX_GET_HH

#include "./scheduler.hh"
#include "./chose_scheduler.hh"

namespace sider::kv::index {
    auto
    get(data::slice* key, uint64_t read_sn, uint64_t free_sn) {
        return chose_scheduler(key)->get(key, read_sn, free_sn);
    }

    auto
    on_scheduler(const data::slice* key) {
        return chose_scheduler(key)->via();
    }
}

#endif //SIDER_KV_INDEX_GET_HH
