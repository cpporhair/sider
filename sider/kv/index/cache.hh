//

//

#ifndef SIDER_KV_INDEX_CACHE_HH
#define SIDER_KV_INDEX_CACHE_HH

#include "./scheduler.hh"
#include "./chose_scheduler.hh"

namespace sider::kv::index {
    auto
    cache(data::data_file* f) {
        return chose_scheduler(f)->cache_file(f);
    }
}

#endif //SIDER_KV_INDEX_CACHE_HH
