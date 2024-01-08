//

//

#ifndef SIDER_KV_FS_FREE_HH
#define SIDER_KV_FS_FREE_HH

#include "./scheduler.hh"
#include "./chose_scheduler.hh"

namespace sider::kv::fs {
    auto
    free_span(data::write_span& span) {
        return chose_scheduler(span.ssd_index)->free(span);
    }

}

#endif //SIDER_KV_FS_FREE_HH
