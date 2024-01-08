//

//

#ifndef SIDER_KV_INDEX_UPDATE_HH
#define SIDER_KV_INDEX_UPDATE_HH

#include "./scheduler.hh"
#include "./chose_scheduler.hh"

namespace sider::kv::index {

    auto
    update(data::data_file* f) {
        return chose_scheduler(f)->update(f);
    }

}

#endif //SIDER_KV_INDEX_UPDATE_HH
