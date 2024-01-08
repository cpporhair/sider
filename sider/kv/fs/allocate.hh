//

//

#ifndef SIDER_KV_FS_ALLOCATE_HH
#define SIDER_KV_FS_ALLOCATE_HH

#include "./scheduler.hh"
#include "./chose_scheduler.hh"

namespace sider::kv::fs {
    auto
    allocate_data_page(data::batch* b) {
        return chose_scheduler()->allocate_data_page(b);
    }

    auto
    allocate_meta_page(_allocate::leader_res* res) {
        return chose_scheduler()->allocate_meta_page(res);
    }
}

#endif //SIDER_KV_FS_ALLOCATE_HH
